/*
 *  Citrusleaf RBUFFER
 *  rbuffer.c - ring buffer wrapper
 *
 *  Implements ring buffer wrapper over file[s]/device[s]
 *
 *  Copyright 2011 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <rbuffer.h>
#include <signal.h>

#define RDES		rbuf_des
#define HDR(i)		(RDES->hdr[(i)])
#define CHDR 		(RDES->chdr)
#define MAX_RECS	(CHDR.seg_size / CHDR.rec_size)

#define RBUFFER_FILE_HEADER_SIZE	512
#define RBUFFER_SEG_HEADER_SIZE		4
#define RBUFFER_FILE_MAGIC		0xd19e56109f11e000L
#define RBUFFER_SEG_MAGIC		0xcf
#define RBUFFER_SEG_SIZE		(RBUFFER_SEG_HEADER_SIZE + CHDR.seg_size)

// Number of Segments Freed when overwrite happens
#define RBUFFER_FLAG_OVERWRITE_CHUNK	5

// Ring Buffer Flags
#define RBUFFER_FLAG_OVERWRITE		0x01
#define RBUFFER_FLAG_PERSIST		0x02
#define RBUFFER_FLAG_TRACE		0x04

// Reason for wrap around
#define RBUFFER_FLAG_OVERWRITE_BUFHEAD 	1
#define RBUFFER_FLAG_OVERWRITE_READ	2

// Flag for read context
#define RBUFFER_CTX_FLAG_NEEDSEEK	0x01
#define RBUFFER_CTX_FLAG_SEARCH		0x02

// Macro
#define RBUFFER_ASSERT(cond, func, ...)	\
do{						\
	if((cond)) {				\
		func(CF_RBUFFER, __VA_ARGS__);  \
	}					\
} while(0);

#define RBTRACE(rb, type, ...)					\
	RBUFFER_ASSERT((rb->chdr.flag & RBUFFER_FLAG_TRACE),	\
				cf_ ##type, __VA_ARGS__);

//#define RBTRACE(rb, type, ...) if (rb->chdr.flag & RBUFFER_FLAG_TRACE) fprintf(stderr, "\n"__VA_ARGS__);


#define RBUFFER_NEXT_SEG(seg_id, f_idx) 	\
	((seg_id + 1) % (HDR(f_idx).fsize / RBUFFER_SEG_SIZE));

#define RBUFFER_PREV_SEG(seg_id, f_idx) 	\
	(((seg_id) + (HDR((f_idx)).fsize / RBUFFER_SEG_SIZE) - 1)  \
                     % (HDR((f_idx)).fsize / RBUFFER_SEG_SIZE))



// Client API To log ring buffer data in xds log. This done when debug is enabled.
int
cf_rbuffer_log(cf_rbuffer *rbuf_des)
{
	RBTRACE(RDES, debug, 
			"Stats [%ld:%ld:%ld:%ld]", RDES->read_stat, RDES->write_stat, RDES->fwrite_stat, RDES->fread_stat);

	RBTRACE(RDES, debug, "Current sptr [%ld:%ld] rptr [%ld:%ld] | rctx [%ld:%ld] | wptr [%ld:%ld] | wctx [%ld:%ld]", 
					CHDR.sptr.seg_id, CHDR.sptr.rec_id,
					CHDR.rptr.seg_id, CHDR.rptr.rec_id,
					RDES->rctx.ptr.seg_id, RDES->rctx.ptr.rec_id,
					CHDR.wptr.seg_id, CHDR.wptr.rec_id,
					RDES->wctx.ptr.seg_id, RDES->wctx.ptr.rec_id);
	return 0;
}

// Client API to setup context in ring buffer for no resume case
void 
cf_rbuffer_setnoresume(cf_rbuffer *rbuf_des)
{
	// Reset the read and start pointer to write pointer.
	CHDR.sptr.fidx = RDES->rctx.ptr.fidx = RDES->wctx.ptr.fidx;
	CHDR.sptr.seg_id = RDES->rctx.ptr.seg_id = RDES->wctx.ptr.seg_id;

	// We set up rec_id to 0 we may loose write on the current segment once
	// noresume is set in ring buffer
	CHDR.sptr.rec_id = RDES->rctx.ptr.rec_id = RDES->wctx.ptr.rec_id = 0;
}

bool  cf__rbuffer_fwrite(cf_rbuffer *, cf_rbuffer_ctx *);

// Client API to persist read/write pointer 
//
// Parameter:
//		rbuf_des:	Ring Buffer Descriptor
//
// Caller:
//		XDS
//		Internally read/write calls it periodically to persist data on disk
//
//	Return:
//		0: On success
//		-1: On failure
//
// Synchronization:
//		Caller needs to have read or write context lock.
//		Acquires the meta lock.
int 
cf_rbuffer_persist(cf_rbuffer *rbuf_des)
{
	int i;

	
	RBTRACE(RDES, debug, 
			"Stats [%ld:%ld:%ld:%ld]", RDES->read_stat, RDES->write_stat, RDES->fwrite_stat, RDES->fread_stat);

	pthread_mutex_lock(&RDES->mlock);
	memcpy (&CHDR.rptr, &RDES->rctx.ptr, sizeof(cf_rbuffer_ptr));
	memcpy (&CHDR.wptr, &RDES->wctx.ptr, sizeof(cf_rbuffer_ptr));

	// Only write header if the log is persistent
	if ( CHDR.flag & RBUFFER_FLAG_PERSIST ) 
	{
		for (i = 0; i < CHDR.nfiles; i++) 
		{
			rewind(RDES->mfd[i]);
			if (1 != fwrite(&CHDR, sizeof(cf_rbuffer_chdr), 1, RDES->mfd[i])) 
			{ 
				cf_warning(CF_RBUFFER, "Ring buffer Common Header Persist failed");
				goto err;
			}
			if (1 != fwrite(&HDR(i), sizeof(cf_rbuffer_hdr), 1, RDES->mfd[i])) 
			{ 
				cf_warning(CF_RBUFFER, "Ring buffer File Header Persist failed");
				goto err; 
			}
			fflush(RDES->mfd[i]);
			cf_atomic64_add(&RDES->fwmeta_stat , 2);
		}			
	}

	pthread_mutex_unlock(&RDES->mlock);
	return 0;
err:
	pthread_mutex_unlock(&RDES->mlock);
	return -1;
}

// Internal function to perform SANITY checks
bool
cf__rbuffer_sanity(cf_rbuffer * rbuf_des)
{
	// If only 1 file configured
	if (CHDR.nfiles == 1)
	{
		// sseg == nseg
		if ((HDR(0).nseg.fidx != HDR(0).sseg.fidx) 
			|| (HDR(0).nseg.seg_id != HDR(0).sseg.seg_id)
			|| (HDR(0).nseg.rec_id != HDR(0).sseg.rec_id))	
		{
			RBTRACE(RDES, warning, "Ring Buffer Sanity Check - 2: Failed");
			return false;
		}

		// sseg == pseg
		if ((HDR(0).pseg.fidx != HDR(0).sseg.fidx) 
		        || (HDR(0).pseg.seg_id != HDR(0).fsize / RBUFFER_SEG_SIZE)
       			|| (HDR(0).pseg.rec_id != 0))  
		{
			RBTRACE(RDES, warning, "Ring Buffer Sanity Check - 3: Failed");
			return false;
		}
	}
	else
	{

		// if nfile > 1

		// sptr == sseg[sptr.fidx]	
		if ((CHDR.sptr.fidx != HDR(CHDR.sptr.fidx).sseg.fidx) 
			|| (CHDR.sptr.rec_id != HDR(CHDR.sptr.fidx).sseg.rec_id))	
		{
			RBTRACE(RDES, warning, "Ring Buffer Sanity Check - 4: Failed");
		
			RBTRACE(RDES, warning, "%d!=%d || %ld != %ld || %ld != %ld",
					CHDR.sptr.fidx, HDR(CHDR.sptr.fidx).sseg.fidx, CHDR.sptr.seg_id, HDR(CHDR.sptr.fidx).sseg.seg_id,
					CHDR.sptr.rec_id,  HDR(CHDR.sptr.fidx).sseg.rec_id);
			return false;
		}

		// nseg[i] == sseg[i+1]
		int i = 0;
		while (i < CHDR.nfiles)
		{
			int next = (i + 1) % CHDR.nfiles;
			if ((HDR(i).fidx != HDR(i).sseg.fidx) 
				|| (HDR(i).nseg.fidx != HDR(next).sseg.fidx))
			{
				RBTRACE(RDES, warning, "Ring Buffer Sanity Check - 5: Failed");
				return false;
			}
			i++;
		} 
	}	
	return true;
}

// Internal function to perform segment read seek on the ring buffer
// 
// Parameter: 
//		rbuf_des	: Ring Buffer Descriptor
//		ctx			: Read Context
//		num_seg		: number of segment to seek
// 		forware		: TRUE if forward seek
//					  FALSE otherwise
// Returns:
//		count		: number of segments it seek
// 
// Synchronization:
//		Caller hold read context lock
//		Acquires meta lock.
int
cf__rbuffer_rseek(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx, int num_seg, bool forward)
{
	int count = 0;
	uint64_t	segid;
	uint64_t	seg_id;
	int 		fidx;

	fidx = ctx->ptr.fidx;
	segid = seg_id = ctx->ptr.seg_id; 

	pthread_mutex_lock(&RDES->mlock);
	if ( forward )
	{
		// At write cannot seek forward
		if ((fidx == RDES->wctx.ptr.fidx)		
			&& (segid == RDES->wctx.ptr.seg_id)) 
		{
			goto rseek_err; 
		}

		while(count < num_seg) 
		{
			segid = RBUFFER_NEXT_SEG(segid, fidx);

			// Hit the start of the file 
			if (segid == HDR(ctx->ptr.fidx).sseg.seg_id) 
			{
				segid = HDR(ctx->ptr.fidx).nseg.seg_id;
				fidx = HDR(ctx->ptr.fidx).nseg.fidx;
			}

			// Hit write
			if ((fidx == RDES->wctx.ptr.fidx)		
				&& (segid == RDES->wctx.ptr.seg_id))
			{
				seg_id = segid;
				count++;
				break;
			}

			// Hit the buffer header
			if ((fidx == CHDR.sptr.fidx) 
				&& (segid == CHDR.sptr.seg_id)) 
			{
				goto rseek_err;
			}	
			seg_id = segid;
			count++;	
		}
	}
	else 
	{
		// At start cannot seek backwards
		if ((fidx == RDES->chdr.sptr.fidx)		
			&& (segid == RDES->chdr.sptr.seg_id)) 
		{
			goto rseek_err;
		}

		while(count < num_seg) 
		{
			// Hit the start of the file; goto pseg 
			if (segid == HDR(fidx).sseg.seg_id) 
			{
				segid = HDR(fidx).pseg.seg_id;
				fidx = HDR(fidx).pseg.fidx;
			}
			else
			{
				segid = RBUFFER_PREV_SEG(segid, fidx);
			}

			// Hit the buffer header
			if ((fidx == CHDR.sptr.fidx) 
				&& (segid == CHDR.sptr.seg_id)) 
			{
				if ((count + 1) == num_seg) 
				{
					seg_id = segid;
					count++;
					break;
				}
				goto rseek_err;
			}	

			// Hit write
			if ((fidx == RDES->wctx.ptr.fidx)		
				&& (segid == RDES->wctx.ptr.seg_id))
			{
				// is a bug; should hit buffer head first
				goto rseek_err;
			}
			
			seg_id = segid;
			count++;	
		}
	}

	if (count != num_seg) 
	{
		goto rseek_err;
	}
	RBTRACE(RDES, debug, "Read seek from [%d:%ld] to [%d:%ld] sptr [%d:%ld] wctx [%d:%ld]", 
					ctx->ptr.fidx, ctx->ptr.seg_id, fidx, seg_id,
					CHDR.sptr.fidx, CHDR.sptr.seg_id, RDES->wctx.ptr.fidx, RDES->wctx.ptr.seg_id);
	ctx->ptr.fidx = fidx;
	ctx->ptr.seg_id = seg_id; 
	ctx->ptr.rec_id	= 0;
	cf__rbuffer_sanity(rbuf_des);

	if ((seg_id == 0) && (fidx == 0))
	{
		//bump the version if you reach the start
		ctx->version = CHDR.version;
	}
	pthread_mutex_unlock(&RDES->mlock);
	return ( count );
rseek_err:
	// In case of error do not seek
	cf__rbuffer_sanity(rbuf_des);
	pthread_mutex_unlock(&RDES->mlock);
	return (0);
}



// Internal function to perform start seek in case we run out of space
// in the ring buffer and hit buffer start. General Strategy is
// 	-	Shift Buffer Start by RBUFFER_FLAG_OVERWRITE_CHUNK number of segments
//		in case overwrite read pointer also if RBUFFER_FLAG_OVERWRITE is set.
//	-   Update the read pointer to start if read pointer is stale
//	-   Update the File Start 
//
// Parameter: 
//		rbuf_des	: Ring Buffer Descriptor
//		move 		: Overwrite mode
// 
// Return:
//		TRUE if seek suceed 
//		False otherwise
//
// Synchronization:
//		Caller has a write context mutex
//		Acquires read context mutex to block read when read/buffer
//			start and file start pointer are updated. 
//
bool
cf__rbuffer_startseek(cf_rbuffer *rbuf_des, int mode) 
{
	int num_seg = 0;
	uint64_t	segid;
	uint64_t	fidx;
	
	// lock read as well
	if (mode == RBUFFER_FLAG_OVERWRITE_READ) 
	{
		if (!(CHDR.flag & RBUFFER_FLAG_OVERWRITE)) 
		{
			return false;
		}
	}
	
	segid = CHDR.sptr.seg_id;
	fidx = CHDR.sptr.fidx;
		
	cf__rbuffer_sanity(rbuf_des);

	while (num_seg < RBUFFER_FLAG_OVERWRITE_CHUNK)
	{
		segid = RBUFFER_NEXT_SEG(segid, fidx);

		// Hit the start of the file. 
		if (segid == HDR(fidx).sseg.seg_id)
		{
			segid = HDR(fidx).nseg.seg_id;
			fidx = HDR(fidx).nseg.fidx;
		}

		// Hit read
		if ((fidx == RDES->rctx.ptr.fidx)		
			&& (segid == RDES->rctx.ptr.seg_id)) 
		{
			if (!(CHDR.flag & RBUFFER_FLAG_OVERWRITE)) 
			{
				cf__rbuffer_sanity(rbuf_des);
				return false;
			}
		}

		// Hit the rbuffer header; is a bug should not 
		// hit self
		if ((fidx == CHDR.sptr.fidx)
			&& (segid == CHDR.sptr.seg_id)) 
		{
			cf__rbuffer_sanity(rbuf_des);
			return false;
		}
		num_seg++;
	}

	RBTRACE(RDES, debug, "Moved [sptr::rptr] from [%d:%ld::%d:%ld] to ", 
					CHDR.sptr.fidx, CHDR.sptr.seg_id, CHDR.rptr.fidx, 
					CHDR.rptr.seg_id);

	CHDR.sptr.seg_id = segid;
	CHDR.sptr.fidx = fidx;

	if (mode == RBUFFER_FLAG_OVERWRITE_READ) 
	{
		CHDR.rptr.fidx = fidx; 
		CHDR.rptr.seg_id = segid;
		RDES->rctx.ptr.fidx = fidx;
		RDES->rctx.ptr.seg_id = segid;
	}

	cf_rbuffer_persist(rbuf_des);
	RBTRACE(RDES, debug, "%d:%ld rptr [%d:%ld] wctx [%d:%ld] rctx [%d:%ld] mode:%d ", 
					CHDR.sptr.fidx, CHDR.sptr.seg_id, 
					CHDR.rptr.fidx, CHDR.rptr.seg_id, 
					RDES->wctx.ptr.fidx, RDES->wctx.ptr.seg_id, 
					RDES->rctx.ptr.fidx, RDES->rctx.ptr.seg_id, 
					mode);

	cf__rbuffer_sanity(rbuf_des);
	return true;
}

// Internal function to perform write seek on the ring buffer
// 
// Parameter: 
//		rbuf_des	: Ring Buffer Descriptor
//		ctx			: Write Context
// 
// Return:
//		TRUE if seek suceed 
//		False otherwise
//
// Synchronization:
//		Caller has a write context lock. 
//		Acquires meta lock
//
bool
cf__rbuffer_wseek(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx)
{
	uint64_t	segid;
	uint64_t	fidx;
	int			check = 0;

	pthread_mutex_lock(&RDES->mlock);
	do 
	{
		segid = ctx->ptr.seg_id;
		fidx = ctx->ptr.fidx;	
		segid = RBUFFER_NEXT_SEG(segid, fidx);
		
		// Hit the start of the file. skip this check if we just
		// jumped to the start of the file while seeking 
		if (segid == HDR(fidx).sseg.seg_id)
		{
			// Move to nseg perform the checks again
			segid = HDR(fidx).nseg.seg_id;
			fidx = HDR(fidx).nseg.fidx;
		}

		// Hit read 
		if ((fidx == RDES->rctx.ptr.fidx)		
			&& (segid == RDES->rctx.ptr.seg_id)) 
		{
			if (!cf__rbuffer_startseek(RDES, RBUFFER_FLAG_OVERWRITE_READ))
			{
				pthread_mutex_unlock(&RDES->mlock);
				return false;
			}
			continue;
		}

		// Hit the rbuffer header
		if ((fidx == CHDR.sptr.fidx)
			&& (segid == CHDR.sptr.seg_id)) 
		{
			if (!cf__rbuffer_startseek(RDES, RBUFFER_FLAG_OVERWRITE_BUFHEAD))
			{
				pthread_mutex_unlock(&RDES->mlock);
				return false;
			}
			continue;
		}
		// Somehow in single check it fails for some cases ... do this
		// check twice
		check++;
	} while (check < 1);

	RBTRACE(RDES, debug, "Write seek from [%d:%ld]", ctx->ptr.fidx, ctx->ptr.seg_id);
	ctx->ptr.fidx = fidx;
	ctx->ptr.seg_id = segid; 
	ctx->ptr.rec_id	= 0;
	RBTRACE(RDES, debug, " to %d:%ld @ sptr [%d:%ld] rctx [%d:%ld:%ld]", 
					ctx->ptr.fidx, ctx->ptr.seg_id, 
					CHDR.sptr.fidx, CHDR.sptr.seg_id, 
					RDES->rctx.ptr.fidx, RDES->rctx.ptr.seg_id, 
					RDES->rctx.ptr.rec_id);
	cf__rbuffer_sanity(rbuf_des);

	if ((fidx == 0) && (segid == 0))
	{
		CHDR.version++;
		RBTRACE(RDES, debug, "Increment version %ld %ld to %d",segid, HDR(fidx).sseg.seg_id,CHDR.version);
	}

	pthread_mutex_unlock(&RDES->mlock);
	return true;
}

// Internal API to check and perform seek on the file
//
// Parameter:
//		rbuf_des: Ring Buffer Descriptor 
// 		ctx		: context
//
//	Synchronization:
//		Caller holds the context mutex
//
int
cf__rbuffer_fseek(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx)
{
	uint64_t to_offset = RBUFFER_FILE_HEADER_SIZE +
							(ctx->ptr.seg_id * RBUFFER_SEG_SIZE);
	uint64_t cur_offset = ftell(ctx->fd[ctx->ptr.fidx]);

	// Because of the partial writes to the disk caused by the batch size
	// the offset for the write pointer may not match.
	if (ctx == &RDES->rctx)
	{
		if (cur_offset != to_offset)
			RBTRACE(RDES, debug, "Offset did not match cur=%d, to=%d", cur_offset, to_offset);
	}
	if (to_offset != cur_offset)
	{
		fseek(ctx->fd[ctx->ptr.fidx], to_offset, SEEK_SET);
	}
	return to_offset;
}

// Internal API to flush the write data to the disk up to write pointer
// 
// Parameter:
//		rbuf_des	: Ring buffer descriptor
//
// Synchronization:
//		Caller holds the write context mutex
//		Acquires meta lock
bool
cf_rbuffer_fflush(cf_rbuffer *rbuf_des)
{
	int offset;
	cf_rbuffer_ctx *ctx = &RDES->wctx;
	// Nothing to flush
	if (ctx->ptr.rec_id == 0)
		return true;
	
	pthread_mutex_lock(&RDES->mlock);
	ctx->buf.num_recs 	= ctx->ptr.rec_id; 
	ctx->buf.version	= CHDR.version;
	ctx->buf.magic 		= RBUFFER_SEG_MAGIC;

	offset = cf__rbuffer_fseek(rbuf_des, ctx);
	if (1 != fwrite(&ctx->buf, RBUFFER_SEG_SIZE, 1, ctx->fd[ctx->ptr.fidx])) 
	{
		pthread_mutex_unlock(&RDES->mlock);
		return false;		
	}
	fflush(ctx->fd[ctx->ptr.fidx]);
	
	RBTRACE(RDES, debug, "Write [file:%d offset:%d seg_id:%ld rec_id:%d size:%d:version:%d] %d", 
						ctx->ptr.fidx, offset, ctx->ptr.seg_id, ctx->buf.num_recs, RBUFFER_SEG_SIZE,
						ctx->buf.version, ctx->buf.magic);

	pthread_mutex_unlock(&RDES->mlock);
	// Get write buffer ready for next write
	if (MAX_RECS == ctx->buf.num_recs)
	{
		memset(ctx->buf.data, 0, CHDR.seg_size);
	}

	cf_atomic64_add(&RDES->fwrite_stat, 1);
	RDES->batch_pos = 0;

	return true;
}

// Internal API to seek and write of the segment on to the file
// 
// Parameter:
//		rbuf_des	: Ring buffer descriptor
//		ctx			: write context 
//
// Synchronization:
//		Caller holds the write context mutex
//		Acquires meta lock
bool
cf__rbuffer_fwrite(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx)
{
	int offset;
	pthread_mutex_lock(&RDES->mlock);
	ctx->buf.num_recs 	= ctx->ptr.rec_id; 	
	ctx->buf.version	= CHDR.version;
	ctx->buf.magic 		= RBUFFER_SEG_MAGIC;
#if 0
	// if buffer is full it is ready for read stamp magic
	if (MAX_RECS == ctx->buf.num_recs)
	{
		ctx->buf.magic = RBUFFER_SEG_MAGIC;
	}
#endif

	offset = cf__rbuffer_fseek(rbuf_des, ctx);
	if (1 != fwrite(&ctx->buf, RBUFFER_SEG_SIZE, 1, ctx->fd[ctx->ptr.fidx])) 
	{
		pthread_mutex_unlock(&RDES->mlock);
		return false;		
	}
		
	fflush(ctx->fd[ctx->ptr.fidx]);
	
	RBTRACE(RDES, debug, "Write [file:%d offset:%d seg_id:%ld max_recs:%d, num_recs:%d, rec_id:%ld size:%d:version:%d] %d", 
						ctx->ptr.fidx, offset, ctx->ptr.seg_id, MAX_RECS, ctx->buf.num_recs, ctx->ptr.rec_id, RBUFFER_SEG_SIZE,
						ctx->buf.version, ctx->buf.magic);

	// Get write buffer ready for next write
	if (MAX_RECS == ctx->buf.num_recs)
	{
		memset(ctx->buf.data, 0, CHDR.seg_size);
		ctx->buf.version = 0;
		ctx->buf.num_recs = 0;
		ctx->buf.magic = 0;
	}

	cf_atomic64_add(&RDES->fwrite_stat, 1);
	RDES->batch_pos = 0;

	pthread_mutex_unlock(&RDES->mlock);
	return true;
}

// Internal API to seek and read the content from file in to the
// context buffer.
// 
// Parameter:
//		rbuf_des	: Ring buffer descriptor
//		ctx			: write context 
//
//
// Returns:
//		0 			: in case of failure
//		1			: in case of success
//		-1			: in case version mismatch
//
// Synchronization:
//		Caller holds the read context mutex
int 
cf__rbuffer_fread(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx)
{
	int offset;
	int ret;

	pthread_mutex_lock(&RDES->mlock);
	// Get read buffer ready for next read 
	memset(ctx->buf.data, 0, CHDR.seg_size);
	ctx->buf.num_recs = 0;	
	ctx->buf.magic = 0;	
	ctx->buf.version = 0;

	// seek on the file  
	offset = cf__rbuffer_fseek(rbuf_des, ctx);
	ret = fread(&ctx->buf, RBUFFER_SEG_SIZE, 1, ctx->fd[ctx->ptr.fidx]);
	if (ret != 1)
	{
		pthread_mutex_unlock(&RDES->mlock);
		RBTRACE(RDES, warning, "Fread failed with error %d %d", ferror(ctx->fd[ctx->ptr.fidx]), feof(ctx->fd[ctx->ptr.fidx]));
		return 0;
	}

	if (ctx->buf.magic != RBUFFER_SEG_MAGIC)
	{
		pthread_mutex_unlock(&RDES->mlock);
		RBTRACE(RDES, debug, "Read Buffer with Bad Magic");
		return 0;
	}
	
	// Return -1 if the current version is greater than the version read
	// from the disk.
	if (ctx->version != ctx->buf.version)
	{
		if (ctx->version > ctx->buf.version)
		{
			pthread_mutex_unlock(&RDES->mlock);
			RBTRACE(RDES, debug, "Read context at the invalid version");
			return -1;
		}
		else
		{
			ctx->version = ctx->buf.version;
		}
	}

	RBTRACE(RDES, detail, "Read [file:%d offset:%d seg_id:%ld max_recs:%d num_recs:%d rec_id:%ld size:%d] %d",
					ctx->ptr.fidx, offset, ctx->ptr.seg_id, MAX_RECS, ctx->buf.num_recs, ctx->ptr.rec_id, RBUFFER_SEG_SIZE, ctx->buf.magic);

	cf_atomic64_add(&RDES->fread_stat, 1);
	pthread_mutex_unlock(&RDES->mlock);
	return 1;
}


// Function to Setup inmemory structure based on passed in file headers 
// Internal function to setup the ring buffer descriptor structure.
// * Fixes the links between files
// * Sets up all the pointers
// * Setup read and write context
// 
// Parameter:
//		rbuf_des	: Ring Buffer Descriptor
//		persist		: TRUE if log is persistent
//					  FALSE otherwise.
//
// Synchronization:
//		None only called at the boot time 
bool 
cf__rbuffer_setup(cf_rbuffer *rbuf_des, cf_rbuffer_config *rcfg)
{
	int 	i;
	bool	numfilechanged = false;
	bool 	isbootstrap = false;
	pthread_mutexattr_t attr;
	int 	ret;

	// Order the files in proper order set up fidx
	if (CHDR.nfiles != rcfg->nfiles) 
	{
		cf_info(CF_RBUFFER, "Number of log file changed from %d to %d", CHDR.nfiles, rcfg->nfiles);
		if (CHDR.nfiles == 0)
		{
			isbootstrap = true;
		}
		CHDR.nfiles = rcfg->nfiles;
		numfilechanged = true;
	}
	
	// Setup the linkages sseg / pseg / nseg.
	for (i = 0; i < CHDR.nfiles; i++)
	{
		int next = ((i + 1) % CHDR.nfiles);
		int prev = ((i + CHDR.nfiles - 1) % CHDR.nfiles);
		sprintf(HDR(i).fname,"%s", rcfg->fname[i]);
		HDR(i).sseg.fidx = i;
		HDR(i).sseg.seg_id = 0;
		HDR(i).sseg.rec_id = 0;
		HDR(i).nseg.fidx = next;	
		HDR(i).nseg.seg_id = HDR(next).sseg.seg_id;
		HDR(i).pseg.fidx = prev;	
		HDR(i).pseg.seg_id = HDR(prev).fsize / RBUFFER_SEG_SIZE;
	}

	pthread_mutexattr_init(&attr);	
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	if (pthread_mutex_init(&RDES->wctx.lock, &attr) != 0)
	{
		pthread_mutexattr_destroy(&attr);
		cf_warning(CF_RBUFFER, "Ring buffer Could not init mutex.. aborting \n");
		return false;
	}
		
	if (pthread_mutex_init(&RDES->rctx.lock, &attr) != 0)
	{
		pthread_mutex_destroy(&RDES->wctx.lock);
		pthread_mutexattr_destroy(&attr);
		cf_warning(CF_RBUFFER, "Ring buffer Could not init mutex.. aborting \n");
		return false;
	}
	
	if (pthread_mutex_init(&RDES->mlock, &attr) != 0)
	{
		pthread_mutex_destroy(&RDES->wctx.lock);
		pthread_mutex_destroy(&RDES->rctx.lock);
		pthread_mutexattr_destroy(&attr);
		cf_warning(CF_RBUFFER, "Ring buffer Could not init mutex.. aborting \n");
		return false;
	}


	// Init read and write context and populate values
	RDES->wctx.ptr.seg_id = CHDR.wptr.seg_id;
	RDES->wctx.ptr.rec_id = CHDR.wptr.rec_id;
	RDES->wctx.ptr.fidx = CHDR.wptr.fidx;
	for (i = 0; i < RBUFFER_MAX_FILES; i++)
	{
		RDES->wctx.fd[i] = RDES->wfd[i];
	}
	RDES->wctx.flag = 0;
	RDES->wctx.version = 1;

	RDES->rctx.ptr.seg_id = CHDR.rptr.seg_id;
	RDES->rctx.ptr.rec_id = CHDR.rptr.rec_id;
	RDES->rctx.ptr.fidx = CHDR.rptr.fidx;
	for (i = 0; i < RBUFFER_MAX_FILES; i++)
	{
		RDES->rctx.fd[i] = RDES->rfd[i];
	}
	RDES->rctx.flag = 0;
	RDES->rctx.version = 1;
	
	memset(&RDES->wctx.buf, 0, sizeof(cf_rbuffer_seg));
	ret = cf__rbuffer_fread(RDES, &RDES->wctx);

	if (ret != 1)
	{
		RBTRACE(RDES, debug, "Setting up write buffer failed");
	}
	memset(&RDES->rctx.buf, 0, sizeof(cf_rbuffer_seg));

	RDES->read_stat = 0;
	RDES->write_stat = 0;
	RDES->fwrite_stat = 0;
	RDES->fwmeta_stat = 0;
	RDES->fread_stat = 0;
	RDES->max_slots = (HDR(0).fsize / RBUFFER_SEG_SIZE) * MAX_RECS;
	RDES->batch_size = rcfg->batch_size;
	RDES->batch_pos = 0;

	// Make sure of the earlier number of file is changed file
	// starts where start pointer points is updated properly
	// DO NOT DELETE THIS COMMENT IT HAS MEANING
	CHDR.flag = 0;
	if ( rcfg->persist ) 
		CHDR.flag |= RBUFFER_FLAG_PERSIST;
		
	if ( rcfg->overwrite ) 
		CHDR.flag |= RBUFFER_FLAG_OVERWRITE;

	if ( rcfg->trace ) 
		CHDR.flag |= RBUFFER_FLAG_TRACE;

	pthread_mutexattr_destroy(&attr);
	return true;
}

// Function to bootstrap the file header with the initial value
// in case it is newly added.
// 
// Parameter:
//		rbuf_des	: Ring Buffer Descriptor
//		rcfg		: Ring buffer configuration
//		fidx		: -1 if boot strap all files
//					  other wise fidx for the file which needs
//					  to be boot strapped
//
// Synchronization:
//		None only called at the time of initialization
bool
cf__rbuffer_bootstrap(cf_rbuffer *rbuf_des, cf_rbuffer_config *rcfg, int fidx)	
{
	int i;
	uint64_t random = cf_get_rand64();
		
	memset(&CHDR, 0, sizeof(cf_rbuffer_chdr));
	CHDR.magic = RBUFFER_FILE_MAGIC;
	CHDR.signature = random; 	
	CHDR.seg_size = RBUFFER_SEG_DATASIZE;
	CHDR.version = 1;
	if (CHDR.seg_size > RBUFFER_SEG_DATASIZE) {
		return false;		
	}
	RDES->wctx.flag &= ~RBUFFER_CTX_FLAG_NEEDSEEK;
	CHDR.rec_size = rcfg->rec_size;
	CHDR.nfiles = rcfg->nfiles;
	
	for (i=0; i<CHDR.nfiles; i++) 
	{
		memset(&HDR(i), 0, sizeof(cf_rbuffer_hdr));
		HDR(i).fidx = i;
		HDR(i).fsize = rcfg->fsize[i];	
		if ((CHDR.seg_size * 2) >= rcfg->fsize[i])
		{
			cf_warning(CF_RBUFFER, "Too small file size %s cannot init ring buffer\n",rcfg->fname[i]);
			return false;
		}
	}		
	return true;
}

// Client API to init the ring buffer
// * Checks the validity of the passed in files
// * Sets up the file new files have been added.
// * Sets up the in memory descriptor for ring buffer. 
// * Sets up all the required in pointer (read/write/wrap/eof).
//
//  Parameter:
//		rcfg		: Ring buffer configuration
// 		persist 	: TRUE if log are persistent
//					  FALSE otherwise
//
//  Caller:
//		XDS at startup to set up digest log
//
//	Synchronization:
//		None required function called at the startup time		
// 
// 
cf_rbuffer *
cf_rbuffer_init(cf_rbuffer_config *rcfg)
{
	cf_rbuffer *rbuf_des = NULL;
	int 	i;
	int stamped_fileidx = -1;
	
	if (rcfg == NULL ) 
	{
		return ( NULL );
	}
	
	for (i=0; i<rcfg->nfiles; i++)
	{
		if (rcfg->fname[i] == NULL) 
		{
			break;
		}
	}
		
	if (i == 0) 
	{
		cf_warning(CF_RBUFFER, "rbuffer: files: init: NULL file name specified, bad config");
		return ( NULL );
	}
				
    // Step1: Re-size all files to the passed in size
	for (i=0; i<rcfg->nfiles; i++) 
	{
		int fd = open(rcfg->fname[i], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (-1 == fd) 
		{
			cf_warning(CF_RBUFFER, "unable to open digest log file %s ",rcfg->fname[i]);
			return( NULL );
		}

		// RESOLVE if file already exist then truncate should check to makes sure the
		// persist is not set
		if (0 != ftruncate(fd, (off_t)rcfg->fsize[i])) 
		{
			cf_info(CF_RBUFFER, "unable to truncate file: errno %d", errno);
			close(fd);
			return( NULL );
		}
		close(fd);
	}

	// Step 2: Open and Setup file Header 
	RDES = malloc(sizeof(cf_rbuffer));
	memset(RDES, 0, sizeof(cf_rbuffer));
	RDES->strapped = false;
	
	if (rcfg->persist) 
	{
		// Get the File head magic and make it consistent on all the files
		for (i = 0; i < rcfg->nfiles; i++) 
		{
			if ((RDES->mfd[i] = fopen(rcfg->fname[i], "r+b")) == NULL) 
			{
				cf_warning(CF_RBUFFER, "Failed to open the digest log file %s for metadata", rcfg->fname[i]);
				goto backout;
			}

			if (stamped_fileidx == -1)
			{
				rewind(RDES->mfd[i]);
				if (1 != fread(&CHDR, sizeof(cf_rbuffer_chdr), 1, RDES->mfd[i])) 
				{
					cf_warning(CF_RBUFFER, "Failed to read digest log file %s", rcfg->fname[i]);
					goto backout;
				}

				if (CHDR.magic != 0) 
				{
					stamped_fileidx = i;
					if (1 != fread(&HDR(i), sizeof(cf_rbuffer_hdr), 1, RDES->mfd[i])) 
					{
						cf_warning(CF_RBUFFER, "Failed to read digest log file %s", rcfg->fname[i]);
						goto backout;
					}
				}
			}
		}

		// System booting up for the first time no stamped file found
		if (stamped_fileidx == -1)	
		{
			if (!cf__rbuffer_bootstrap(RDES, rcfg, -1)) 
			{
				cf_warning(CF_RBUFFER, "Boot strap failed for digest log file %s", rcfg->fname[i]);
				goto backout;
			}
		}	
		else 
		{
			for (i = 0; i < rcfg->nfiles; i++) 
			{
				if (i == stamped_fileidx) 
				{
					continue;
				}

				fseek(RDES->mfd[i], sizeof(cf_rbuffer_chdr), SEEK_SET);
				if (1 != fread(&HDR(i), sizeof(cf_rbuffer_hdr), 1, RDES->mfd[i])) 
				{
					cf_warning(CF_RBUFFER, "Failed to read digest log file %s", rcfg->fname[i]);
					goto backout;
				}
		
				if (HDR(i).fsize == 0) 
				{
					HDR(i).fsize = rcfg->fsize[i];
				}
				else if (HDR(i).fsize != rcfg->fsize[i])
				{
					cf_warning(CF_RBUFFER, "Passed in file size does not match already stamped file size %s", rcfg->fname[i]);
					goto backout;
				}
			}
		}
	}
	else 
	{
		stamped_fileidx = -1;	
		if (!cf__rbuffer_bootstrap(RDES, rcfg, -1)) 
		{
			cf_warning(CF_RBUFFER, "Boot strap failed for digest log file %s", rcfg->fname[i]);
			goto backout;
		}
	}
	for (i = 0; i < rcfg->nfiles; i++) 
	{
		if ((RDES->rfd[i] = fopen(rcfg->fname[i], "r+b")) == NULL) 
		{
			cf_warning(CF_RBUFFER, "Failed to open the digest log file %s for reading", rcfg->fname[i]);
			goto backout;
		}
		setvbuf(RDES->rfd[i], NULL, _IONBF, 0);
		fseek(RDES->rfd[i], RBUFFER_FILE_HEADER_SIZE, SEEK_SET);
	
		if (!rcfg->persist)
		{
			if ((RDES->wfd[i] = fopen(rcfg->fname[i], "w+b")) == NULL) 
			{	
				cf_warning(CF_RBUFFER, "Failed to open the digest log file %s for writing", rcfg->fname[i]);
				goto backout;
			}
		}
		else
		{
			if ((RDES->wfd[i] = fopen(rcfg->fname[i], "r+b")) == NULL) 
			{	
				cf_warning(CF_RBUFFER, "Failed to open the digest log file %s for writing", rcfg->fname[i]);
				goto backout;
			}
		}

		setvbuf(RDES->wfd[i], NULL, _IONBF, 0);
		fseek(RDES->wfd[i], RBUFFER_FILE_HEADER_SIZE, SEEK_SET);
	}

	// Step 3: Setup prev/next pointer and read and write buffer.
	if (!cf__rbuffer_setup(RDES, rcfg))
	{
		goto backout;
	}
	
	// Persist the header
	cf_rbuffer_persist(RDES);
	cf_rbuffer_log(RDES);
	RDES->strapped = true;
	
	return RDES;
backout:
	for (i=0 ; i<rcfg->nfiles ; i++) 
	{
		cf_rbuffer_close(RDES);	
	}
	return ( NULL );
}

//  Client API for reinit of ring buffer
//  * Checks the validity of the passed in info
//  * Sets up the file if new onese are added. Or any valid configuration
//		changes
//  * Blocks all the writes/reads and update the relevant in-memory 
//		and on-disk meta info.
//	* Resume read/write
//
//  Parameter:
//		rbuf_des	: ring buffer descriptor
//		rcfg		: new configuration
// 
//  Caller:
//		XDS
//
//	Synchronization:
//		Blocks all reads and writes
bool 
cf_rbuffer_reinit(cf_rbuffer *rbuf_des, cf_rbuffer_config *rcfg)
{
	cf_rbuffer *new_rbuf_des = cf_rbuffer_init(rcfg);
	if (new_rbuf_des == new_rbuf_des) {}	
	return true;
}

// Client API to enable disable ring buffer tracing
// 
// Parameter:
//		rbuf_des	: Ring buffer descriptor
//		val			: true to enable false to disable
//
// Synchronization:
//		None required
void cf_rbuffer_trace(cf_rbuffer *rbuf_des, bool val)
{
    if (val == true)
        CHDR.flag |= RBUFFER_FLAG_TRACE;
    else
        CHDR.flag &= ~RBUFFER_FLAG_TRACE;
}


//  Client API to seek the passed in context pointer.
// 
//  With the notion of moving back as moving to the left and moving forward as moving to 
//  right; Condition which apply are
// 	1. Read cannot move to the left of the head of the ring buffer.
//	2. Read cannot move to the right of the current write pointer.
//  
//  Parameter:
//		rbuf_des	: Ring buffer Desriptor
//		ctx		: context pointer
//		num_recs	: Number of records to seek
//
//  Synchronization:
//		Acquires lock on the context mutex when seeking.
int 
cf_rbuffer_seek(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx, int num_recs, int whence)
{
	uint64_t	num_seg=0;
	bool		forward;

	pthread_mutex_lock(&RDES->mlock);
	if (num_recs > 0)
		forward = true;
	else
	{
		forward = false;
		num_recs = num_recs * (-1);
	}
	
	if (ctx == NULL)
	{
		ctx = &RDES->rctx;
	}	
	else
	{
		cf_rbuffer_getsetctx(rbuf_des, ctx, whence);
			
		// invalid version return -1 seek position is invalid
		if (cf__rbuffer_fread(RDES, ctx) == -1)
		{
			pthread_mutex_unlock(&RDES->mlock);
			return -1;
		}
	}

	RBTRACE(RDES, debug, "%d move %d with max(%d) from %ld:%ld ",forward, num_recs, MAX_RECS, ctx->ptr.seg_id, ctx->ptr.rec_id);
	if (forward)
	{
		if (num_recs > (MAX_RECS - ctx->ptr.rec_id))
		{
			uint64_t temp_recid = ((num_recs - (MAX_RECS - ctx->ptr.rec_id)) % MAX_RECS);
			num_seg = ((num_recs - (MAX_RECS - ctx->ptr.rec_id)) / MAX_RECS);
			if (temp_recid == MAX_RECS)
				num_seg = num_seg + 1;

			if (!num_seg || (num_seg != cf__rbuffer_rseek(rbuf_des, ctx, num_seg, forward)))
			{
				RBTRACE(RDES, debug, " gives %ld and reaches nowhere", num_seg);
				pthread_mutex_unlock(&RDES->mlock);
				return 0;
			}
			ctx->ptr.rec_id = temp_recid;
		}
		else
		{
			ctx->ptr.rec_id += num_recs;
		}
	}
	else 
	{
		if (ctx->ptr.rec_id < num_recs)
		{
			uint64_t temp_recid = MAX_RECS - ((num_recs - ctx->ptr.rec_id) % MAX_RECS);
			num_seg = ((num_recs - ctx->ptr.rec_id) / MAX_RECS);
			if(temp_recid != 0)
				num_seg = num_seg + 1;

			if (!num_seg || (num_seg != cf__rbuffer_rseek(rbuf_des, ctx, num_seg, forward))) 
			{
				RBTRACE(RDES, debug, "gives %ld and reaches nowhere", num_seg);
				pthread_mutex_unlock(&RDES->mlock);
				return 0;
			}
			ctx->ptr.rec_id = temp_recid;
		}
		else
		{
			ctx->ptr.rec_id -= num_recs;
		}

	}
	RBTRACE(RDES, debug, "gives %ld and reaches %ld:%ld", num_seg, ctx->ptr.seg_id, ctx->ptr.rec_id);

	// need to force the read if seek happened
	ctx->buf.magic = 0;
	pthread_mutex_unlock(&RDES->mlock);
	return num_recs;
}

//  Client API to perform read from the ring buffer. 
//
// Parameter:
//		rbuf_des : Ring buffer descriptor
//		buf		 : Buffer to copy data into
//		numrecs  : Number of records requested
//		ctx		 : Client cached scan context. Null in case internal read
//				   pointer is to be used.
//
// Returns
//		number of records read
//		if number of records read is 0 then -1 in case of version mismatch
//
// Synchronization:
//		Acquires the context mutex when reading
int 
cf_rbuffer_read(cf_rbuffer *rbuf_des, cf_rbuffer_ctx *ctx, char *buf, int numrecs)
{
	int start = 0;
	int count = -1;
	int total = 0;
	int ret = 0;
	char type[11];

	if (ctx == NULL) 
	{
		sprintf(type, "Internal");
		ctx = &RDES->rctx;
	}
	else
	{
		sprintf(type, "Contextual");
	}
	
	pthread_mutex_lock(&ctx->lock);	

	while (numrecs > 0)
	{
		// If seek and read required 
		if ((MAX_RECS == ctx->ptr.rec_id)
			|| (ctx->flag & RBUFFER_CTX_FLAG_NEEDSEEK))
		{
			if (cf__rbuffer_rseek(RDES, ctx, 1, true) != 1) 
			{
				ctx->flag |= RBUFFER_CTX_FLAG_NEEDSEEK;
				break;
			}
	
			ctx->flag &= ~RBUFFER_CTX_FLAG_NEEDSEEK;

			ret = cf__rbuffer_fread(RDES, ctx);
			if (1 != ret)
			{
				break;
			}
		}
		else if (ctx->ptr.rec_id >= ctx->buf.num_recs)
		{
			ret = cf__rbuffer_fread(RDES, ctx);

			if (1 != ret)
			{
				break;
			}
			if (ctx->ptr.rec_id >= ctx->buf.num_recs)
			{
				break;
			}
		} 
		else if (ctx->buf.magic != RBUFFER_SEG_MAGIC) 
		{
			ret = cf__rbuffer_fread(RDES, ctx);

			if (1 != ret)
			{
				break;
			}
			if (ctx->buf.magic != RBUFFER_SEG_MAGIC)
			{
				break;
			}
		}
	
		// Do not read further if version mismatch, need to re-read
		// the buffer
		if (ctx->version > ctx->buf.version)
		{
			ctx->buf.magic = 0;	
			RBTRACE(RDES, debug, "Version Mismatch %d:%d:%ld", ctx->version, ctx->buf.version,ctx->ptr.seg_id);
			break;
		}

		// Read data from in-memory buffer into passed in buffer
		if (numrecs <= (MAX_RECS - ctx->ptr.rec_id)) 
		{
			count = numrecs;
			numrecs = 0;	
		}	
		else 
		{
			count = MAX_RECS - ctx->ptr.rec_id;
			numrecs -= count;
		}

		memcpy(buf + (start * CHDR.rec_size),
				ctx->buf.data + (ctx->ptr.rec_id * CHDR.rec_size), 
				count * CHDR.rec_size);
		total = total + count;

		RBTRACE(RDES, debug, "%s Read [%d:%ld:%ld] max_recs:%d total:%d count:%d",
					type, ctx->ptr.fidx, ctx->ptr.seg_id, ctx->ptr.rec_id, MAX_RECS, numrecs, count);

		ctx->ptr.rec_id += count;
		start = total;
	}
	pthread_mutex_unlock(&ctx->lock);
	cf_atomic64_add(&RDES->read_stat, total);
	if ((ret == -1) && (total == 0))
		return -1;
	return total;	
}


// Client API to perform write 
//
// Parameter:
//		rbuf_des : Ring buffer descriptor
//		buf		 : Buffer to copy data from
//		numrecs  : Number of records requested
//
//  Returns:
//		0: On Success 
//		otherwise Failure
//
// Synchronization:
//		Acquires the write context mutex when writing.
int 
cf_rbuffer_write(cf_rbuffer *rbuf_des, char *buf, int numrecs)
{
	cf_rbuffer_ctx *ctx = &RDES->wctx;

	pthread_mutex_lock(&ctx->lock);

	int start = 0;
	int count = -1;
	int total = 0;

	// Seek if previous seek failed
	if (ctx->flag & RBUFFER_CTX_FLAG_NEEDSEEK)
	{
		if (!cf__rbuffer_wseek(RDES, ctx)) 
		{
			goto err;
		}
		ctx->flag &= ~RBUFFER_CTX_FLAG_NEEDSEEK;
	}

	while (numrecs > 0)
	{
		// Write passed in records into the in-memory write
		// buffer
		if (numrecs <= (MAX_RECS - ctx->ptr.rec_id)) 
		{
			count = numrecs;
			numrecs = 0;	
		}	
		else 
		{
			count = MAX_RECS - ctx->ptr.rec_id;
			numrecs -= count;
		}
		
		// Insert into the in-memory buffer
		memcpy((ctx->buf.data + (ctx->ptr.rec_id * CHDR.rec_size)), 
				(buf + (start * CHDR.rec_size)), 
				count * CHDR.rec_size);
		total = total + count;

		RBTRACE(RDES, debug, "Wrote %d:%ld:%ld",ctx->ptr.fidx, ctx->ptr.seg_id, 	
					ctx->ptr.rec_id);
		ctx->ptr.rec_id += count;
		RDES->batch_pos += count;

		// If the buffer is full write to the file.	
		// Write the buffer and update the write pointer
		if (ctx->ptr.rec_id == MAX_RECS) 
		{
			if (!cf__rbuffer_fwrite(RDES, ctx)) 
			{
				goto err;
			}
	
			if (!cf__rbuffer_wseek(RDES, ctx)) 
			{
				ctx->flag |= RBUFFER_CTX_FLAG_NEEDSEEK;
				// only seek failed .. write is successful
				// if return count of number of records written
				break;
			}

			// Persist the data and meta data on to disk
			cf_rbuffer_persist(rbuf_des);
		}
		else if (RDES->batch_pos == RDES->batch_size) 
		{
			if (!cf__rbuffer_fwrite(RDES, ctx)) 
			{
				goto err;
			}
			// Persist the data and meta data on to disk
			cf_rbuffer_persist(rbuf_des);
		}
		else if (numrecs == 0)
		{
			if (!cf__rbuffer_fwrite(RDES, ctx)) 
			{
				goto err;
			}
			cf_rbuffer_persist(rbuf_des);
		}
		start = total;
	}			
	cf_atomic64_add(&RDES->write_stat, total);
	pthread_mutex_unlock(&ctx->lock);
	return total;
err:
	pthread_mutex_unlock(&ctx->lock);
	return -1;
}

// Client API to seek the start to the passed in context. This is used
// for the reclaim.
//
//  Parameter:	
//		rbuf_des	: Ring buffer Descriptor
//		rbctx		: point to which start seeks.
//
//	Caller:
//		XDS
//
//	Synchronization:
//		Acquires meta lock while reading.
//
//	Returns:
//		Number of records reclaimed
uint64_t 
cf_rbuffer_setstart(cf_rbuffer *rbuf_des, cf_rbuffer_ctx* ctx)
{
	uint64_t segid, tsegid;
	int	 fidx, tfidx;
	uint64_t num_seg = 0;
	pthread_mutex_lock(&RDES->mlock);

	segid = CHDR.sptr.seg_id;
	fidx = CHDR.sptr.fidx;

	tsegid = ctx->ptr.seg_id;
	tfidx = ctx->ptr.fidx;

	if ((segid == tsegid)
		&& (fidx == tfidx))
	{
		//Nothing to seek
		pthread_mutex_unlock(&RDES->mlock);
		return 0;
	}
	
	RBTRACE(RDES, debug, "Start: Start Reclaimed from [%d:%ld] to [%d:%ld]",
				fidx, segid, tfidx, tsegid);
	
	cf__rbuffer_sanity(rbuf_des);

	while (1)
	{
		segid = RBUFFER_NEXT_SEG(segid, fidx);

		// Hit the start of the file. 
		if (segid == HDR(fidx).sseg.seg_id)
		{
			segid = HDR(fidx).nseg.seg_id;
			fidx = HDR(fidx).nseg.fidx;
		}

		// Hit read
		if ((fidx == RDES->rctx.ptr.fidx)		
			&& (segid == RDES->rctx.ptr.seg_id)) 
		{
			// cannot seek beyond read
			break;
		}

		// Hit the rbuffer header; is a bug should not 
		// hit self
		if ((fidx == CHDR.sptr.fidx)
			&& (segid == CHDR.sptr.seg_id)) 
		{
			// is error do not seek
			goto end;
		}
		num_seg++;
		if ((segid == tsegid)
			&& (fidx == tfidx))
		{
			break;
		}
	}

	RBTRACE(RDES, debug, "End: Start Reclaimed from [%d:%ld] to [%d:%ld]",
				CHDR.sptr.fidx, CHDR.sptr.seg_id,
				ctx->ptr.fidx, ctx->ptr.seg_id);

	CHDR.sptr.fidx = ctx->ptr.fidx;
	CHDR.sptr.seg_id = ctx->ptr.seg_id;
	CHDR.sptr.rec_id = 0;
	pthread_mutex_unlock(&RDES->mlock);
	return num_seg*MAX_RECS;
end:
	pthread_mutex_unlock(&RDES->mlock);
	return 0;
}

//  Client API to request and cache read nad write context
//
//  Parameter:	
//		rbuf_des	: Ring buffer Descriptor
//		iswrite		: true [current write]
//					: false [current read]
//
//	Caller:
//		XDS
//
//	Synchronization:
//		Acquires meta lock while reading.
//
//	Returns:
//	 	Valid pointer on success
//		NULL in case of failure.
cf_rbuffer_ctx *
cf_rbuffer_getsetctx(cf_rbuffer *rbuf_des, cf_rbuffer_ctx* ctx, int whence)
{
	cf_rbuffer_ptr *ptr;
	int i;
	
	pthread_mutex_lock(&RDES->mlock);

	// SEEK_CUR is seek to read pointer
	if (whence == SEEK_CUR)
		ptr = &RDES->rctx.ptr;
	// SEEK_END is seek to write pointer
	if (whence == SEEK_END)
		ptr = &RDES->wctx.ptr;
	// SEEK_SET is seek to start pointer
	else 
		ptr = &CHDR.sptr;

	if (ctx == NULL)
	{
		cf_rbuffer_ctx *new_ctx = malloc(sizeof(cf_rbuffer_ctx));
		memset(new_ctx, 0, sizeof(cf_rbuffer_ctx));
		new_ctx->ptr.fidx = ptr->fidx;
		new_ctx->ptr.seg_id = ptr->seg_id;
		new_ctx->ptr.rec_id = ptr->rec_id;
	
		pthread_mutex_init(&new_ctx->lock, NULL);

		for (i = 0; i<CHDR.nfiles; i++)
		{
			if ((new_ctx->fd[ptr->fidx] = fopen(HDR(ptr->fidx).fname, "rb")) == NULL) 
			{
				cf_warning(CF_RBUFFER, "Failed to open the digest log file %s for caching context", HDR(new_ctx->ptr.fidx).fname);
				cf_rbuffer_closectx(new_ctx);
				pthread_mutex_unlock(&RDES->mlock);
				return ( NULL );
			}
		}
		memset(new_ctx->buf.data, 0, CHDR.seg_size);
		new_ctx->buf.num_recs = 0;	
		new_ctx->buf.magic = 0;	
		new_ctx->buf.version = 0;
	
		new_ctx->version = 0;
		new_ctx->flag = RBUFFER_CTX_FLAG_SEARCH;

		pthread_mutex_unlock(&RDES->mlock);
		return new_ctx;
	}
	else
	{
		if (whence == SEEK_CUR)
		{
			pthread_mutex_unlock(&RDES->mlock);
			return ctx;
		}
		pthread_mutex_lock(&ctx->lock);
		ctx->ptr.fidx = ptr->fidx;
		ctx->ptr.seg_id = ptr->seg_id;
		ctx->ptr.rec_id = ptr->rec_id;

		memset(ctx->buf.data, 0, CHDR.seg_size);
		ctx->buf.num_recs = 0;	
		ctx->buf.magic = 0;	
		ctx->buf.version = 0;
		ctx->version = 0;

		pthread_mutex_unlock(&ctx->lock);
		pthread_mutex_unlock(&RDES->mlock);
		return ctx;
	}	
}
		
//  Client API to request and cache read nad write context
//
//  Parameter:	
//		ctx
//
//	Caller:
//		XDS
//
void  
cf_rbuffer_closectx(cf_rbuffer_ctx *ctx)
{
	pthread_mutex_destroy(&ctx->lock);
	
	for (int i=0; i<RBUFFER_MAX_FILES;i++)
	{
		if (ctx->fd[i])
		{
			fclose(ctx->fd[i]);
		}
		ctx->fd[i] = NULL;
	}
	free(ctx);
}

//  Client API to find outstanding records between read and write pointer.
// 
//  Parameter:
//		rbuf_des	: Ring buffer Descriptor
//
//  Caller:
//		XDS for resuming shipping after node restarts
//
// 	Synchronization:
//		Either caller needs to have sync or should be single threaded.
//
// Returns:
//		Number of records
uint64_t
cf_rbuffer_outstanding(cf_rbuffer *rbuf_des)
{
	uint64_t fsize = 0;
	uint64_t i;
	cf_rbuffer_ctx ctx;
	uint64_t num_recs = 0;

	// memcpy can be used ctx has no buffer changes are done to the
	memcpy(&ctx, &RDES->rctx, sizeof(cf_rbuffer_ctx));
	for (i = 0; i < CHDR.nfiles; i++)
		fsize += HDR(i).fsize;

	uint64_t num_segs = fsize / RBUFFER_SEG_SIZE;

	// Seek as much as possible pretty dump but works and is in-memory
	// and is only done once	
	for (i = 0; i < num_segs; i++)
		if (cf__rbuffer_rseek(rbuf_des, &ctx, 1, true) != 1)
			break;	

	if (i)
	{
		num_recs = ((MAX_RECS - RDES->rctx.ptr.rec_id)
				+ (i - 1)*MAX_RECS
				+ RDES->wctx.ptr.rec_id);
	}
	else
	{
		num_recs = (RDES->wctx.ptr.rec_id - RDES->rctx.ptr.rec_id);
	}
	return num_recs;
}




//  Client API to close the ring buffer interface.
// 
//  Parameter:
//		rbuf_des	: Ring buffer Descriptor
//
//  Caller:
//		XDS
//
// 	Synchronization:
//		None caller needs to be make sure no read and write happening when close 
//		is called otherwise the behavior is undefined.
//
// Returns:
//		0 on success
//	 	otherwise failure
//
bool 
cf_rbuffer_close(cf_rbuffer *rbuf_des)
{
	int i;

	if (RDES->strapped)
	{
		// Persist the data and meta data on to disk
		cf_rbuffer_persist(rbuf_des);
	}

	// DESTROY READ AND WRITE and cached contexts here.
	pthread_mutex_destroy(&RDES->rctx.lock);
	pthread_mutex_destroy(&RDES->wctx.lock);
	pthread_mutex_destroy(&RDES->mlock);

	// Close all open FD
	for (i = 0; i < CHDR.nfiles; i++)
	{
		if (rbuf_des->mfd[i] != NULL)
			fclose(rbuf_des->mfd[i]);
		if (rbuf_des->rfd[i] != NULL)
			fclose(rbuf_des->rfd[i]);
		if (rbuf_des->wfd[i] != NULL)
			fclose(rbuf_des->wfd[i]);
	}
	memset(rbuf_des, 0, sizeof(cf_rbuffer));
	free(RDES);
	RDES = NULL;
	return 0;
}


// Important infrastructure piece it better works and is loud in case of failure. 
// Standalone testing ....

typedef struct testrecord_s 
{
	char buf[50];
} testrecord;

int rbuffer_writes(void *arg, int start, uint64_t total, int size) 
{
	cf_rbuffer *rb = (cf_rbuffer *)(arg); 
	testrecord rec[size];
	uint64_t wcount = start; 
	uint64_t ret = 0;
	uint64_t maxcount = start+total;
	while ( wcount < maxcount ) 
	{
		int counter = 0;
		int write_size = 0;
		
		memset (rec, 0, rb->chdr.rec_size*size);
	
		if ((maxcount - wcount) < size)
		{
			write_size = maxcount - wcount;
		}
		else
		{
			write_size = size;
		}

		while (counter < write_size)
		{
			sprintf(rec[counter].buf, "%49ld", wcount);
			wcount++;
			counter++;
		}

		counter = 0;
		while (counter < write_size)
		{
			ret = cf_rbuffer_write(rb, (char *)(rec+counter), write_size - counter);
			if (ret == -1)
			{
				sleep(2);
				continue;
			}
			if (ret != (write_size - counter))
			{
				counter = ret;	
				fprintf(stderr, "Could not write attempting rewrite %d %d\n", counter, write_size);
				continue;
			}
			break;
		}
	}
	fprintf(stderr, "Write Total: %ld Batch Size: %d\n", total, size);
	cf_rbuffer_fflush(rb);
	return total;
}
void *rbuffer_writer_1(void *arg)
{
	rbuffer_writes(arg, 0, 10000, 1);
	pthread_exit(NULL);
}

void *rbuffer_writer_2(void *arg)
{
	rbuffer_writes(arg, 10000,10000, 1);
	pthread_exit(NULL);
}

void *rbuffer_writer_3(void *arg)
{
	rbuffer_writes(arg, 0, 10000, 500);
	pthread_exit(NULL);
}

void *rbuffer_writer_4(void *arg)
{
	rbuffer_writes(arg, 0, 30000, 1);
	pthread_exit(NULL);
}

int rbuffer_reads(void *arg, uint64_t total, int size, cf_rbuffer_ctx* myctx) 
{
	cf_rbuffer *rb = (cf_rbuffer *)(arg); 
	testrecord rec[size];	
	char tracker[50001];
	memset(tracker, 0, 20000);
	long int start=-1;
	uint64_t count=1;
	uint64_t duplicate=0;
	uint64_t cur = 0;
	int ret;
	while ( count <= total )
	{
		memset(rec, 0, rb->chdr.rec_size*size);
		ret = cf_rbuffer_read(rb, myctx, (char *)&rec, size); 
		if (ret == 0) 	
		{
			sleep(1);
		} 
		else 
		{
			if (ret != size)
			{
				fprintf(stderr, "Read %d records instead of %d \n", ret,size);
			}
			int counter = 0;
			while (counter < ret)
			{
				cur = atol(rec[counter].buf);
				if(start == -1)
					start = cur;
				if (tracker[cur] != 0)
				{
					fprintf(stderr, "Duplicate Read %ld\n", cur);
					duplicate++;
					count--;
					counter++;
					continue;
				}
				else
				{
					tracker[cur] = 1;
				}
				counter++;
			}
			count += counter;
		}
	}
	fprintf(stderr, "READ  Total: %ld [%ld to %ld] Batch Size: %d: Duplicates: %ld \n", total, start, cur, size, duplicate);
	return total;
}

void *rbuffer_reader_1(void *arg)
{
	rbuffer_reads(arg, 10000, 1, NULL);
	pthread_exit(NULL);
}

void *rbuffer_reader_2(void *arg)
{
	rbuffer_reads(arg, 20000, 1, NULL);
	pthread_exit(NULL);
}

void *rbuffer_reader_3(void *arg)
{
	rbuffer_reads(arg, 10000, 20, NULL);
	pthread_exit(NULL);
}


//  Variation:
//		Overwrite [Write should be able to overwrite and position 
//					it properly]
//		Persist	  [Across reboot and shutdown write and read should
//					be able to resume from the point it left ]

//  Test Case 1:
//
//	Thread1 : Writes to the file single record
//  Thread2 : Reads from the file single record
int
cf_rbuffer_test1()
{
	pthread_t	rbuffer_writer_th;
	pthread_t	rbuffer_reader_th;
	int 		type = 0;
	while (type < 2)
	{
		cf_rbuffer_config cfg;
		cfg.rec_size = 50;
		cfg.batch_size = 50;
		cfg.trace = true;
		if (type == 0)
		{
			cfg.nfiles = 1;
			cfg.fname[0] = strdup("/tmp/digestr_logtest1");
			cfg.fsize[0] = 1000000;
		}
		else
		{
			cfg.nfiles = 3;
			cfg.fname[0] = strdup("/tmp/digestr_logtest1_0");
			cfg.fsize[0] = 100000;
			cfg.fname[1] = strdup("/tmp/digestr_logtest1_1");
			cfg.fsize[1] = 100000;
			cfg.fname[2] = strdup("/tmp/digestr_logtest1_2");
			cfg.fsize[2] = 100000;
		}

	
		// Nooverwrite / Nopersist
		cfg.overwrite = false;
		cfg.persist = false;
		cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

		if (!rb)
		{
			fprintf(stderr, "Ring Buffer Init Failed\n");
			return -1;
		}
	
		pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_1, rb);	
		pthread_create(&rbuffer_reader_th, 0, rbuffer_reader_1, rb);	
	
		void *retval;
		if (0 != pthread_join(rbuffer_writer_th, &retval))
		{
			fprintf(stderr, "rbuffer test 1: write could not join %d\n",errno);
			return (-1);
		}
		
		if (0 != retval)
		{
			fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
			return (-1);	
		}
	
		if (0 != pthread_join(rbuffer_reader_th, &retval))
		{
			fprintf(stderr, "rbuffer test 1: read could not join %d\n",errno);
			return (-1);
		}
	
		if (0 != retval)
		{
			fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
			return (-1);	
		}
	
		fprintf(stderr, "Stats [Total file:%d Total Reads:%ld: Total Writes:%ld: Total Fwrites:%ld: Total Freads:%ld]\n", 
					(type==0)?1:3, rb->read_stat, rb->write_stat, rb->fwrite_stat, rb->fread_stat);
		cf_rbuffer_close(rb);
		type++;
	}
	
	return 0;
}

//  Test Case 2:
//  Thread1 : Write to the file multiple record
//	Thread2 : Write to the file multiple record
//  Thread3 : Read from the file multiple record
int
cf_rbuffer_test2()
{
	pthread_t	rbuffer_writer_th;
	pthread_t	rbuffer_writer_th1;
	pthread_t	rbuffer_reader_th;
	int			type = 0;
	cf_rbuffer_config cfg;

	while (type < 2)
	{
		cfg.rec_size = 50;
		cfg.batch_size = 50;
		cfg.trace = true;

		if (type == 0)
		{
			cfg.nfiles = 1;
			cfg.fname[0] = strdup("/tmp/digestr_logtest2");
			cfg.fsize[0] = 1000000;
		}
		else
		{
			cfg.nfiles = 3;
			cfg.fname[0] = strdup("/tmp/digestr_logtest2_0");
			cfg.fsize[0] = 100000;
			cfg.fname[1] = strdup("/tmp/digestr_logtest2_1");
			cfg.fsize[1] = 100000;
			cfg.fname[2] = strdup("/tmp/digestr_logtest2_2");
			cfg.fsize[2] = 100000;
		}

		// Nooverwrite / Nopersist
		cfg.overwrite = false;
		cfg.persist = false;
		cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

		if (!rb)
		{
			fprintf(stderr, "Ring Buffer Init Failed\n");
			return -1;
		}
		pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_1, rb);	
		pthread_create(&rbuffer_writer_th1, 0, rbuffer_writer_2, rb);	
		pthread_create(&rbuffer_reader_th, 0, rbuffer_reader_2, rb);	

		void *retval;
		if (0 != pthread_join(rbuffer_writer_th, &retval))
		{
			fprintf(stderr, "rbuffer test 2: write 1 could not join %d\n",errno);
			return (-1);
		}

		if (0 != retval)
		{
			fprintf(stderr, "rbuffer test 2: returned error %p\n",retval);
			return (-1);	
		}

		if (0 != pthread_join(rbuffer_writer_th1, &retval))
		{
			fprintf(stderr, "rbuffer test 2: write 2 could not join %d\n",errno);
			return (-1);
		}

		if (0 != retval)
		{
			fprintf(stderr, "rbuffer test 2: returned error %p\n",retval);
			return (-1);	
		}

		if (0 != pthread_join(rbuffer_reader_th, &retval))
		{
			fprintf(stderr, "rbuffer test 2: read could not join %d\n",errno);
			return (-1);
		}

		if (0 != retval)
		{
			fprintf(stderr, "rbuffer test 2: returned error %p\n",retval);
			return (-1);	
		}
		fprintf(stderr, "Stats [Total file:%d Total Reads:%ld: Total Writes:%ld: Total Fwrites:%ld: Total Freads:%ld]\n", 
					(type==0)?1:3, rb->read_stat, rb->write_stat, rb->fwrite_stat, rb->fread_stat);
		cf_rbuffer_close(rb);
		type++;
	}

	return 0;
}


//  Test Case 3:
//
//  Thread1 : Write to the file multiple record
//	Thread2 : Read from the file multiple record
int
cf_rbuffer_test3()
{
	pthread_t	rbuffer_writer_th;
	pthread_t	rbuffer_reader_th;

	cf_rbuffer_config cfg;
	cfg.nfiles = 1;
	cfg.rec_size = 50;
	cfg.batch_size = 100;
	cfg.trace = true;
	cfg.fname[0] = strdup("/tmp/digestr_logtest3");
	cfg.fsize[0] = 1000000;

	
	// Nooverwrite / Nopersist
	cfg.overwrite = false;
	cfg.persist = false;
	cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

	if (!rb)
	{
		fprintf(stderr, "Ring Buffer Init Failed\n");
		return -1;
	}

	pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_3, rb);	
	pthread_create(&rbuffer_reader_th, 0, rbuffer_reader_3, rb);	

	void *retval;
	if (0 != pthread_join(rbuffer_writer_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: write could not join %d\n",errno);
		return (-1);
	}
	
	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}

	if (0 != pthread_join(rbuffer_reader_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: read could not join %d\n",errno);
		return (-1);
	}

	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}

	fprintf(stderr, "Stats [Total Reads:%ld: Total Writes:%ld: Total Fwrites:%ld: Total Freads:%ld]\n", 
					rb->read_stat, rb->write_stat, rb->fwrite_stat, rb->fread_stat);
	cf_rbuffer_close(rb);

	return 0;
}

//  Test Case 4:
//
//	Thread1 : Writes to the file single record
//  Thread2 : Reads from the file single record
//  with wrapping of writes
int
cf_rbuffer_test4()
{
	pthread_t	rbuffer_writer_th;
	pthread_t	rbuffer_reader_th;

	cf_rbuffer_config cfg;
	cfg.nfiles = 1;
	cfg.rec_size = 50;
	cfg.batch_size = 50;
	cfg.trace = true;
	cfg.fname[0] = strdup("/tmp/digestr_logtest4");
	cfg.fsize[0] = 100000;

	
	// Nooverwrite / Nopersist
	cfg.overwrite = false;
	cfg.persist = false;
	cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

	if (!rb)
	{
		fprintf(stderr, "Ring Buffer Init Failed\n");
		return -1;
	}

	pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_1, rb);	
	pthread_create(&rbuffer_reader_th, 0, rbuffer_reader_1, rb);	

	void *retval;
	if (0 != pthread_join(rbuffer_writer_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: write could not join %d\n",errno);
		return (-1);
	}
	
	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}

	if (0 != pthread_join(rbuffer_reader_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: read could not join %d\n",errno);
		return (-1);
	}

	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}

	fprintf(stderr, "Stats [Total Reads:%ld: Total Writes:%ld: Total Fwrites:%ld: Total Freads:%ld]\n", 
					rb->read_stat, rb->write_stat, rb->fwrite_stat, rb->fread_stat);
	cf_rbuffer_close(rb);

	return 0;
}

//  Test Case 5:
//  Thread1 : Write to the file multiple record
//	Thread2 : Write to the file multiple record
//  Thread3 : Read from the file multiple record
//	with write wrapping
int
cf_rbuffer_test5()
{
	pthread_t	rbuffer_writer_th;
	pthread_t	rbuffer_writer_th1;
	pthread_t	rbuffer_reader_th;
	cf_rbuffer_config cfg;

	cfg.nfiles = 1;
	cfg.rec_size = 50;
	cfg.batch_size = 50;
	cfg.trace = true;
	cfg.fname[0] = strdup("/tmp/digestr_logtest5");
	cfg.fsize[0] = 100000;

	// Nooverwrite / Nopersist
	cfg.overwrite = false;
	cfg.persist = false;
	cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

	if (!rb)
	{
		fprintf(stderr, "Ring Buffer Init Failed\n");
		return -1;
	}
	pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_1, rb);	
	pthread_create(&rbuffer_writer_th1, 0, rbuffer_writer_2, rb);	
	pthread_create(&rbuffer_reader_th, 0, rbuffer_reader_2, rb);	

	void *retval;
	if (0 != pthread_join(rbuffer_writer_th, &retval))
	{
		fprintf(stderr, "rbuffer test 2: write 1 could not join %d\n",errno);
		return (-1);
	}
	
	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 2: returned error %p\n",retval);
		return (-1);	
	}

	if (0 != pthread_join(rbuffer_writer_th1, &retval))
	{
		fprintf(stderr, "rbuffer test 2: write 2 could not join %d\n",errno);
		return (-1);
	}
	
	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 2: returned error %p\n",retval);
		return (-1);	
	}

	if (0 != pthread_join(rbuffer_reader_th, &retval))
	{
		fprintf(stderr, "rbuffer test 2: read could not join %d\n",errno);
		return (-1);
	}

	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 2: returned error %p\n",retval);
		return (-1);	
	}
	fprintf(stderr, "Stats [Total Reads:%ld: Total Writes:%ld: Total Fwrites:%ld: Total Freads:%ld]\n", 
					rb->read_stat, rb->write_stat, rb->fwrite_stat, rb->fread_stat);
	cf_rbuffer_close(rb);

	return 0;
}


//  Test Case 6:
//
//  Thread1 : Write to the file multiple record
//	Thread2 : Read from the file multiple record
//  Persist test 
int
cf_rbuffer_test6()
{
	cf_rbuffer_config cfg;
	cfg.nfiles = 1;
	cfg.rec_size = 50;
	cfg.batch_size = 100;
	cfg.trace = true;
	cfg.fname[0] = strdup("/tmp/digestr_logtest6");
	cfg.fsize[0] = 1000000000;

	
	// Nooverwrite / Nopersist
	cfg.overwrite = false;

	// open dummy so the file gets truncated
	cfg.persist = false;
	cf_rbuffer *rb = cf_rbuffer_init(&cfg);	
	cf_rbuffer_close(rb);

	cfg.persist = true;
	rb = cf_rbuffer_init(&cfg);	

	if (!rb)
	{
		fprintf(stderr, "Ring Buffer Init Failed\n");
		return -1;
	}

	int retval = rbuffer_writes(rb, 0, 50000, 1);
	
	if (50000 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %d\n",retval);
		return (-1);	
	}
	rbuffer_reads(rb, 10000, 20, NULL);
	cf_rbuffer_close(rb);
	rb = cf_rbuffer_init(&cfg);
	rbuffer_reads(rb, 10000, 20, NULL);
	cf_rbuffer_close(rb);

	return 0;
}

//  Test Case 7:
//
//  Thread1 : Write to the file multiple record
//	Thread2 : Read from the file multiple record
//  main thread doing seeks
int
cf_rbuffer_test7()
{
	pthread_t	rbuffer_writer_th;

	cf_rbuffer_config cfg;
	cfg.nfiles = 1;
	cfg.rec_size = 50;
	cfg.batch_size = 100;
	cfg.trace = true;
	cfg.fname[0] = strdup("/tmp/digestr_logtest7");
	cfg.fsize[0] = 10000000;

	
	// Nooverwrite / Nopersist
	cfg.overwrite = false;
	cfg.persist = false;
	cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

	if (!rb)
	{
		fprintf(stderr, "Ring Buffer Init Failed\n");
		return -1;
	}

	pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_1, rb);	
#if 0
	void *retval;
	if (0 != pthread_join(rbuffer_writer_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: write could not join %d\n",errno);
		return (-1);
	}
	
	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}
#endif
		
	// Manipulation with read pointer
	cf_rbuffer_ctx *myctx = cf_rbuffer_getsetctx(rb, NULL, SEEK_SET);
	rbuffer_reads(rb, 10, 1, myctx);
	if (cf_rbuffer_seek(rb, myctx, -11, SEEK_CUR) != 0)
		fprintf(stderr, "Assertion 1: Failed\n");
	else
		fprintf(stderr, "Assertion 1: Passed\n");
	cf_rbuffer_closectx(myctx);

	myctx = cf_rbuffer_getsetctx(rb, NULL, SEEK_SET);
	if (cf_rbuffer_seek(rb, myctx, 14999, SEEK_CUR) != 0)
		fprintf(stderr, "Assertion 2: Failed \n");
	else
		fprintf(stderr, "Assertion 2: Passed \n");
	cf_rbuffer_closectx(myctx);

	myctx = cf_rbuffer_getsetctx(rb, NULL, SEEK_SET);
	if ((cf_rbuffer_seek(rb, myctx, 100, SEEK_CUR) != 100)
		|| (rbuffer_reads(rb, 10, 1, myctx) != 10))
		fprintf(stderr, "Assertion 3: Failed \n");
	else
		fprintf(stderr, "Assertion 3: Passed \n");
	cf_rbuffer_closectx(myctx);

	myctx = cf_rbuffer_getsetctx(rb, NULL, SEEK_SET);
	if ((cf_rbuffer_seek(rb, myctx, 99, SEEK_CUR) != 99)
		|| (rbuffer_reads(rb, 10, 1, myctx) != 10)
		|| (cf_rbuffer_seek(rb, myctx, -99, SEEK_CUR) != 99)
		|| (rbuffer_reads(rb, 10, 1, myctx) != 10))
		fprintf(stderr, "Assertion 4: Failed \n");
	else
		fprintf(stderr, "Assertion 4: Passed \n");
	cf_rbuffer_closectx(myctx);
	
	cf_rbuffer_close(rb);
	return 0;
}

//  Test Case 8:
//
//  Multipe file
//	Thread1 : Writes to the file single record
//  Thread2 : Reads from the file single record
int
cf_rbuffer_test8()
{
	pthread_t	rbuffer_writer_th;
	pthread_t	rbuffer_reader_th;

	cf_rbuffer_config cfg;
	cfg.nfiles = 2;
	cfg.rec_size = 50;
	cfg.batch_size = 50;
	cfg.trace = true;
	cfg.fname[0] = strdup("/tmp/digestr_logtest8_0");
	cfg.fname[1] = strdup("/tmp/digestr_logtest8_1");
	cfg.fsize[0] = 100000;
	cfg.fsize[1] = 100000;

	
	// Nooverwrite / Nopersist
	cfg.overwrite = false;
	cfg.persist = false;
	cf_rbuffer *rb = cf_rbuffer_init(&cfg);	

	if (!rb)
	{
		fprintf(stderr, "Ring Buffer Init Failed\n");
		return -1;
	}

	pthread_create(&rbuffer_writer_th, 0, rbuffer_writer_1, rb);	
	pthread_create(&rbuffer_reader_th, 0, rbuffer_reader_1, rb);	

	void *retval;
	if (0 != pthread_join(rbuffer_writer_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: write could not join %d\n",errno);
		return (-1);
	}
	
	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}

	if (0 != pthread_join(rbuffer_reader_th, &retval))
	{
		fprintf(stderr, "rbuffer test 1: read could not join %d\n",errno);
		return (-1);
	}

	if (0 != retval)
	{
		fprintf(stderr, "rbuffer test 1: returned error %p\n",retval);
		return (-1);	
	}

	fprintf(stderr, "Stats [Total Reads:%ld: Total Writes:%ld: Total Fwrites:%ld: Total Freads:%ld]\n", 
					rb->read_stat, rb->write_stat, rb->fwrite_stat, rb->fread_stat);
	cf_rbuffer_close(rb);

	return 0;
}


int 
cf_rbuffer_testall()
{
	int passcount = 0;
	
	if (0 != cf_rbuffer_test1())
		fprintf(stderr, "Test 1: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 1: PASS\n"); 
	}
	fprintf(stderr, "\n"); 
	
	if (0 != cf_rbuffer_test2())
		fprintf(stderr, "Test 2: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 2: PASS\n"); 
	}

	fprintf(stderr, "\n"); 
	if (0 != cf_rbuffer_test3())
		fprintf(stderr, "Test 3: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 3: PASS\n"); 
	}

	fprintf(stderr, "\n"); 
	if (0 != cf_rbuffer_test4())
		fprintf(stderr, "Test 4: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 4: PASS\n"); 
	}
	
	
	fprintf(stderr, "\n"); 
	if (0 != cf_rbuffer_test5())
		fprintf(stderr, "Test 5: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 5: PASS\n"); 
	}

	fprintf(stderr, "\n"); 

	if (0 != cf_rbuffer_test6())
		fprintf(stderr, "Test 6: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 6: PASS\n"); 
	}


	if (0 != cf_rbuffer_test7())
		fprintf(stderr, "Test 7: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 7: PASS\n"); 
	}

	if (0 != cf_rbuffer_test8())
		fprintf(stderr, "Test 8: FAIL\n"); 
	else
	{
		passcount++;
		fprintf(stderr, "Test 8: PASS\n"); 
	}
	
	if (passcount != 8)
		return ( -1 );

	return 0;
}
/*  
gcc -g -fno-common -fno-strict-aliasing -rdynamic  -Wall -D_FILE_OFFSET_BITS=64 -std=gnu99 -D_REENTRANT -D_GNU_SOURCE  -D MARCH_x86_64 -march=native -msse4 -MMD -o ../obj/rbuffertest.o  -c -I../include -I../../cf/include -I../../xds/include rbuffertest.c 
gcc -L../lib/ $1 -lpthread -lcf -lcrypto ../obj/rbuffertest.o ../obj/rbuffer.o ../obj/fault.o ../obj/cf_random.o ../obj/dynbuf.o ../obj/cf_str.o ../obj/vector.o -o rbuffertest
*/
