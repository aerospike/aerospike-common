#pragma once
#include "../cf_b64.h"

/******************************************************************************
 * ALIASES
 ******************************************************************************/

// Move these to a CF file at some point if they work
#define base64_validate_input cf_base64_validate_input
#define base64_encode_maxlen cf_base64_encode_maxlen
#define base64_encode cf_base64_encode
#define base64_tostring cf_base64_tostring
#define base64_decode_inplace cf_base64_decode_inplace
#define base64_decode cf_base64_decode
#define base64_test cf_base64_test