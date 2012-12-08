/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>

extern int cf_socket_read_timeout(int fd, uint8_t *buf, size_t buf_len, uint64_t trans_deadline, int attempt_ms);
extern int cf_socket_write_timeout(int fd, uint8_t *buf, size_t buf_len, uint64_t trans_deadline, int attempt_ms);
extern int cf_socket_read_forever(int fd, uint8_t *buf, size_t buf_len);
extern int cf_socket_write_forever(int fd, uint8_t *buf, size_t buf_len);
extern int cf_create_nb_socket(struct sockaddr_in *sa, int timeout);
