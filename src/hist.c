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


//	Histogram structure for saving/calculating Latency/throughput


histogram * 
histogram_create_pct(char *name)
{
	histogram * h = cf_malloc(sizeof(histogram));
	if (!h)	return(0);
	if (strlen(name) >= sizeof(h->name)-1) { cf_free(h); return(0); }
	strcpy(h->name, name);
	h->n_counts = 0;
	h->n_counts_pct = 0;
	memset(&h->count, 0, sizeof(h->count)); //keeping old behaviour of histogram
	memset(&h->count_pct, 0, sizeof(h->count_pct)); //new counter which is reset everytime ticker is called
	memset(&h->secs, 0, sizeof(h->secs)); //data structure to save max 60 seconds of latency.
	memset(&h->mins, 0, sizeof(h->mins)); //data structure for saving last 60 minutes of latency information
	memset(&h->hours, 0, sizeof(h->hours)); //data structure for saving last 24 hours of latency information
	return(h);
}


void histogram_clear(histogram *h)
{
	cf_atomic_int_set(&h->n_counts, 0);

	for (int i = 0; i < N_COUNTS; i++) {
		cf_atomic_int_set(&h->count[i], 0);
	}
}


//	Reset transactions and bucket information.	


void histogram_clear_pct(histogram *h)
{
	cf_atomic_int_set(&h->n_counts_pct, 0);

	for (int i = 0; i < N_COUNTS; i++) {
		cf_atomic_int_set(&h->count_pct[i], 0);
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


//	Specialized histogram dump function : It calculates the latency and saves latency infomation for the 60 seconds, depending on ticker interval
//	Averages the Latency for seconds to save 59 minutes of latency information.
//	Averages the Latency for minutes to save 24 hours of latency information.
	

void histogram_dump_pct( histogram *h )
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

	struct tm *ptr;
    	time_t ltime;
    	ltime = time(NULL); /* get current calendar time */
    	ptr = gmtime(&ltime);  // return time in the form of tm structure
	static cf_atomic_int cur_count;

	// 1st element [0] is always the total number of transaction for the period.
	cf_atomic_int_set(&h->secs[ptr->tm_sec][0], h->n_counts_pct); 

	for (j=N_COUNTS-1 ; j >= 0 ; j-- ) if (h->count_pct[j]) break;
	for (k=0;k<N_PCT;k++) {
		cf_atomic_int_set(&cur_count,0);

		// calculate the number of transactions over k period
		// where k=1 is trans over 1 ms, k=2 is trans over 2 ms, 
		// k=3 is trans over 4 ms bucket 
		
		for (i = 0; i <= j; i++) {
			if (h->count_pct[i] > 0) {
				if (k < i ) continue; //v. imp  for calc.
				cf_atomic_int_add(&cur_count, h->count_pct[i]);
			}
		}

		// As the value in cur_count is total trans from 0 to 1ms,2ms,4 ms
		// we subtract total transactions from cur_count to get trans over.

		cf_atomic_int_sub(&cur_count, h->n_counts_pct);

		// note: saving the cur_count as positive and saving it at k+1
		//       because, k=0 is reserved for n_counts_pct for the period.

		cf_atomic_int_set(&h->secs[ptr->tm_sec][k+1], -cur_count);

		// As we already have seconds data we can keep adding it to min
		// and hour data structure, so we will always have uptodate stats.

		cf_atomic_int_add(&h->mins[ptr->tm_min][k], h->secs[ptr->tm_sec][k]);
		cf_atomic_int_add(&h->hours[ptr->tm_hour][k], h->secs[ptr->tm_sec][k]);
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


//	Duplicate of histogram_insert_data_point only additional function is to 
//	increment n_counts_pct and count_pct[index]
//	n_counts_pct is total number of transaction from the last nsup period.
//	count_pct[index] is the total number of transaction in each bucket from
//	the last nsup period


void histogram_insert_data_point_pct( histogram *h, uint64_t start)
{
	cf_atomic_int_incr(&h->n_counts);
	cf_atomic_int_incr(&h->n_counts_pct); 
	
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
	cf_atomic_int_incr( &h->count_pct[ index ] );
	
}
#endif // USE_GETCYCLES


void histogram_get_counts(histogram *h, histogram_counts *hc)
{
	for (int i=0;i<N_COUNTS;i++)
		hc->count[i] = h->count[i];
	return;
}

linear_histogram * 
linear_histogram_create(char *name, uint64_t start, uint64_t max_offset)
{
	linear_histogram * h = cf_malloc(sizeof(linear_histogram));
	if (!h)	return(0);
	if (strlen(name) >= sizeof(h->name)-1) { cf_free(h); return(0); }
	strcpy(h->name, name);
	h->n_counts = 0;
	h->start = start;
    h->offset20 = max_offset / 20;
	if (h->offset20 == 0) // avoid divide by zero while inserting data point
		h->offset20 = 1;
	memset(&h->count, 0, sizeof(h->count));
	return(h);
}

void linear_histogram_insert_data_point( linear_histogram *h, uint64_t point)
{
	cf_atomic_int_incr(&h->n_counts);
	
    int64_t offset = point - h->start;
	int index = 0;
	if (offset > 0) {
		index = offset / h->offset20;
		if (index > 19)
			index = 19;
	}

	cf_atomic_int_incr( &h->count[ index ]);
	
}

void linear_histogram_get_counts(linear_histogram *h, linear_histogram_counts *hc)
{
	for (int i=0;i<LINEAR_N_COUNTS;i++)
		hc->count[i] = h->count[i];
	return;
}

// This routine is not thread safe and should be called from a single threaded routine
size_t linear_histogram_get_index_for_pct(linear_histogram *h, size_t pct)
{
	if (h->n_counts == 0)
		return 1;
	int min_limit = (h->n_counts * pct) / 100;
	if (min_limit >= h->n_counts)
		return LINEAR_N_COUNTS;
	int count = 0;
	for (int i=0;i<LINEAR_N_COUNTS;i++) {
		count += h->count[i];
		if (count >= min_limit)
			return (i+1);
	}
	return LINEAR_N_COUNTS;
}

void linear_histogram_dump( linear_histogram *h )
{
	char printbuf[100];
	int pos = 0; // location to print from
	printbuf[0] = '\0';
	
	cf_debug(AS_NSUP, "linear histogram dump: %s (%zu total)",h->name, h->n_counts);
	int i, j;
	int k = 0;
	for (j=LINEAR_N_COUNTS-1 ; j >= 0 ; j-- ) if (h->count[j]) break;
	for (i=0;i<LINEAR_N_COUNTS;i++) if (h->count[i]) break;
	for (; i<=j;i++) {
		if (h->count[i] > 0) { // print only non zero columns
			int bytes = sprintf((char *) (printbuf + pos), " (%02d: %010zu) ", i, h->count[i]);
			if (bytes <= 0) 
			{
				cf_debug(AS_NSUP, "linear histogram printing error. Bailing ...");
				return;
			}
			pos += bytes;
		    if (k % 4 == 3){
		    	 cf_debug(AS_NSUP, "%s", (char *) printbuf);
		    	 pos = 0;
		    	 printbuf[0] = '\0';
		    }
		    k++;
		}
	}
	if (pos > 0) 
	    cf_debug(AS_NSUP, "%s", (char *) printbuf);
}
