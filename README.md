# Aerospike Common

Library for commonly used or shared code.

## Build

aerospike-common currently builds both a static archive and a dynamic shared library.

To only build the static archive:

	$ make libaerospike-common.a

To only build the dynamic shared library:

	$ make libaerospike-common.so

Alternatively, you can use `all` target:

	$ make all

To clean up:

	$ make clean

To run tests:

	# make test

