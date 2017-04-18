// Copyright (c) 2015 Alexander Færøy. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "buffer.h"
#include "buffer_writer.h"
#include "openssh.h"

#include <sodium.h>

static bool openssh_write_public(const char *output_directory, const char *username, size_t username_length, unsigned char *public)
{
    // Public key file.
    char *public_file_path = NULL;
    int fd = 0;

    if (output_directory != NULL)
    {
        size_t public_file_path_len = strlen(output_directory) + 16;
        public_file_path = malloc(public_file_path_len);
        snprintf(public_file_path, public_file_path_len, "%s/id_ed25519.pub", output_directory);
    }
    else
        public_file_path = strdup("id_ed25519.pub");

    printf("Saving OpenSSH public key to %s ...\n", public_file_path);

    fd = open(public_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (fd == -1)
    {
        fprintf(stderr, "Error: Unable to write public key (%s)\n", strerror(errno));
        free(public_file_path);
        return false;
    }

    struct buffer *body = buffer_new(51);
    struct buffer *body_base64 = NULL;
    struct buffer_writer *body_writer = buffer_writer_new(body);

    buffer_writer_write_uint32(body_writer, 11);
    buffer_writer_write_value(body_writer, "ssh-ed25519", 11);
    buffer_writer_write_uint32(body_writer, 32);
    buffer_writer_write_value(body_writer, public, 32);

    buffer_base64_encode(body, &body_base64);

    struct buffer * final_buf = buffer_new(12 + body_base64->size + 1 + username_length + 1);
    struct buffer_writer * final_writer = buffer_writer_new(final_buf);

    buffer_writer_write_asciiz(final_writer, "ssh-ed25519 ");
    buffer_writer_write_value(final_writer, body_base64->data, body_base64->size);
    buffer_writer_write_asciiz(final_writer, " ");
    buffer_writer_write_value(final_writer, username, username_length);
    buffer_writer_write_asciiz(final_writer, "\n");

    buffer_free(body);
    buffer_free(body_base64);
    buffer_writer_free(body_writer);

    const char * final_str = buffer_string(final_buf);
    if(write(fd, final_str, strlen(final_str)) != (ssize_t) strlen(final_str))
    {
       fprintf(stderr, "Error: Unable to write public key file (%s)\n", strerror(errno));
       return false;
    }

    buffer_free(final_buf);
    buffer_writer_free(final_writer);

    if (close(fd))
    {
        fprintf(stderr, "Error: Unable to close public key file (%s)\n", strerror(errno));
        free(public_file_path);
        return false;
    }

    free(public_file_path);

    return true;
}

static bool openssh_write_secret(const char *output_directory, const char *username, size_t username_length, unsigned char *public, unsigned char *secret)
{
    char *secret_file_path = NULL;
    int fd = 0;

    if (output_directory != NULL)
    {
        size_t secret_file_path_len = strlen(output_directory) + 12;
        secret_file_path = malloc(secret_file_path_len);
        snprintf(secret_file_path, secret_file_path_len, "%s/id_ed25519", output_directory);
    }
    else
        secret_file_path = strdup("id_ed25519");

    printf("Saving OpenSSH secret key to %s ...\n", secret_file_path);

    fd = open(secret_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (fd == -1)
    {
        fprintf(stderr, "Error: Unable to write secret key (%s)\n", strerror(errno));
        free(secret_file_path);
        return false;
    }

    uint32_t private_length = (4 + 4) + (4 + 11 + 4 + 32) + (4 + 64) + (4 + username_length);
    uint32_t padding = private_length % 8;

    if (padding != 0)
        padding = 8 - padding;

    struct buffer *body = buffer_new(15 + (4 + 4) + (4 + 4) + 4 + 4 + 4 + (4 + 11 + 4 + 32) + 4 + private_length + padding);
    struct buffer *body_base64 = NULL;
    struct buffer_writer *body_writer = buffer_writer_new(body);

    // Header.
    buffer_writer_write_value(body_writer, "openssh-key-v1\x00", 15);

    // Cipher.
    buffer_writer_write_uint32(body_writer, 4);
    buffer_writer_write_value(body_writer, "none", 4);

    // KDF.
    buffer_writer_write_uint32(body_writer, 4);
    buffer_writer_write_value(body_writer, "none", 4);

    // KDF Options.
    buffer_writer_write_uint32(body_writer, 0);

    // Number of Public Keys
    buffer_writer_write_uint32(body_writer, 1);

    // Public Keys Length.
    buffer_writer_write_uint32(body_writer, 51);

    // Public Key.
    buffer_writer_write_uint32(body_writer, 11);
    buffer_writer_write_value(body_writer, "ssh-ed25519", 11);
    buffer_writer_write_uint32(body_writer, 32);
    buffer_writer_write_value(body_writer, public, 32);

    // Private key section
    uint32_t checkint = randombytes_random();

    buffer_writer_write_uint32(body_writer, private_length + padding);

    buffer_writer_write_uint32(body_writer, checkint);
    buffer_writer_write_uint32(body_writer, checkint);

    buffer_writer_write_uint32(body_writer, 11);
    buffer_writer_write_value(body_writer, "ssh-ed25519", 11);
    buffer_writer_write_uint32(body_writer, 32);
    buffer_writer_write_value(body_writer, public, 32);

    buffer_writer_write_uint32(body_writer, 64);
    buffer_writer_write_value(body_writer, secret, 64);

    buffer_writer_write_uint32(body_writer, username_length);
    buffer_writer_write_value(body_writer, username, username_length);

    uint8_t pad = 0;
    while (padding--)
        buffer_writer_write_uint8(body_writer, ++pad & 0xff);

    buffer_writer_free(body_writer);

    // BASE64 encode the buffer.
    buffer_base64_encode(body, &body_base64);


    struct buffer * fd_buf = buffer_new(
      36   // "BEGIN OPENSSH PRIVATE KEY..."
      + body_base64->size
      + (body_base64->size / 70) + (body_base64->size % 70) // newlines
      + 34 // "END OPENSSH PRIVATE KEY..."
    );
    struct buffer_writer * fd_writer = buffer_writer_new(fd_buf);

    buffer_writer_write_asciiz(fd_writer, "-----BEGIN OPENSSH PRIVATE KEY-----\n");
    buffer_writer_write_asciiz_with_linewrapping(fd_writer, (char *) body_base64->data, 70);
    buffer_writer_write_asciiz(fd_writer, "\n");
    buffer_writer_write_asciiz(fd_writer, "-----END OPENSSH PRIVATE KEY-----\n");

    buffer_writer_free(fd_writer);

    const char * fd_str = buffer_string(fd_buf);
    if(write(fd, fd_str, strlen(fd_str)) != (ssize_t) strlen(fd_str))
    {
       fprintf(stderr, "Error: Unable to write private key file (%s)\n", strerror(errno));
       return false;
    }

    close(fd);

    buffer_free(body);
    buffer_free(body_base64);
    buffer_free(fd_buf);

    free(secret_file_path);

    return true;
}

bool openssh_write(const char *output_directory, const char *username, size_t username_length, unsigned char *secret, unsigned char *public)
{
    if (! openssh_write_secret(output_directory, username, username_length, public, secret))
        return false;

    if (! openssh_write_public(output_directory, username, username_length, public))
        return false;

    return true;
}
