/* ================ sha1.h ================ */
/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain

Last modified by Martin Chang for the Trantor project
*/

#pragma once

#include <stdint.h>

typedef struct
{
    uint32_t state[5];
    size_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void trantor_sha1_transform(uint32_t state[5], const unsigned char buffer[64]);
void trantor_sha1_init(SHA1_CTX* context);
void trantor_sha1_update(SHA1_CTX* context,
                         const unsigned char* data,
                         size_t len);
void trantor_sha1_final(unsigned char digest[20], SHA1_CTX* context);
