// Copyright (c) 2015 Alexander Færøy. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GUARD_BUFFER_WRITER_H
#define GUARD_BUFFER_WRITER_H 1

#include <stdint.h>

#include "buffer.h"

struct buffer_writer
{
    size_t write_offset;
    struct buffer *buffer;
};

typedef struct buffer_writer buffer_writer_t;

buffer_writer_t* buffer_writer_new(struct buffer *buffer);

void buffer_writer_free(buffer_writer_t *buffer_writer);

void buffer_writer_reset(buffer_writer_t *buffer_writer);
void buffer_writer_set_offset(buffer_writer_t *buffer_writer, size_t offset);

bool buffer_writer_write_value(buffer_writer_t *buffer_writer, const void *value, size_t size);

bool buffer_writer_write_asciiz(buffer_writer_t *buffer_writer, const char *str);
bool buffer_writer_write_asciiz_with_linewrapping(buffer_writer_t *buffer_writer, const char *str, const size_t linewrapping);

bool buffer_writer_write_uint8(buffer_writer_t *buffer_writer, uint8_t value);
bool buffer_writer_write_uint16(buffer_writer_t *buffer_writer, uint16_t value);
bool buffer_writer_write_uint32(buffer_writer_t *buffer_writer, uint32_t value);

#endif
