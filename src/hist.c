/*
 *  Citrusleaf Foundation
 *  src/timer.c - timer functionality
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"

// #define DEBUG 1




histogram * 
histogram_create(char *name)
{
	histogram * h = cf_malloc(sizeof(histogram));
	if (!h)	return(0);
	if (strlen(name) >= sizeof(h->name)-1) { cf_free(h); return(0); }
	strcpy(h->name, name);
	h->n_counts = 0;
	memset(&h->count, 0, sizeof(h->count));
	return(h);
}

void histogram_clear(histogram *h)
{
	cf_atomic_int_set(&h->n_counts, 0);

	for (int i = 0; i < N_COUNTS; i++) {
		cf_atomic_int_set(&h->count[i], 0);
	}
}

void histogram_dump( histogram *h )
{
	char printbuf[100];
	int pos = 0; // location to print from
	printbuf[0] = '\0';
	
	cf_info(AS_INFO, "histogram dump: %s (%zu total)",h->name, h->n_counts);
	int i, j;
	int k = 0;
	for (j=N_COUNTS-1 ; j >= 0 ; j-- ) if (h->count[j]) break;
	for (i=0;i<N_COUNTS;i++) if (h->count[i]) break;
	for (; i<=j;i++) {
		if (h->count[i] > 0) { // print only non zero columns
			int bytes = sprintf((char *) (printbuf + pos), " (%02d: %010zu) ", i, h->count[i]);
			if (bytes <= 0) 
			{
				cf_info(AS_INFO, "histogram printing error. Bailing ...");
				return;
			}
			pos += bytes;
		    if (k % 4 == 3){
		    	 cf_info(AS_INFO, "%s", (char *) printbuf);
		    	 pos = 0;
		    	 printbuf[0] = '\0';
		    }
		    k++;
		}
	}
	if (pos > 0) 
	    cf_info(AS_INFO, "%s", (char *) printbuf);
}

#ifdef USE_CLOCK

void histogram_start( histogram *h, histogram_measure *hm)
{
	cf_atomic_int_incr(&h->n_counts);
//	clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &hm->start);
	clock_gettime( CLOCK_MONOTONIC, &hm->start);
}

void histogram_stop(histogram *h, histogram_measure *hm)
{
	struct timespec now_ts;
//	clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &now_ts);
	clock_gettime( CLOCK_MONOTONIC, &now_ts);
	uint64_t start = (hm->start.tv_sec * 1000000000L) + hm->start.tv_nsec;
	uint64_t now = (now_ts.tv_sec * 1000000000L) + now_ts.tv_nsec;
	uint64_t delta = now - start;
	
	int index = bits_find_last_set_64(delta);
	if (index < 0) index = 0;
	
	cf_atomic_int_incr( &h->count[ index ] );
	
}
#endif // USE CLOCK


#ifdef USE_GETCYCLES

void histogram_start( histogram *h, histogram_measure *hm)
{
	cf_atomic_int_incr(&h->n_counts);
	hm->start = cf_getms();
//	hm->start = hist_getcycles();
}

void histogram_stop(histogram *h, histogram_measure *hm)
{
//	uint64_t delta = hist_getcycles() - hm->start;
    uint64_t delta = cf_getms() - hm->start;
	
	int index = bits_find_last_set_64(delta);
	if (index < 0) index = 0;
	
	cf_atomic_int_incr( &h->count[ index ] );
	
}

void histogram_insert_data_point( histogram *h, uint64_t start)
{
	cf_atomic_int_incr(&h->n_counts);
	
    uint64_t end = cf_getms(); 
    uint64_t delta = end - start;
	
	int index = bits_find_last_set_64(delta);
	if (index < 0) index = 0;   
	if (start > end)
	{
	    // Need to investigate why in some cases start is a couple of ms greater than end
		// Could it be rounding error (usually the difference is 1 but sometimes I have seen 2
	    // cf_info(AS_INFO, "start = %"PRIu64" > end = %"PRIu64"", start, end);
		index = 0;
	}   
       
	cf_atomic_int_incr( &h->count[ index ] );
	
}

#endif // USE_GETCYCLES


void histogram_get_counts(histogram *h, histogram_counts *hc)
{
	for (int i=0;i<N_COUNTS;i++)
		hc->count[i] = h->count[i];
	return;
}

linear_histogram * 
linear_histogram_create(char *name, uint64_t start, uint64_t max_offset, int num_buckets)
{
	if (num_buckets > MAX_LINEAR_BUCKETS) {
		cf_crash(AS_INFO, CF_GLOBAL, "linear histogram num_buckets %u > max %u", num_buckets, MAX_LINEAR_BUCKETS);
	}

	linear_histogram * h = cf_malloc(sizeof(linear_histogram));
	if (!h)	return(0);
	if (strlen(name) >= sizeof(h->name)-1) { cf_free(h); return(0); }
	strcpy(h->name, name);
	h->n_counts = 0;
	h->num_buckets = num_buckets;
	h->start = start;
    h->bucket_offset = max_offset / h->num_buckets;
	if (h->bucket_offset == 0) // avoid divide by zero while inserting data point
		h->bucket_offset = 1;
	memset(&h->count, 0, sizeof(h->count));
	return(h);
}

// Note: not thread safe!
void linear_histogram_clear(linear_histogram *h, uint64_t start, uint64_t max_offset)
{
	h->n_counts = 0;
	h->start = start;
    h->bucket_offset = max_offset / h->num_buckets;
	if (h->bucket_offset == 0) // avoid divide by zero while inserting data point
		h->bucket_offset = 1;
	memset(&h->count, 0, sizeof(h->count));
}

void linear_histogram_insert_data_point( linear_histogram *h, uint64_t point)
{
	cf_atomic_int_incr(&h->n_counts);
	
    int64_t offset = point - h->start;
	int index = 0;
	if (offset > 0) {
		index = offset / h->bucket_offset;
		if (index > h->num_buckets - 1)
			index = h->num_buckets - 1;
	}

	cf_atomic_int_incr( &h->count[ index ]);
	
}

void linear_histogram_insert_data_point_val( linear_histogram *h, uint64_t point, int64_t val)
{
	cf_atomic_int_add(&h->n_counts, val);

    int64_t offset = point - h->start;
	int index = 0;
	if (offset > 0) {
		index = offset / h->bucket_offset;
		if (index > h->num_buckets - 1)
			index = h->num_buckets - 1;
	}

	cf_atomic_int_add(&h->count[ index ], val);

}

void linear_histogram_get_counts(linear_histogram *h, linear_histogram_counts *hc)
{
	for (int i = 0; i < h->num_buckets; i++) {
		hc->count[i] = h->count[i];
	}
}

uint64_t linear_histogram_get_total(linear_histogram *h)
{
	int64_t total = 0;

	for (int i = 0; i < h->num_buckets; i++) {
		total += cf_atomic_int_get(h->count[i]);
	}

	return (uint64_t)total;
}

// This routine is not thread safe and should be called from a single threaded routine
size_t linear_histogram_get_index_for_pct(linear_histogram *h, size_t pct)
{
	if (h->n_counts == 0)
		return 1;
	int min_limit = (h->n_counts * pct) / 100;
	if (min_limit >= h->n_counts)
		return h->num_buckets;
	int count = 0;
	for (int i = 0; i < h->num_buckets; i++) {
		count += h->count[i];
		if (count >= min_limit)
			return (i+1);
	}
	return h->num_buckets;
}

// Note: not thread safe!
uint64_t linear_histogram_get_offset_for_subtotal(linear_histogram *h, int64_t subtotal)
{
	if (h->n_counts == 0)
		return 0;
	int count = 0;
	for (int i = 0; i < h->num_buckets; i++) {
		count += h->count[i];
		if (count >= subtotal)
			return h->start + ((i+1) * h->bucket_offset);
	}
	return h->start + (h->num_buckets * h->bucket_offset);
}

void linear_histogram_dump( linear_histogram *h )
{
	char printbuf[100];
	int pos = 0; // location to print from
	printbuf[0] = '\0';
	
	cf_info(AS_INFO, "linear histogram dump: %s (%zu total)",h->name, h->n_counts);
	int i, j;
	int k = 0;
	for (j = h->num_buckets - 1; j >= 0; j--) if (h->count[j]) break;
	for (i = 0; i < h->num_buckets; i++) if (h->count[i]) break;
	for (; i<=j;i++) {
		if (h->count[i] > 0) { // print only non zero columns
			int bytes = sprintf((char *) (printbuf + pos), " (%02d: %010zu) ", i, h->count[i]);
			if (bytes <= 0) 
			{
				cf_info(AS_INFO, "linear histogram printing error. Bailing ...");
				return;
			}
			pos += bytes;
		    if (k % 4 == 3){
		    	 cf_info(AS_INFO, "%s", (char *) printbuf);
		    	 pos = 0;
		    	 printbuf[0] = '\0';
		    }
		    k++;
		}
	}
	if (pos > 0) 
	    cf_info(AS_INFO, "%s", (char *) printbuf);
}
