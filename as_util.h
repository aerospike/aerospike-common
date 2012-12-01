#pragma once

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define as_util_hook(hook,default,object,args...) \
    (object && object->hooks && object->hooks->hook ? object->hooks->hook(object, ## args) : default)

#define as_util_fromval(object,type_id,type) \
    (as_val_type(object) == type_id ? (type *) object : NULL)
