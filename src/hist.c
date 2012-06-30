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

/*
	Histogram structure for saving/calculating Latency/throughput
*/

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
	memset(&h->latency, 0, sizeof(h->latency)); //latency information for the current histogram. -do i really need this ?
	return(h);
}


void histogram_clear(histogram *h)
{
	cf_atomic_int_set(&h->n_counts, 0);

	for (int i = 0; i < N_COUNTS; i++) {
		cf_atomic_int_set(&h->count[i], 0);
	}
}

/*
	Reset transactions and bucket information.	
*/

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

/*
	Specialized histogram dump function : It calculates the latency and saves latency infomation for the 60 seconds, depending on ticker interval
	Averages the Latency for seconds to save 59 minutes of latency information.
	Averages the Latency for minutes to save 24 hours of latency information.
	
*/
void histogram_dump_pct( histogram *h )
{
	char printbuf[100];
	int pos = 0; // location to print from
	printbuf[0] = '\0';
    	struct tm *ptr;
    	time_t ltime;
	float sum=0.0;

    	ltime = time(NULL); /* get current calendar time */
    	ptr = gmtime(&ltime);  // return time in the form of tm structure

	float percentages[N_COUNTS];
	float percentage;	
	
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


/*
	Calculate percentage for each bucket in the histogram, h->n_counts_pct is total number of transactions from the last ticker interval.
	h->count_pct[i] is the number of transactions in each bucket.
	"i" is basically used below for counting buckets of min/secs/hours/percentages.

	
*/

	for (j=N_COUNTS-1 ; j >= 0 ; j-- ) if (h->count_pct[j]) break;
	for (i=0;i<N_COUNTS;i++) if (h->count_pct[i]) break;
	for (; i<=j;i++) {
		if (h->count_pct[i] > 0) { 
			percentages[i]=((float)h->count_pct[i]/(float)h->n_counts_pct)*100.0;
		}
	}

/*
	Calculate latency over 1,2,4,8,16,32,64 
	"k" is used for keeping track of latency over the 1,2..64 ms buckets
	Add percentages for all buckets over k and subtract 100.

*/	

	for (k=0;k<N_PCT;k++) {
		percentage=0.0;
		for (i=0;i<=j;i++) {
			if (k < i ) continue;
			percentage=percentage+percentages[i];	
		} 
		if (percentage > 0.0) h->latency[k]=100.0-percentage;
	
	}


/*
	Save the latency in the histogram struct for the current second when the ticker/hist was triggered, so we can save max 60 seconds of data or based on ticker.
	so if ticker interval is 10 , then there will 6 buckets of seconds data.

*/

	for (k=0;k<N_PCT;k++) {
		h->secs[ptr -> tm_sec][k]=h->latency[k];	
	}

/*
	Average the h->secs data structure, and save it into the minutes data structure. This will always try to average data , 
	so whatever ticker interval is, the data for the last minute will always be accurate (hopefully!).

*/
	
	for (k=0;k<N_PCT;k++) {
		sum=0.0;
		j=0;
		for(i=0;(i<=N_SECS) ;i++) {
			sum += h->secs[i][k];
			if (h->secs[i][k] > 0.0)    j++;	
		}

		if (sum >0.0)  h->mins[ptr->tm_min][k]=sum/j;

	}
	
/*
	Average the h->mins data structure and save it into the hours data structure.
	
*/
	for (k=0;k<N_PCT;k++) {
		sum=0.0;
		for(i=0;(i<N_MINS);i++) {
			sum += h->mins[i][k];
		}

		if (sum > 0.0)  h->hours[ptr -> tm_hour][k]=sum/N_MINS;

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

/*
	Duplicate of histogram_insert_data_point only additional function is to increment n_counts_pct and count_pct[index]
*/

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
