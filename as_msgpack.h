
#include <msgpack.h>
#include "as_types.h"


int as_msgpack_pack_boolean(msgpack_packer *, as_boolean *);
int as_msgpack_pack_integer(msgpack_packer *, as_integer *);
int as_msgpack_pack_string(msgpack_packer *, as_string *);
int as_msgpack_pack_list(msgpack_packer *, as_list *);
int as_msgpack_pack_map(msgpack_packer *, as_map *);
int as_msgpack_pack_rec(msgpack_packer *, as_rec *);
int as_msgpack_pack_pair(msgpack_packer *, as_pair *);

int as_msgpack_pack_val(msgpack_packer *, as_val *);
