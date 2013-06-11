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

#include <aerospike/as_val.h>

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/**
 * Global NIL value
 */
extern const as_val as_nil;

/******************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * PRIVATE:
 * Internal helper function for destroying an as_val.
 */
void as_nil_val_destroy(as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the hashcode of an as_val.
 */
uint32_t as_nil_val_hashcode(const as_val * v);

/**
 * PRIVATE:
 * Internal helper function for getting the string representation of an as_val.
 */
char * as_nil_val_tostring(const as_val * v);
