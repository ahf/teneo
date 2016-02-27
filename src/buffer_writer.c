// Copyright (c) 2015 Alexander Færøy. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <assert.h>
#include <string.h>
#include <arpa/inet.h>

#include "buffer_writer.h"
#include "memory.h"

buffer_writer_t* buffer_writer_new(struct buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    buffer_writer_t *buffer_writer = NULL;

    buffer_writer = malloc(sizeof(*buffer_writer));
    assert(buffer_writer != NULL);

    buffer_writer->write_offset = 0;
    buffer_writer->buffer = buffer;

    return buffer_writer;
}

void buffer_writer_free(buffer_writer_t *buffer_writer)
{
    free(buffer_writer);
}

void buffer_writer_reset(buffer_writer_t *buffer_writer)
{
    buffer_writer_set_offset(buffer_writer, 0);
}

void buffer_writer_set_offset(buffer_writer_t *buffer_writer, size_t offset)
{
    if (buffer_writer == NULL)
        return;

    buffer_writer->write_offset = offset;
}

bool buffer_writer_write_value(buffer_writer_t *buffer_writer, const void *value, size_t size)
{
    if (buffer_writer == NULL || buffer_writer->buffer == NULL || value == NULL || size == 0)
        return false;

    if (buffer_writer->write_offset + size > buffer_writer->buffer->size)
        return false;

    memcpy(buffer_writer->buffer->data + buffer_writer->write_offset, value, size);
    buffer_writer->write_offset += size;

    return true;
}

bool buffer_writer_write_uint8(buffer_writer_t *buffer_writer, uint8_t value)
{
    return buffer_writer_write_value(buffer_writer, &value, sizeof(value));
}

bool buffer_writer_write_uint16(buffer_writer_t *buffer_writer, uint16_t value)
{
    uint16_t v = htons(value);

    return buffer_writer_write_value(buffer_writer, &v, sizeof(v));
}

bool buffer_writer_write_uint32(buffer_writer_t *buffer_writer, uint32_t value)
{
    uint32_t v = htonl(value);

    return buffer_writer_write_value(buffer_writer, &v, sizeof(v));
}
