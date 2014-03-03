/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#if defined(__linux__)

#include <netinet/in.h>
#include <asm/byteorder.h>

#define cf_swap_to_be16(_n) __cpu_to_be16(_n)
#define cf_swap_from_be16(_n) __be16_to_cpu(_n)

#define cf_swap_to_be32(_n) __cpu_to_be32(_n)
#define cf_swap_from_be32(_n) __be32_to_cpu(_n)

#define cf_swap_to_be64(_n) __cpu_to_be64(_n)
#define cf_swap_from_be64(_n) __be64_to_cpu(_n)

#endif // __linux__

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#include <arpa/inet.h>

#define cf_swap_to_be16(_n) OSSwapHostToBigInt16(_n)
#define cf_swap_from_be16(_n) OSSwapBigToHostInt16(_n)

#define cf_swap_to_be32(_n) OSSwapHostToBigInt32(_n)
#define cf_swap_from_be32(_n) OSSwapBigToHostInt32(_n)

#define cf_swap_to_be64(_n) OSSwapHostToBigInt64(_n)
#define cf_swap_from_be64(_n) OSSwapBigToHostInt64(_n)

#endif // __APPLE__

#if defined(CF_WINDOWS)
#include <stdint.h>
#include <stdlib.h>
#include <WinSock2.h>

#define cf_swap_to_be16(_n) _byteswap_uint16(_n)
#define cf_swap_from_be16(_n) _byteswap_uint16(_n)

#define cf_swap_to_be32(_n) _byteswap_uint32(_n)
#define cf_swap_from_be32(_n) _byteswap_uint32(_n)

#define cf_swap_to_be64(_n) _byteswap_uint64(_n)
#define cf_swap_from_be64(_n) _byteswap_uint64(_n)
#endif // CF_WINDOWS
