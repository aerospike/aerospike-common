/*
 * Copyright 2008-2018 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <citrusleaf/cf_clock.h>

#if !defined(_MSC_VER)

bool
cf_clock_init()
{
	return true;
}

#else

double cf_clock_freq = 0.0;
int64_t cf_clock_start = 0;
uint64_t cf_wall_clock_start = 0;

static bool
clock_init()
{
	LARGE_INTEGER li;

	if (!QueryPerformanceFrequency(&li)) {
		return false;
	}

	cf_clock_freq = (double)li.QuadPart / (1000.0 * 1000.0 * 1000.0);
	QueryPerformanceCounter(&li);
	cf_clock_start = li.QuadPart;
	return true;
}

static void
wall_clock_init()
{
	SYSTEMTIME s;
	FILETIME f;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);

	cf_wall_clock_start = ((uint64_t)f.dwHighDateTime << 32) + (uint64_t)f.dwLowDateTime;
}

bool
cf_clock_init()
{
	wall_clock_init();
	return clock_init();
}
#endif
