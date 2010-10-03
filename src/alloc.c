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

	cf_info(CF_RCALLOC, "--------- p %p history (idx %d) ------------",p, g_free_ring->idx);
	cf_info(CF_RCALLOC, "--------- 	  %s %d ------------",file,line);
	
	for (int i = g_free_ring->idx - 1;i >= 0; i--) {
		if (g_free_ring->s[i].ptr == p) {
			suspect *s = &g_free_ring->s[i];
			cf_info(CF_RCALLOC, "%05d : %s %s %d",
				i, state_str[s->state], s->file, s->line); 
		}
	}
	
	for (int i = g_free_ring->alloc_sz - 1; i >= g_free_ring->idx; i--) {
		if (g_free_ring->s[i].ptr == p) {
			suspect *s = &g_free_ring->s[i];
			cf_info(CF_RCALLOC, "%05d : %s %s %d",
				i, state_str[s->state], s->file, s->line); 
		}
	}		
	
	pthread_mutex_unlock(&g_free_ring->LOCK);
	
	
}


void cf_rc_init(char *clib_path) {

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



void cf_rc_init(char *clib_path) {
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
		cf_warning(CF_RCALLOC, "rccount: null address");
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


/* cf_rc_reserve
 * Get a reservation on a memory region */
int
_cf_rc_reserve(void *addr, char *file, int line)
{
#ifdef EXTRA_CHECKS	
	if (addr == 0) {
		cf_warning(CF_RCALLOC, "rcreserve: null address");
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
		cf_warning(CF_RCALLOC, "rcreserve: reserving without reference count, addr %p count %d very bad",
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
		cf_warning(CF_RCALLOC, "rcrelease: null address");
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
        cf_warning(CF_RCALLOC, "rcrelease: double release attempted!");
    else if (autofree && (0 == cf_atomic32_get(*rc)))
        free((void*)rc);
*/        
	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));
	
	c = cf_atomic32_decr(&hdr->count);
#ifdef EXTRA_CHECKS
	if (c & 0xF0000000) {
		cf_warning(CF_RCALLOC, "rcrelease: releasing to a negative reference count: %p",addr);
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


/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with bytes of value zero */
void *
_cf_rc_alloc(size_t sz, char *file, int line)
{
	uint8_t *addr;
	size_t asz = sizeof(cf_rc_hdr) + sz; // debug for stability - rounds us back to regular alignment on all systems

	addr = cf_malloc(asz);
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
	cf_assert(addr, CF_RCALLOC, CF_PROCESS, CF_CRITICAL, "null address");

	cf_rc_hdr *hdr = (cf_rc_hdr *) ( ((uint8_t *)addr) - sizeof(cf_rc_hdr));

#if 0
	if (hdr->count == 0) {
		cf_warning(CF_RCALLOC, "rcfree: freeing an object that still has a refcount %p",addr);
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
	
	cf_free((void *)hdr);

	return;
}
