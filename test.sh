#!/bin/bash

gcc -std=c99 -pthread -O2 -gdwarf-4 -Wall -Wextra \
	-D_GNU_SOURCE -DENHANCED_THREADS \
	-Itarget/Linux-x86_64/include \
	-otest \
	test.c \
	target/Linux-x86_64/lib/libaerospike-common.a

./test

