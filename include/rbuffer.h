/*
 *  Citrusleaf RBUFFER
 *  rbuffer.h - Headers specific to ring buffer
 *
 *  Copyright 2011 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#pragma once 
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>

/* SYNOPSIS
 * Ring Buffer
 * Implements general purpose fixed records size ring buffer wrapper
 * over the file[s]. currently supports upto 4 files
 */

#include <cf.h>


#define RBUFFER_MAX_FILES 				4
#define RBUFFER_MAX_FILENAME			256
#define RBUFFER_SEG_DATASIZE			8000

// Pointers in Ring Buffer
typedef struct cf_rbuffer_pointer_s {
	uint8_t				fidx;			// file index
	uint64_t			seg_id;			// block id 
	uint64_t			rec_id;			// record id
} cf_rbuffer_ptr;

// Data Segment 
typedef struct cf_rbuffer_seg_s {
	char					data[RBUFFER_SEG_DATASIZE];
	int16_t					version;
	uint8_t					num_recs;	// Number of records	
	uint8_t					magic;		// Stamp the block magic in the end
} cf_rbuffer_seg;

// File Common Header
typedef struct cf_rbuffer_chdr_s {
	uint64_t 			magic;		// Sane log file
	cf_signature		signature;	// Matching log files [in case of multiple files]
	uint16_t			seg_size;	// Segment Size
	uint16_t			rec_size;	// Log Record Size
	uint8_t				flag;		// properties
	uint8_t				nfiles;		// Number of files
	cf_rbuffer_ptr		sptr;		// Start
	cf_rbuffer_ptr		rptr;		// Current Read
	cf_rbuffer_ptr		wptr;		// Current Write
	int16_t				version;	// ring buffer version
} cf_rbuffer_chdr;

// File Header
typedef struct cf_rbuffer_hdr_s {
	off_t				fsize;	// Size
	uint8_t				fidx;	// Index [in case of multiple files]
	char 				fname[RBUFFER_MAX_FILENAME];
	cf_rbuffer_ptr		sseg;	// Start  
	cf_rbuffer_ptr		nseg;	// Next Segment [after current file]
	cf_rbuffer_ptr		pseg;	// Prev Segment [before current file]
	uint16_t			unused[11];
} cf_rbuffer_hdr;

// Scan context with File pointer and mutex. A scan context is thread safe.
typedef struct cf_rbuffer_ctx_s {
	cf_rbuffer_ptr		ptr;		
	pthread_mutex_t		lock;
	FILE *				fd[RBUFFER_MAX_FILES];
	cf_rbuffer_seg		buf;
	uint8_t				flag;
	int16_t				version;
} cf_rbuffer_ctx;

// In-memory Ring Buffer Descriptor 
typedef struct cf_rbuffer_s {
	// Headers
	cf_rbuffer_chdr		chdr;
	cf_rbuffer_hdr		hdr[RBUFFER_MAX_FILES];

	// Internal read write and meta FD
	FILE *				rfd[RBUFFER_MAX_FILES];	
	FILE *				wfd[RBUFFER_MAX_FILES];
	FILE *				mfd[RBUFFER_MAX_FILES];
	pthread_mutex_t		mlock;
	
	// Internal Read/Write Contexts
	cf_rbuffer_ctx 		rctx;
	cf_rbuffer_ctx  	wctx;
	uint64_t			batch_pos;
	bool 				strapped;
	
	// Statistics
	cf_atomic64			read_stat;
	cf_atomic64			write_stat;
	cf_atomic64			fwrite_stat;
	cf_atomic64			fwmeta_stat;
	cf_atomic64			fread_stat;
	cf_atomic64			batch_size;
	cf_atomic64			max_slots;
} cf_rbuffer;

// Config 
typedef struct cf_rbuffer_config_s {
	char		*fname[RBUFFER_MAX_FILES];
	off_t		fsize[RBUFFER_MAX_FILES];
	uint8_t 	nfiles;
	uint64_t	rec_size;
	uint64_t	batch_size;
	bool		overwrite;
	bool		persist;
	bool		trace;
} cf_rbuffer_config;

extern cf_rbuffer*		cf_rbuffer_init(cf_rbuffer_config *);
extern bool 			cf_rbuffer_reinit(cf_rbuffer *, cf_rbuffer_config *);
extern void 			cf_rbuffer_trace(cf_rbuffer *, bool);
extern bool 		cf_rbuffer_close(cf_rbuffer *);
extern int 		cf_rbuffer_read(cf_rbuffer *, cf_rbuffer_ctx*, char *, int);
extern int 		cf_rbuffer_write(cf_rbuffer *, char *, int);
extern cf_rbuffer_ctx*	cf_rbuffer_getsetctx(cf_rbuffer *, cf_rbuffer_ctx *, int);
extern int 		cf_rbuffer_seek(cf_rbuffer *, cf_rbuffer_ctx *, int, int);
extern int		cf_rbuffer_persist(cf_rbuffer *);
extern bool 		cf_rbuffer_fflush(cf_rbuffer *);
extern void 		cf_rbuffer_closectx(cf_rbuffer_ctx*);
extern void				cf_rbuffer_log(cf_rbuffer *);
extern uint64_t		cf_rbuffer_outstanding(cf_rbuffer *);
extern void		cf_rbuffer_setnoresume(cf_rbuffer *);
extern void		cf_rbuffer_setfailover(cf_rbuffer *);
extern void		cf_rbuffer_setnofailover(cf_rbuffer *);
extern uint64_t		cf_rbuffer_setstart(cf_rbuffer *, cf_rbuffer_ctx *);

// Test
extern int cf_rbuffer_test();


