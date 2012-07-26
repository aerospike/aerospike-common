/*
 *  Citrusleaf Foundation
 *  include/alloc.h - memory allocation framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#ifdef MEM_COUNT
#include <math.h>       // for exp2().
#include <time.h>       // For timeval_t & time().
#include <sys/param.h>  // For MIN().
#endif

#include "cf.h"

#include <dlfcn.h>

// #define USE_CIRCUS 1

// #define EXTRA_CHECKS 1

void *   (*g_malloc_fn) (size_t s) = 0;
int      (*g_posix_memalign_fn) (void **memptr, size_t alignment, size_t sz);
void     (*g_free_fn) (void *p) = 0;
void *   (*g_calloc_fn) (size_t nmemb, size_t sz) = 0;
void *   (*g_realloc_fn) (void *p, size_t sz) = 0;
char *   (*g_strdup_fn) (const char *s) = 0;
char *   (*g_strndup_fn) (const char *s, size_t n) = 0;


#ifdef MEM_COUNT

struct shash_s *mem_count_shash;
cf_atomic64 mem_count = 0;
cf_atomic64 mem_count_mallocs = 0;
cf_atomic64 mem_count_frees = 0;
cf_atomic64 mem_count_callocs = 0;
cf_atomic64 mem_count_reallocs = 0;
cf_atomic64 mem_count_strdups = 0;
cf_atomic64 mem_count_strndups = 0;
cf_atomic64 mem_count_vallocs = 0;

/*
 * Maximum length of a string representing a location in the program.
 */
#define MAX_LOCATION_LEN  100

/*
 * Type representing a location in the program, a string of the form:  "<Filename>:<LineNumber>".
 */
typedef char location_t[MAX_LOCATION_LEN];

/*
 * Type of allocation operation being performed.
 */
typedef enum alloc_type_enum 
{
	CF_ALLOC_TYPE_CALLOC,
	CF_ALLOC_TYPE_MALLOC,
	CF_ALLOC_TYPE_REALLOC,
	CF_ALLOC_TYPE_VALLOC,
	CF_ALLOC_TYPE_FREE,
	CF_ALLOC_TYPE_STRDUP,
	CF_ALLOC_TYPE_STRNDUP
} alloc_type;

/*
 * Type representing the location in the program and
 *  size in bytes of a memory allocation by "{c,m,re,v}alloc()".
 */
typedef struct alloc_loc_s {
	location_t loc;               // Location of memory allocation in the program.
	alloc_type type;              // Type of the allocation.
	size_t sz;                    // Size in bytes of the allocation.
} alloc_loc_t;

/*
 * Type representing cumulative information about allocations happening at a given location in the program.
 */
typedef struct alloc_info_s {
	size_t net_sz;                // Net allocation in bytes at this program location.
	ssize_t delta_sz;             // Last change in net allocation in bytes at this program location.
	size_t net_alloc_count;       // Net number of allocations at this program location.
	size_t total_alloc_count;     // Total number of allocations at this program location.
	time_t time_last_modified;    // Time of last net allcation change at this program location.
	// XXX -- Including this field here is a total hack, since it's only used for report generation!  ***PSI***  13-Jul-2012
	//         (Wasting the space in the accounting records could be avoided if a different record type was used for reporting.)
	location_t loc;               // Location of memory allocation in the program.
} alloc_info_t;

/*
 * Hash table mapping pointers to the location and size of allocation.
 */
static shash *ptr2loc_shash = NULL;

/*
 * Hash table mapping allocation locations to the allcation info.
 */
static shash *loc2alloc_shash = NULL;

/*
 * Forward references.
 */
static void make_location(location_t *loc, char *file, int line);
static void copy_location(location_t *loc_out, location_t *loc_in);

static uint32_t
location_hash_fn(void *loc)
{
	char *b = (char *) loc;
	uint32_t acc = 0;

	for (int i = 0; i < sizeof(location_t); i++) {
		acc += *b++;
	}

	return acc;
}

/* mem_count_init
 * This function must be called prior to using the memory counting allocation functions. */
int
mem_count_init()
{
	if (SHASH_OK != shash_create(&mem_count_shash, ptr_hash_fn, sizeof(void *), sizeof(size_t *), 100000, SHASH_CR_MT_MANYLOCK | SHASH_CR_UNTRACKED)) {
		cf_crash(CF_ALLOC, CF_PROCESS, "Failed to allocate mem_count_shash");
	}
	
	if (SHASH_OK != shash_create(&ptr2loc_shash, ptr_hash_fn, sizeof(void *), sizeof(alloc_loc_t), 100000, SHASH_CR_MT_MANYLOCK | SHASH_CR_UNTRACKED)) {
		cf_crash(CF_ALLOC, CF_PROCESS, "Failed to allocate ptr2loc_shash");
	}

	if (SHASH_OK != shash_create(&loc2alloc_shash, location_hash_fn, sizeof(location_t), sizeof(alloc_info_t), 10000, SHASH_CR_MT_MANYLOCK | SHASH_CR_UNTRACKED)) {
		cf_crash(CF_ALLOC, CF_PROCESS, "Failed to allocate loc2alloc_shash");
	}

	cf_atomic64_set(&mem_count, 0);
	cf_atomic64_set(&mem_count_mallocs, 0);
	cf_atomic64_set(&mem_count_frees, 0);
	cf_atomic64_set(&mem_count_callocs, 0);
	cf_atomic64_set(&mem_count_reallocs, 0);
	cf_atomic64_set(&mem_count_strdups, 0);
	cf_atomic64_set(&mem_count_strndups, 0);
	cf_atomic64_set(&mem_count_vallocs, 0);

	return(0);
}

/* get_human_readable_memory_size
 * Return human-readable value (in terms of floting point powers of 2 ^ 10) for a given memory size. */
static void
get_human_readable_memory_size(ssize_t sz, double *quantity, char **scale)
{
	size_t asz = labs(sz);

	if (asz >= (1 << 30)) {
		*scale = "GB";
		*quantity = ((double) sz) / exp2(30.0);
	} else if (asz >= (1 << 20)) {
		*scale = "MB";
		*quantity = ((double) sz) / exp2(20.0);
	} else if (asz >= (1 << 10)) {
		*scale = "KB";
		*quantity = ((double) sz) / exp2(10.0);
	} else {
		*scale = "B";
		*quantity = (double) sz;
	}
}

/* mem_count_stats
 * Print the current memory allocation statistics. */
void
mem_count_stats()
{
	cf_info(CF_ALLOC, "Mem Count Stats:");
	cf_info(CF_ALLOC, "=============================================");

	size_t mc = cf_atomic64_get(mem_count);
	double quantity = 0.0;
	char *scale = "B";
	get_human_readable_memory_size(mc, &quantity, &scale);

	size_t mcm = cf_atomic64_get(mem_count_mallocs);
	size_t mcf = cf_atomic64_get(mem_count_frees);
	size_t mcc = cf_atomic64_get(mem_count_callocs);
	size_t mcr = cf_atomic64_get(mem_count_reallocs);
	size_t mcs = cf_atomic64_get(mem_count_strdups);
	size_t mcsn = cf_atomic64_get(mem_count_strndups);
	size_t mcv = cf_atomic64_get(mem_count_vallocs);

	cf_info(CF_ALLOC, "mem_count: %ld (%.3f %s)", mc, quantity, scale);
	cf_info(CF_ALLOC, "=============================================");
	cf_info(CF_ALLOC, "net mallocs: %ld", (mcm + mcc + mcv - mcf));
	cf_info(CF_ALLOC, "=============================================");
	cf_info(CF_ALLOC, "mem_count_mallocs: %ld", mcm);
	cf_info(CF_ALLOC, "mem_count_frees : %ld", mcf); 
	cf_info(CF_ALLOC, "mem_count_callocs: %ld", mcc);
	cf_info(CF_ALLOC, "mem_count_reallocs: %ld", mcr);
	cf_info(CF_ALLOC, "mem_count_strdups: %ld", mcs);
	cf_info(CF_ALLOC, "mem_count_strndups: %ld", mcsn);
	cf_info(CF_ALLOC, "mem_count_vallocs: %ld", mcv);
	cf_info(CF_ALLOC, "=============================================");
}

/* mem_count_alloc_info
 * Lookup the allocation info for a location in the program.
 * (This is the workhorse for the "alloc_info" info command.) */
int
mem_count_alloc_info(char *file, int line, cf_dyn_buf *db)
{
	location_t loc;
	alloc_info_t alloc_info;
	make_location(&loc, file, line);

	if (SHASH_OK == shash_get(loc2alloc_shash, &loc, &alloc_info)) {
		double quantity = 0.0;
		char *scale = "B";

		cf_info(CF_ALLOC, "Allocation @ location \"%s\":", loc);
		get_human_readable_memory_size(alloc_info.net_sz, &quantity, &scale);
		cf_info(CF_ALLOC, "\tnet_sz: %zu (%.3f %s)", alloc_info.net_sz, quantity, scale);
		get_human_readable_memory_size(alloc_info.delta_sz, &quantity, &scale);
		cf_info(CF_ALLOC, "\tdelta_sz: %ld (%.3f %s)", alloc_info.delta_sz, quantity, scale);
		cf_info(CF_ALLOC, "\tnet_alloc_count: %zu", alloc_info.net_alloc_count);
		cf_info(CF_ALLOC, "\ttotal_alloc_count: %zu", alloc_info.total_alloc_count);
		struct tm *mod_time_tm = gmtime(&(alloc_info.time_last_modified));
		char time_str[50];
		strftime(time_str, sizeof(time_str), "%b %d %Y %T %Z", mod_time_tm);
		cf_info(CF_ALLOC, "\ttime_last_modified: %ld (%s)", alloc_info.time_last_modified, time_str);
	} else {
		cf_warning(CF_ALLOC, "Cannot find allocation at location: \"%s\".", loc);
		return -1;
	}

	return 0;
}

/*
 * Type representing the different kinds of report records.
 * (Note:  This is a union so we can share the stack allocation.)
 */
typedef union output_u {
	alloc_loc_t *p2l_output;      // The ptr2loc report.
	alloc_info_t *l2a_output;     // The loc2alloc report.
} output_u_t;

/*
 * Type representing a memory allocation count report.
 */
typedef struct mem_count_report_s {
	sort_field_t sort_field;     // Field to sort on (input.)
	int top_n;                   // Number of top entries to return (input.)
	int num_records;             // Number of records of in the report (output.)
	// NB:  Exactly one of the following two report fields will be stack-allocted by the caller:
	output_u_t u;                // The report itself (output.)
} mem_count_report_t;

/* mem_count_report_p2l_reduce_fn
 * This shash reduce function is used to extract the records for the pointer to program location report.
 */
static int
mem_count_report_p2l_reduce_fn(void *key, void *data, void *udata)
{
	alloc_loc_t *alloc_loc = (alloc_loc_t *) data;
	mem_count_report_t *report = (mem_count_report_t *) udata;
	alloc_loc_t *rec = report->u.p2l_output;

	if (alloc_loc->sz > rec[report->top_n - 1].sz) {
		int i = 0;
		while (i < report->num_records) {
			if (alloc_loc->sz > rec[i].sz) {
				memmove(&(rec[i + 1]), &(rec[i]), (report->num_records - i - 1) * sizeof(alloc_loc_t));
				break;
			}
			i++;
		}
		copy_location(&(rec[i].loc), &(alloc_loc->loc));
		rec[i].sz = alloc_loc->sz;
		if (report->num_records < report->top_n) {
			report->num_records++;
		}
	}

	return 0;
}

/* copy_alloc_info
 * Duplicate the values of one allocation info object to another. */
static void
copy_alloc_info(alloc_info_t *to, alloc_info_t *from)
{
	to->net_sz = from->net_sz;
	to->delta_sz = from->delta_sz;
	to->net_alloc_count = from->net_alloc_count;
	to->total_alloc_count = from->total_alloc_count;
	to->time_last_modified = from->time_last_modified;
}

/* mem_count_report_l2a_reduce_fn
 * This shash reduce function is used to extract the records for the program location to allocation info report.
 */
static int
mem_count_report_l2a_reduce_fn(void *key, void *data, void *udata)
{
	location_t *loc = (location_t *) key;
	alloc_info_t *alloc_info = (alloc_info_t *) data;
	mem_count_report_t *report = (mem_count_report_t *) udata;
	alloc_info_t *rec = report->u.l2a_output;

	switch (report->sort_field) {
	  case CF_ALLOC_SORT_NET_SZ:
		  if (alloc_info->net_sz > rec[report->top_n - 1].net_sz) {
			  int i = 0;
			  while (i < report->num_records) {
				  if (alloc_info->net_sz > rec[i].net_sz) {
					  memmove(&(rec[i + 1]), &(rec[i]), (report->num_records - i - 1) * sizeof(alloc_info_t));
					  break;
				  }
				  i++;
			  }
			  copy_location(&(rec[i].loc), loc);
			  copy_alloc_info(&(rec[i]), alloc_info);
			  if (report->num_records < report->top_n) {
				  report->num_records++;
			  }
		  }
		  break;
	
	  case CF_ALLOC_SORT_TIME_LAST_MODIFIED:
		  if (alloc_info->time_last_modified > rec[report->top_n - 1].time_last_modified) {
			  int i = 0;
			  while (i < report->num_records) {
				  if (alloc_info->time_last_modified > rec[i].time_last_modified) {
					  memmove(&(rec[i + 1]), &(rec[i]), (report->num_records - i - 1) * sizeof(alloc_info_t));
					  break;
				  }
				  i++;
			  }
			  copy_location(&(rec[i].loc), loc);
			  copy_alloc_info(&(rec[i]), alloc_info);
			  if (report->num_records < report->top_n) {
				  report->num_records++;
			  }
		  }
		  break;

	  case CF_ALLOC_SORT_NET_ALLOC_COUNT:
		  if (alloc_info->net_alloc_count > rec[report->top_n - 1].net_alloc_count) {
			  int i = 0;
			  while (i < report->num_records) {
				  if (alloc_info->net_alloc_count > rec[i].net_alloc_count) {
					  memmove(&(rec[i + 1]), &(rec[i]), (report->num_records - i - 1) * sizeof(alloc_info_t));
					  break;
				  }
				  i++;
			  }
			  copy_location(&(rec[i].loc), loc);
			  copy_alloc_info(&(rec[i]), alloc_info);
			  if (report->num_records < report->top_n) {
				  report->num_records++;
			  }
		  }
		  break;

	  case CF_ALLOC_SORT_TOTAL_ALLOC_COUNT:
		  if (alloc_info->total_alloc_count > rec[report->top_n - 1].total_alloc_count) {
			  int i = 0;
			  while (i < report->num_records) {
				  if (alloc_info->total_alloc_count > rec[i].total_alloc_count) {
					  memmove(&(rec[i + 1]), &(rec[i]), (report->num_records - i - 1) * sizeof(alloc_info_t));
					  break;
				  }
				  i++;
			  }
			  copy_location(&(rec[i].loc), loc);
			  copy_alloc_info(&(rec[i]), alloc_info);
			  if (report->num_records < report->top_n) {
				  report->num_records++;
			  }
		  }
		  break;

	  case CF_ALLOC_SORT_DELTA_SZ:
		  if (labs(alloc_info->delta_sz) > labs(rec[report->top_n - 1].delta_sz)) {
			  int i = 0;
			  while (i < report->num_records) {
				  if (labs(alloc_info->delta_sz) > labs(rec[i].delta_sz)) {
					  memmove(&(rec[i + 1]), &(rec[i]), (report->num_records - i - 1) * sizeof(alloc_info_t));
					  break;
				  }
				  i++;
			  }
			  copy_location(&(rec[i].loc), loc);
			  copy_alloc_info(&(rec[i]), alloc_info);
			  if (report->num_records < report->top_n) {
				  report->num_records++;
			  }
		  }
		  break;

	  default:
		  cf_warning(CF_ALLOC, "Unknown mem count report sort field: %d", report->sort_field);
		  return -1;
	}

	return 0;
}

/* mem_count_report
 * Generate and print the memory accounting report selected by sort_field and top_n.
 * (This is the workhorse for the "alloc_info" info command.) */
void
mem_count_report(sort_field_t sort_field, int top_n, cf_dyn_buf *db)
{
	mem_count_report_t report;
	size_t output_u_sz = top_n * MAX(sizeof(alloc_loc_t), sizeof(alloc_info_t));

	cf_debug(CF_ALLOC, "Size of mem count shashes: p2l: %u ; l2a: %u", shash_get_size(ptr2loc_shash), shash_get_size(loc2alloc_shash));

	/* First report: By pointer value. */

	report.sort_field = sort_field;
	report.top_n = top_n;
	if (!(report.u.p2l_output = alloca(output_u_sz))) {
		cf_crash(CF_ALLOC, CF_PROCESS, "failed to alloca(%zu) report.u.p2l_output", output_u_sz);
	}
	memset(report.u.p2l_output, 0, output_u_sz);
	report.num_records = 0;

	shash_reduce(ptr2loc_shash, mem_count_report_p2l_reduce_fn, &report);

	cf_info(CF_ALLOC, "Mem Ptr Size Report:");
	cf_info(CF_ALLOC, "--------------------");
	for (int i = 0; i < MIN(report.num_records, report.top_n); i++) {
		cf_info(CF_ALLOC, "Top %2d:  Location: %-25s sz: %zu", i, report.u.p2l_output[i].loc, report.u.p2l_output[i].sz);
	}
	cf_info(CF_ALLOC, "--------------------");

	/* Second report: By program location. */

	memset(report.u.l2a_output, 0, output_u_sz);
	report.num_records = 0;

	shash_reduce(loc2alloc_shash, mem_count_report_l2a_reduce_fn, &report);

	cf_info(CF_ALLOC, "Mem Loc Count Report: (sorted by %s):", (CF_ALLOC_SORT_NET_SZ == sort_field ? "space" :
																(CF_ALLOC_SORT_DELTA_SZ == sort_field ? "change" :
																 (CF_ALLOC_SORT_NET_ALLOC_COUNT == sort_field ? "net_count" :
																  (CF_ALLOC_SORT_TOTAL_ALLOC_COUNT == sort_field ? "total_count" :
																   (CF_ALLOC_SORT_TIME_LAST_MODIFIED == sort_field ? "time" : "???"))))));
	cf_info(CF_ALLOC, "---------------------");
	for (int i = 0; i < MIN(report.num_records, report.top_n); i++) {
		alloc_info_t *rec = &(report.u.l2a_output[i]);
		cf_info(CF_ALLOC, " Top %2d: Location: %-25s sz: %10zu  dsz: %10ld  na: %6zu  ta: %6zu  tlm: %ld", i, rec->loc, rec->net_sz, rec->delta_sz, rec->net_alloc_count, rec->total_alloc_count, rec->time_last_modified);
	}
	cf_info(CF_ALLOC, "---------------------");
}

/* mem_count_shutdown
 * NB:  This can only be called when no other threads are running.  (Maybe that means never?) */
void
mem_count_shutdown()
{
	shash_destroy(loc2alloc_shash);
	shash_destroy(ptr2loc_shash);
	shash_destroy(mem_count_shash);
}

/* make_location
 * Combine the file and line number into a program location. */
static void
make_location(location_t *loc, char *file, int line)
{
	memset((char *) loc, 0, sizeof(location_t));
	snprintf((char *) loc, sizeof(location_t), "%s:%d", file, line);
}

/* copy_location
 * Copy one location_t object representing a program location to another. */
static void
copy_location(location_t *loc_out, location_t *loc_in)
{
	memset((char *) loc_out, 0, sizeof(location_t));
	memcpy((char *) loc_out, (char *) loc_in, sizeof(location_t));
}

/* update_alloc_info
 * Update function to combine old and new allocation info for a particular location in the program.
 * The old value will be NULL if the key is not present.
 * The updated new value is the output. */
static void
update_alloc_info(void *key, void *value_old, void *value_new, void *udata)
{
	location_t *loc = (location_t *) key;
	alloc_info_t *alloc_info_old = (alloc_info_t *) value_old;
	alloc_info_t *alloc_info_new = (alloc_info_t *) value_new;

	alloc_info_new->net_sz += (alloc_info_old ? alloc_info_old->net_sz : 0);
	alloc_info_new->net_alloc_count += (alloc_info_old ? alloc_info_old->net_alloc_count : 0);
	alloc_info_new->total_alloc_count += (alloc_info_old ? alloc_info_old->total_alloc_count : 0);
	alloc_info_new->time_last_modified = time(NULL);
	// XXX -- Clear out the hack field only used for report generation.  ***PSI***  13-Jul-2012
	make_location(&(alloc_info_new->loc), "(unused)", 0);

	if (0 > alloc_info_new->net_alloc_count) {
		cf_crash(CF_ALLOC, CF_PROCESS, "allocation count for location \"%s\" just went negative!", loc);
	}
}

/* update_alloc_at_location
 * Internal function to change the memory counting for a location in a file. */
static void
update_alloc_at_location(void *p, size_t sz, alloc_type type, char *file, int line)
{
	alloc_loc_t alloc_loc;

	cf_atomic64_add(&mem_count, (CF_ALLOC_TYPE_FREE != type ? sz : - sz));

	location_t *loc = &alloc_loc.loc;
	make_location(loc, file, line);
	alloc_loc.sz = sz;
	alloc_loc.type = type;

	if (CF_ALLOC_TYPE_FREE != type) {
		if (SHASH_OK != shash_put_unique(ptr2loc_shash, &p, &alloc_loc)) {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not put %p into ptr2loc_shash with sz: %zu loc: \"%s\"", p, sz, loc);
		}
	} else {
		if (SHASH_OK != shash_get_and_delete(ptr2loc_shash, &p, &alloc_loc)) {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not get %p from ptr2loc_shash with sz: %zu loc: \"%s\"", p, sz, loc);
		}
	}

	// Need to stack-allocate old and new values to pass into the update function.
	alloc_info_t alloc_info_old;
	alloc_info_t alloc_info_new;

	alloc_info_new.net_sz = alloc_info_new.delta_sz = (CF_ALLOC_TYPE_FREE != type ? sz : -sz);
	alloc_info_new.net_alloc_count = (CF_ALLOC_TYPE_FREE != type ? 1 : -1);
	alloc_info_new.total_alloc_count = 1;

	if (SHASH_OK != shash_update(loc2alloc_shash, loc, &alloc_info_old, &alloc_info_new, update_alloc_info, 0)) {
		cf_crash(CF_ALLOC, CF_PROCESS, "Could not update loc2alloc_shash with sz: %zu loc: \"%s\"", sz, loc);
	}
}

void *
cf_malloc_count(size_t sz, char *file, int line)
{
	void *p = malloc(sz);

	cf_atomic64_incr(&mem_count_mallocs);

	if (p) {
		if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
			update_alloc_at_location(p, sz, CF_ALLOC_TYPE_MALLOC, file, line);
		} else {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
		}
	}

	return(p);
}

void
cf_free_count(void *p, char *file, int line)
{
	/* Apparently freeing 0 is both being done by our code and permitted in the GLIBC implementation. */
	if (!p) {
		cf_info(CF_ALLOC, "[Ignoring cf_free(0)!]");
		return;
	}

	size_t sz = 0;

	cf_atomic64_incr(&mem_count_frees);

	if (SHASH_OK == shash_get_and_delete(mem_count_shash, &p, &sz)) {
		update_alloc_at_location(p, sz, CF_ALLOC_TYPE_FREE, file, line);
		free(p);
	} else {
		cf_crash(CF_ALLOC, CF_PROCESS, "Could not find pointer %p in mem_count_shash", p);
	}
}

void *
cf_calloc_count(size_t nmemb, size_t sz, char *file, int line)
{
	void *p = calloc(nmemb, sz);

	cf_atomic64_incr(&mem_count_callocs);

	if (p) {
		if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
			update_alloc_at_location(p, sz, CF_ALLOC_TYPE_CALLOC, file, line);
		} else {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
		}
	}

	return(p);
}

void *
cf_realloc_count(void *ptr, size_t sz, char *file, int line)
{
	void *p = realloc(ptr, sz);

	cf_atomic64_incr(&mem_count_reallocs);

	if (!ptr) {
		// If ptr is NULL, realloc() is equivalent to malloc().
		if (p) {
			if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
				cf_atomic64_incr(&mem_count_mallocs);
				update_alloc_at_location(p, sz, CF_ALLOC_TYPE_MALLOC, file, line);
			} else {
				cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
			}
		}
	} else if (!sz) { 
		// if sz is NULL, realloc() is equivalent to free().
		if (SHASH_OK == shash_get_and_delete(mem_count_shash, &ptr, &sz)) {
			cf_atomic64_incr(&mem_count_frees);
			update_alloc_at_location(ptr, sz, CF_ALLOC_TYPE_FREE, file, line);
		} else {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not find pointer %p in mem_count_shash", p);
		}
	} else {
		// Otherwise, the old block is freed and a new block of the requested size is allocated.
		if (p) {
			size_t old_sz = 0;

			if (SHASH_OK == shash_get_and_delete(mem_count_shash, &ptr, &old_sz)) {
				update_alloc_at_location(ptr, old_sz, CF_ALLOC_TYPE_FREE, file, line);
				if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
					update_alloc_at_location(p, sz, CF_ALLOC_TYPE_REALLOC, file, line);
				} else {
					cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
				}
			} else {
				cf_crash(CF_ALLOC, CF_PROCESS, "Could not find pointer %p in mem_count_shash", p);
			}
		}
	}

	return(p);
}

void *
cf_strdup_count(const char *s, char *file, int line)
{
	void *p = strdup(s);
	size_t sz = strlen(s) + 1;

	cf_atomic64_incr(&mem_count_strdups);

	if (p) {
		if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
			update_alloc_at_location(p, sz, CF_ALLOC_TYPE_STRDUP, file, line);
		} else {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
		}
	}

	return(p);
}

void *
cf_strndup_count(const char *s, size_t n, char *file, int line)
{
	void *p = strndup(s, n);
	size_t sz = MIN(n, strlen(s)) + 1;

	cf_atomic64_incr(&mem_count_strndups);

	if (p) {
		if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
			update_alloc_at_location(p, sz, CF_ALLOC_TYPE_STRNDUP, file, line);
		} else {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
		}
	}

	return(p);
}

void *
cf_valloc_count(size_t sz, char *file, int line)
{
	void *p = 0;

	cf_atomic64_incr(&mem_count_vallocs);

	if (0 == posix_memalign(&p, 4096, sz)) {
		if (SHASH_OK == shash_put_unique(mem_count_shash, &p, &sz)) {
			update_alloc_at_location(p, sz, CF_ALLOC_TYPE_VALLOC, file, line);
		} else {
			cf_crash(CF_ALLOC, CF_PROCESS, "Could not add ptr: %p sz: %zu to mem_count_shash", p, sz);
		}
		return(p);
	} else {
		cf_crash(CF_ALLOC, CF_PROCESS, "posix_memalign() failed to allocate sz: %zu", sz);
	}

	return(0);
}

#elif defined(MEM_TRACK)

struct shash_s *mem_alloced;
cf_atomic64 mem_alloced_sum;

int
mem_alloced_reduce_fn(void *key, void *data, void *udata) {
	mem_track_alloc *p_mta = (mem_track_alloc *)data;
	cf_info(CF_ALLOC, "%p | %zu (%s:%d)", *(void **)key, p_mta->sz, p_mta->file, p_mta->line);
	return(0);
}

void *
cf_malloc_track(size_t sz, char *file, int line) {
	void *p = malloc(sz);

	mem_track_alloc mta;
	mta.sz = sz;
	strcpy(mta.file, file);
	mta.line = line;
	shash_put(mem_alloced, &p, &mta);
	cf_atomic64_add(&mem_alloced_sum, sz);

	return(p);
}

void
cf_free_track(void *p, char *file, int line) {
	mem_track_alloc mta;
	shash_get_and_delete(mem_alloced, &p, &mta);
	cf_atomic64_sub(&mem_alloced_sum, mta.sz);

	free(p);
}

void *
cf_calloc_track(size_t nmemb, size_t sz, char *file, int line) {
	void *p = calloc(nmemb, sz);

	mem_track_alloc mta;
	mta.sz = sz;
	strcpy(mta.file, file);
	mta.line = line;
	shash_put(mem_alloced, &p, &mta);
	cf_atomic64_add(&mem_alloced_sum, sz);

	return(p);
}

void *
cf_realloc_track(void *ptr, size_t sz, char *file, int line) {
	void *p = realloc(ptr, sz);

	if (p != ptr) {
		mem_track_alloc mta;
		shash_get_and_delete(mem_alloced, &p, &mta);
		int64_t memory_delta = sz - mta.sz;

		mta.sz = sz;
		strcpy(mta.file, file);
		mta.line = line;
		shash_put(mem_alloced, &p, &mta);

		if (memory_delta) {
			cf_atomic64_add(&mem_alloced_sum, memory_delta);
		}
	} else {
		mem_track_alloc *p_mta;
		pthread_mutex_t *mem_alloced_lock;
		shash_get_vlock(mem_alloced, &p, (void **)&p_mta, &mem_alloced_lock);
		int64_t memory_delta = sz - p_mta->sz;

		p_mta->sz = sz;
		strcpy(p_mta->file, file);
		p_mta->line = line;

		pthread_mutex_unlock(mem_alloced_lock);

		if (memory_delta) {
			cf_atomic64_add(&mem_alloced_sum, memory_delta);
		}
	}

	return(p);
}

void *
cf_strdup_track(const char *s, char *file, int line) {
	void *p = strdup(s);

	mem_track_alloc mta;
	mta.sz = strlen(p) + 1;
	strcpy(mta.file, file);
	mta.line = line;
	shash_put(mem_alloced, &p, &mta);
	cf_atomic64_add(&mem_alloced_sum, mta.sz);

	return(p);
}

void *
cf_strndup_track(const char *s, size_t n, char *file, int line) {
	void *p = strndup(s, n);

	mem_track_alloc mta;
	mta.sz = strlen(p) + 1;
	strcpy(mta.file, file);
	mta.line = line;
	shash_put(mem_alloced, &p, &mta);
	cf_atomic64_add(&mem_alloced_sum, mta.sz);

	return(p);
}

void *
cf_valloc_track(size_t sz, char *file, int line) {
	void *p = 0;
	if (0 == posix_memalign( &p, 4096, sz)) {
		mem_track_alloc mta;
		mta.sz = sz;
		strcpy(mta.file, file);
		mta.line = line;
		shash_put(mem_alloced, &p, &mta);
		cf_atomic64_add(&mem_alloced_sum, sz);

		return(p);
	}
	return(0);
}

#endif  // defined(MEM_TRACK)

int
alloc_function_init(char *so_name)
{
	if (so_name) {
//		void *clib_h = dlopen(so_name, RTLD_LAZY | RTLD_LOCAL );
		void *clib_h = dlopen(so_name, RTLD_NOW | RTLD_GLOBAL );
		if (!clib_h) {
			cf_warning(AS_AS, " WARNING: could not initialize memory subsystem, allocator %s not found",so_name);
			fprintf(stderr, " WARNING: could not initialize memory subsystem, allocator %s not found\n",so_name);
			return(-1);
		}
		
		g_malloc_fn = dlsym(clib_h, "malloc");
		g_posix_memalign_fn = dlsym(clib_h, "posix_memalign");
		g_free_fn = dlsym(clib_h, "free");
		g_calloc_fn = dlsym(clib_h, "calloc");
		g_realloc_fn = dlsym(clib_h, "realloc");
		g_strdup_fn = dlsym(clib_h, "strdup");
		g_strndup_fn = dlsym(clib_h, "strndup");
		
		// dlclose(alloc_fn);
	}
	else {
		g_malloc_fn = malloc;
		g_posix_memalign_fn = posix_memalign;
		g_free_fn = free;
		g_calloc_fn = calloc;
		g_realloc_fn = realloc;
		g_strdup_fn = strdup;
		g_strndup_fn = strndup;
	}
	return(0);	
}


#ifdef USE_CIRCUS

#define CIRCUS_SIZE (1024 * 1024)

// default is track all
// int cf_alloc_track_sz = 0;

// track something specific in size
int cf_alloc_track_sz = 40;

#define STATE_FREE 1
#define STATE_ALLOC 2
#define STATE_RESERVE 3

char *state_str[] = {0, "free", "alloc", "reserve" };

typedef struct {
	void *ptr;
	char file[16];
	int  line;	
	int  state;
} suspect;

typedef struct {
	pthread_mutex_t LOCK;
	int		alloc_sz;
	int		idx;
	suspect s[];	
	
} free_ring;


free_ring *g_free_ring;

void
cf_alloc_register_free(void *p, char *file, int line)
{
	pthread_mutex_lock(&g_free_ring->LOCK);
	
	int idx = g_free_ring->idx;
	suspect *s = &g_free_ring->s[idx];
	s->ptr = p;
	memcpy(s->file,file,15);
	s->file[15] = 0;
	s->line = line;
	s->state = STATE_FREE;
	idx++;
	g_free_ring->idx = (idx == CIRCUS_SIZE) ? 0 : idx;
	
//	if (idx == 1024)
//		raise(SIGINT);
	
	pthread_mutex_unlock(&g_free_ring->LOCK);
	
}

void
cf_alloc_register_alloc(void *p, char *file, int line)
{
	pthread_mutex_lock(&g_free_ring->LOCK);
	
	int idx = g_free_ring->idx;
	suspect *s = &g_free_ring->s[idx];
	s->ptr = p;
	memcpy(s->file,file,15);
	s->file[15] = 0;
	s->line = line;
	s->state = STATE_ALLOC;
	idx++;
	g_free_ring->idx = (idx == CIRCUS_SIZE) ? 0 : idx;
	
//	if (idx == 1024)
//		raise(SIGINT);
	
	pthread_mutex_unlock(&g_free_ring->LOCK);
	
}

void
cf_alloc_register_reserve(void *p, char *file, int line)
{
	pthread_mutex_lock(&g_free_ring->LOCK);
	
	int idx = g_free_ring->idx;
	suspect *s = &g_free_ring->s[idx];
	s->ptr = p;
	memcpy(s->file,file,15);
	s->file[15] = 0;
	s->line = line;
	s->state = STATE_RESERVE;

	idx++;
	g_free_ring->idx = (idx == CIRCUS_SIZE) ? 0 : idx;
	
//	if (idx == 1024)
//		raise(SIGINT);
	
	pthread_mutex_unlock(&g_free_ring->LOCK);
	
}


void
cf_alloc_print_history(void *p, char *file, int line)
{
// log the history out to the log file, good if you're about to crash
	pthread_mutex_lock(&g_free_ring->LOCK);

	cf_info(CF_ALLOC, "--------- p %p history (idx %d) ------------",p, g_free_ring->idx);
	cf_info(CF_ALLOC, "--------- 	  %s %d ------------",file,line);
	
	for (int i = g_free_ring->idx - 1;i >= 0; i--) {
		if (g_free_ring->s[i].ptr == p) {
			suspect *s = &g_free_ring->s[i];
			cf_info(CF_ALLOC, "%05d : %s %s %d",
				i, state_str[s->state], s->file, s->line); 
		}
	}
	
	for (int i = g_free_ring->alloc_sz - 1; i >= g_free_ring->idx; i--) {
		if (g_free_ring->s[i].ptr == p) {
			suspect *s = &g_free_ring->s[i];
			cf_info(CF_ALLOC, "%05d : %s %s %d",
				i, state_str[s->state], s->file, s->line); 
		}
	}		
	
	pthread_mutex_unlock(&g_free_ring->LOCK);
	
	
}


void
cf_rc_init(char *clib_path) {

	alloc_function_init(clib_path);
	
	// if we're using the circus, initialize it
	g_free_ring = cf_malloc( sizeof(free_ring) + (CIRCUS_SIZE * sizeof(suspect)) );
		
	pthread_mutex_init(&g_free_ring->LOCK, 0);
	g_free_ring->alloc_sz = CIRCUS_SIZE;
	g_free_ring->idx = 0;
	memset(g_free_ring->s, 0, CIRCUS_SIZE * sizeof(suspect));
	return;

}

#else // NO CIRCUS



void
cf_rc_init(char *clib_path) {
	alloc_function_init(clib_path);
}

#endif

/*
**
**
**
**
**
*/




/* cf_rc_count
 * Get the reservation count for a memory region */
int
cf_rc_count(void *addr)
{
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_ALLOC, "rccount: null address");
		raise(SIGINT);
		return(0);
	}
#endif	

	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));

	return((int) hdr->count );
}

/* notes regarding 'add' and 'decr' vs 'addunless' ---
** 
** A suggestion is to use 'addunless' in both the reserve and release code
** which you will see below. That use pattern causes somewhat better
** behavior in buggy use patterns, and allows asserts in cases where the
** calling code is behaving incorrectly. For example, if attempting to reserve a
** reference count where the count has already dropped to 0 (thus is free), 
** can be signaled with a message - and halted at 0, which is far safer.
** However, please not that using this functionality
** changes the API, as the return from 'addunless' is true or false (1 or 0)
** not the old value. Thus, the return from cf_rc_reserve and release will always
** be 0 or 1, instead of the current reference count number.
**
** As some of the citrusleaf client code currently uses the reference count
** as reserved, this code is using the 'add' and 'subtract'
*/


/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with bytes of value zero */
void *
_cf_rc_alloc(size_t sz, char *file, int line)
{
	uint8_t *addr;
	size_t asz = sizeof(cf_rc_hdr) + sz; // debug for stability - rounds us back to regular alignment on all systems

#ifdef MEM_COUNT
	// Track the calling program location.
	addr = cf_malloc_count(asz, file, line);
#else
	addr = cf_malloc(asz);
#endif

	if (NULL == addr)
		return(NULL);

	cf_rc_hdr *hdr = (cf_rc_hdr *) addr;
	hdr->count = 1;  // doesn't have to be atomic
	hdr->sz = sz;
	byte *base = addr + sizeof(cf_rc_hdr);

#ifdef USE_CIRCUS
	if (cf_alloc_track_sz && (cf_alloc_track_sz == hdr->sz))
		cf_alloc_register_alloc(addr, file, line);
#endif		
	
	return(base);
}


/* cf_rc_free
 * Deallocate a reference-counted memory region */
void
_cf_rc_free(void *addr, char *file, int line)
{
	cf_assert(addr, CF_ALLOC, CF_PROCESS, CF_CRITICAL, "null address");

	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));

#if 0
	if (hdr->count != 0) {
		cf_warning(CF_ALLOC, "rcfree: freeing an object that still has a refcount %p",addr);
#ifdef USE_CIRCUS
		cf_alloc_print_history(addr, file, line);
#endif
		raise(SIGINT);
		return;
	}
#endif
	
#ifdef USE_CIRCUS
	if (cf_alloc_track_sz && (cf_alloc_track_sz == hdr->sz))
		cf_alloc_register_free(addr, file, line);
#endif		
	
#ifdef MEM_COUNT
	cf_free_count((void *)hdr, file, line);
#else
	cf_free((void *)hdr);
#endif

	return;
}


#ifdef MEM_TRACK

/* cf_rc_reserve
 * Get a reservation on a memory region */
int
_cf_rc_reserve(void *addr, char *file, int line)
{
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_ALLOC, "rcreserve: null address");
		return(0);
	}
#endif	
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));

#ifdef EXTRA_CHECKS
	// while not very atomic, does provide an interesting test
	// warning. While it might seem logical to check for 0 as well, the as fabric
	// system uses a perversion which doesn't use the standard semantics and will get
	// tripped up by this check
	if (hdr->count & 0x80000000) {
		cf_warning(CF_ALLOC, "rcreserve: reserving without reference count, addr %p count %d very bad",
			addr,hdr->count);
#ifdef USE_CIRCUS
		cf_alloc_print_history(addr, file, line);
#endif
		raise(SIGINT);
		return(0);
	}
#endif	

#ifdef USE_CIRCUS
	if (cf_alloc_track_sz && (cf_alloc_track_sz == hdr->sz))
		cf_alloc_register_reserve(addr, file, line);
#endif		


/* please see the previous note about add vs addunless
*/
	// smb_mb();
	int i = (int) cf_atomic32_add(&hdr->count, 1);
	// smb_mb();
	return(i);
}


/* _cf_rc_release
 * Release a reservation on a memory region */
int
_cf_rc_release(void *addr, bool autofree, char *file, int line)
{
	int c;
	
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_ALLOC, "rcrelease: null address");
		raise(SIGINT);
		return(-1); // don't tell misbehaving code to free null
	} 
#endif	

	/* Release the reservation; if this reduced the reference count to zero,
	 * then free the block if autofree is set, and return 1.  Otherwise,
	 * return 0
	 * Note that a straight decrement is less safe.  Consider the case where
	 * three threads are trying to operate on a block with reference count 1
	 * (yes this is ILLEGAL USE).  The first two threads try to release: the
	 * first one succeeds and frees the block (refcount 0), the second one
	 * succeeds and decrements the block (refcount -1) but doesn't free it
	 * (because the refcount is nonzero), and the third thread tries to
	 * acquire a reference and succeeds, making the refcount 0 on a block
	 * that has been freed but is also supposedly in use.  What a mess that
	 * would be. Using addunless allows the atomic checking that the reference
	 * count wasn't zero, so would result in multiple frees of the same object,
	 * which is an easier error to find and debug. 
	 * However, please see the earlier note about the API - whether this
	 * function should return 1 or 0 for having a reference count, or can/should
	 * return the actual count of the object */
/*	 
 	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));
    c = cf_atomic32_addunless(rc, 0, -1);
    if (0 == c)
        cf_warning(CF_ALLOC, "rcrelease: double release attempted!");
    else if (autofree && (0 == cf_atomic32_get(*rc)))
        free((void*)rc);
*/        
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	
	c = cf_atomic32_decr(&hdr->count);
#ifdef EXTRA_CHECKS
	if (c & 0xF0000000) {
		cf_warning(CF_ALLOC, "rcrelease: releasing to a negative reference count: %p",addr);
#ifdef USE_CIRCUS
		cf_alloc_print_history(addr, file, line);
#endif
		raise(SIGINT);
		return(-1);
	}
#endif
	if ((0 == c) && autofree) {
#ifdef USE_CIRCUS
		if (cf_alloc_track_sz && (cf_alloc_track_sz == hdr->sz))
			cf_alloc_register_free(addr, file, line);
#endif		

		cf_free((void *)hdr);
	}

	return(c);
}

#else  // !defined(MEM_TRACK)

int
_cf_rc_reserve(void *addr)
{
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	int i = (int) cf_atomic32_add(&hdr->count, 1);
	return(i);
}

int
_cf_rc_release(void *addr) {
	int c;
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	c = cf_atomic32_decr(&hdr->count);
	return(c);
}

int
_cf_rc_releaseandfree(void *addr) {
	int c;
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	c = cf_atomic32_decr(&hdr->count);
	if (0 == c) {
		cf_free((void *)hdr);
	}
	return(c);
}

#endif // defined(MEM_TRACK)
