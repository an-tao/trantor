/*********************************************************************
 * Filename:   md5.h
 * Author:     Brad Conte (brad AT bradconte.com)
 * Copyright:
 * Disclaimer: This code is presented "as is" without any guarantees.
 * Details:    Defines the API for the corresponding MD5 implementation.
 *********************************************************************/

#ifndef MD5_H
#define MD5_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define MD5_BLOCK_SIZE 32  // MD5 outputs a 32 byte digest

typedef struct
{
    unsigned char data[64];
    unsigned int datalen;
    unsigned long long bitlen;
    unsigned int state[4];
} MD5_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void trantor_md5_init(MD5_CTX *ctx);
void trantor_md5_update(MD5_CTX *ctx, const unsigned char data[], size_t len);
void trantor_md5_final(MD5_CTX *ctx, unsigned char hash[]);

#endif  // MD5_H