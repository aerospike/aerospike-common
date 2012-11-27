/*
 * cf/include/hist_track.h
 *
 * A histogram with cached data.
 *
 * Citrusleaf, 2012.
 * All rights reserved.
 */

#pragma once


//==========================================================
// Includes
//

#include <stdbool.h>
#include <stdint.h>
#include "dynbuf.h"


//==========================================================
// Typedefs
//

typedef struct cf_hist_track_s cf_hist_track;

typedef enum {
	CF_HIST_TRACK_FMT_PACKED,
	CF_HIST_TRACK_FMT_TABLE
} cf_hist_track_info_format;


//==========================================================
// Public API
//

//------------------------------------------------
// Constructor/Destructor
//
cf_hist_track* cf_hist_track_create(const char* name);
void cf_hist_track_destroy(cf_hist_track* this);

//------------------------------------------------
// Start/Stop Caching Data
//
bool cf_hist_track_start(cf_hist_track* this, uint32_t back_sec,
		uint32_t slice_sec, const char* thresholds);
void cf_hist_track_stop(cf_hist_track* this);

//------------------------------------------------
// Histogram API "Overrides"
//
void cf_hist_track_clear(cf_hist_track* this);
void cf_hist_track_dump(cf_hist_track* this);

// This is just a pass-through to histogram_insert_data_point():
void cf_hist_track_insert_data_point(cf_hist_track* this, uint64_t start_time);

//------------------------------------------------
// Get Statistics from Cached Data
//
void cf_hist_track_get_info(cf_hist_track* this, uint32_t back_sec,
		uint32_t duration_sec, uint32_t slice_sec, bool throughput_only,
		cf_hist_track_info_format info_fmt, cf_dyn_buf* db_p);

//------------------------------------------------
// Get Current Settings
//
void cf_hist_track_get_settings(cf_hist_track* this, cf_dyn_buf* db_p);
