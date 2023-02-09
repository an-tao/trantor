/* ================ sha1.h ================ */
/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain

Last modified by Martin Chang for the Trantor project
*/

#pragma once

typedef struct
{
    u_int32_t state[5];
    u_int32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void TrantorSHA1Transform(u_int32_t state[5], const unsigned char buffer[64]);
void TrantorSHA1Init(SHA1_CTX* context);
void TrantorSHA1Update(SHA1_CTX* context,
                       const unsigned char* data,
                       u_int32_t len);
void TrantorSHA1Final(unsigned char digest[20], SHA1_CTX* context);
