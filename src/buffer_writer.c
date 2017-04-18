// Copyright (c) 2015 Alexander Færøy. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <assert.h>
#include <string.h>
#include <arpa/inet.h>

#include "buffer_writer.h"
#include "memory.h"

#define MUSTCHECK __attribute__((warn_unused_result))

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

bool MUSTCHECK buffer_writer_write_value(buffer_writer_t *buffer_writer, const void *value, const size_t size)
{
    if (buffer_writer == NULL || buffer_writer->buffer == NULL
      || value == NULL || size == 0
      || NULL == buffer_writer->buffer->data)
        return false;

    if (buffer_writer->write_offset + size > buffer_writer->buffer->size)
        return false;

    memcpy(buffer_writer->buffer->data + buffer_writer->write_offset, value, size);
    buffer_writer->write_offset += size;

    return true;
}

bool MUSTCHECK buffer_writer_write_asciiz_with_linewrapping(buffer_writer_t *buffer_writer, const char *str, const size_t linewrapping)
{
  if(NULL == buffer_writer || NULL == buffer_writer->buffer
    || NULL == buffer_writer->buffer->data
    || NULL == str)
    return false;

  size_t max_including_zero = 1 + buffer_writer->buffer->size - buffer_writer->write_offset;
  size_t s_len = strnlen(str, max_including_zero);

  if(s_len == max_including_zero) // longer than available
    return false;

  size_t required_linebreaks = linewrapping ? (s_len / linewrapping) : 0;

  size_t offset = 0;

  while(required_linebreaks != 0){
     if(  !buffer_writer_write_value(buffer_writer, str + offset, linewrapping)
       || !buffer_writer_write_value(buffer_writer, "\n", 1))
     {
       // erase new half-added content, rewind the writer to original state
       size_t windback_offset = max_including_zero - 1 - buffer_writer->buffer->size;
       memory_zero(buffer_writer->buffer->data + windback_offset, buffer_writer->write_offset - windback_offset);
       buffer_writer_set_offset(buffer_writer, windback_offset);
       return false;
     }
     offset = offset + linewrapping;
     required_linebreaks -= 1;
  }

  return buffer_writer_write_value(buffer_writer, str + offset, s_len - offset);
}

bool MUSTCHECK buffer_writer_write_asciiz(buffer_writer_t *buffer_writer, const char *str)
{
  return buffer_writer_write_asciiz_with_linewrapping(buffer_writer, str, 0);
}

bool MUSTCHECK buffer_writer_write_uint8(buffer_writer_t *buffer_writer, uint8_t value)
{
    return buffer_writer_write_value(buffer_writer, &value, sizeof(value));
}

bool MUSTCHECK buffer_writer_write_uint16(buffer_writer_t *buffer_writer, uint16_t value)
{
    uint16_t v = htons(value);

    return buffer_writer_write_value(buffer_writer, &v, sizeof(v));
}

bool MUSTCHECK buffer_writer_write_uint32(buffer_writer_t *buffer_writer, uint32_t value)
{
    uint32_t v = htonl(value);

    return buffer_writer_write_value(buffer_writer, &v, sizeof(v));
}
