// Copyright (c) 2015 Alexander Færøy. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <sodium.h>

#include "buffer.h"
#include "openssh.h"
#include "profile.h"
#include "readpassphrase.h"
#include "memory.h"

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s [ options ] [ output directory ]\n", program);
    fprintf(stderr, "\n");

    fprintf(stderr, "Help Options:\n");
    fprintf(stderr, "  -h, --help                Show help options\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "Key Options:\n");
    fprintf(stderr, "  -u, --user <username>     Specify which username to use\n");
    fprintf(stderr, "  -p, --profile <profile>   Specify which profile to use\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Available Profiles:\n");
    fprintf(stderr, "      2015v1\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "Output Format Options:\n");
    fprintf(stderr, "  -s, --openssh             Output OpenSSH public and private key\n");
    fprintf(stderr, "\n");
}

static struct option options[] = {
    // Output Formats.
    {"openssh", no_argument, NULL, 's'},

    // Key Options.
    {"profile", required_argument, NULL, 'p'},
    {"user", required_argument, NULL, 'u'},

    // Help.
    {"help", no_argument, NULL, 'h'}
};

int main(int argc, char *argv[])
{
    char option;
    bool success = true;

    // Output formats.
    bool ssh_output = false;
    bool gpg_output = false;

    // Key options.
    char *profile_name = DEFAULT_PROFILE;

    char *username = NULL;
    size_t username_length = 0;

    // Output directory.
    char *output_directory = NULL;

    // Passphrase.
    char passphrase[1024];
    char passphrase_verify[sizeof passphrase];

    // Initialize base64 encoder and decoder.
    buffer_init();

    while ((option = getopt_long(argc, argv, "shu:p:", options, NULL)) != -1)
    {
        switch (option)
        {
            case 's':
                ssh_output = true;
                break;

            case 'g':
                gpg_output = true;
                break;

            case 'p':
                profile_name = optarg;
                break;

            case 'u':
                username = optarg;
                username_length = strlen(username);
                break;

            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        };
    }

    if (argv[optind] != NULL)
    {
        output_directory = argv[optind];
    }

    // Username is required.
    if (username == NULL)
    {
        fprintf(stderr, "Error: Missing username option ...\n");
        return EXIT_FAILURE;
    }

    // We want at least one output format.
    if (! (ssh_output || gpg_output))
    {
        fprintf(stderr, "Error: Missing output format(s) ...\n");
        return EXIT_FAILURE;
    }

    if (! is_valid_profile_name(profile_name))
    {
        fprintf(stderr, "Error: Invalid profile '%s' ...\n", profile_name);
        return EXIT_FAILURE;
    }

    if (sodium_init() == -1)
    {
        fprintf(stderr, "Error: Unable to initialize libsodium ...\n");
        return EXIT_FAILURE;
    }

    sodium_mlock(passphrase, sizeof(passphrase));
    sodium_mlock(passphrase_verify, sizeof(passphrase_verify));
    memory_zero(passphrase, sizeof passphrase);
    memory_zero(passphrase_verify, sizeof passphrase);

    if(NULL == readpassphrase("Enter passphrase: ", passphrase, sizeof(passphrase), RPP_ECHO_OFF)
    || NULL == readpassphrase("Confirm passphrase: ", passphrase_verify, sizeof(passphrase), RPP_ECHO_OFF))
    {
        fprintf(stderr, "Error: Program failed to read passphrases.\n");
        success = false;
        goto cleanup_passphrase_and_exit;
    }

    if(0 != strncmp(passphrase, passphrase_verify, sizeof(passphrase_verify)))
    {
        fprintf(stderr, "Error: Passphrases do not match.\n");
        success = false;
        goto cleanup_passphrase_and_exit;
    }

    if(strlen(passphrase) < 10)
    {
        fprintf(stderr, "Error: Provided passphrase is shorter than 10 characters.\n");
        success = false;
        goto cleanup_passphrase_and_exit;
    }

    unsigned char public[crypto_sign_PUBLICKEYBYTES];
    unsigned char secret[crypto_sign_SECRETKEYBYTES];

    sodium_mlock(public, sizeof(public));
    sodium_mlock(secret, sizeof(secret));
    sodium_mlock(username, username_length);

    // Avoid randomness here.
    sodium_memzero(public, sizeof(public));
    sodium_memzero(secret, sizeof(secret));

    printf("Generating key pair using the '%s' profile ...\n", profile_name);
    printf("This may take a little while ...\n");

    if (generate_keypair(profile_name, username, username_length, passphrase, strlen(passphrase), secret, public))
    {
        printf("Succesfully generated key pair ...\n");

        if (ssh_output)
        {
            openssh_write(output_directory, username, username_length, secret, public);
        }
    }
    else
    {
        fprintf(stderr, "Error: Unable to generate key pair ...\n");
        success = false;
    }

    // Zeros out the buffers as well.
    sodium_munlock(public, sizeof(public));
    sodium_munlock(secret, sizeof(secret));
    sodium_munlock(username, username_length);

cleanup_passphrase_and_exit:
    sodium_munlock(passphrase, sizeof(passphrase));
    sodium_munlock(passphrase_verify, sizeof(passphrase_verify));

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
