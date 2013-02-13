/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

/**
 * A general purpose hashtable implementation
 * Good at multithreading
 * Just, hopefully, the last reasonable hash table you'll ever need
 */
#include "cf_crypto.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <openssl/sha.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * sha1_to_hex
 * Desc: Convert a sha1 returned hash to a hexadecimal string. 
 */
bool sha1_to_hex(unsigned char * hash, unsigned char * sha1_hex_buff) { 
        if (!sha1_hex_buff || !hash) return false; 
        for (unsigned int i = 0; i < SHA_DIGEST_LENGTH; i++) { 
                sprintf((char *)(sha1_hex_buff + (i * 2)), "%02x", hash[i]); 
        } 
        return true; 
}

