#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_qiot_utils_md5.data.bss")
#pragma data_seg(".ble_qiot_utils_md5.data")
#pragma const_seg(".ble_qiot_utils_md5.text.const")
#pragma code_seg(".ble_qiot_utils_md5.text")
#endif
/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2018-2020 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_qiot_md5.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & LL_SYNC_EN)

/* Implementation that should never be optimized out by the compiler */
static void _utils_md5_zeroize(void *v, size_t n)
{
    volatile unsigned char *p = v;
    while (n--) {
        *p++ = 0;
    }
}

/*
 * 32-bit integer manipulation macros (little endian)
 */
#ifndef IOT_MD5_GET_UINT32_LE
#define IOT_MD5_GET_UINT32_LE(n, b, i)                                                                \
    {                                                                                                 \
        (n) = ((uint32_t)(b)[(i)]) | ((uint32_t)(b)[(i) + 1] << 8) | ((uint32_t)(b)[(i) + 2] << 16) | \
              ((uint32_t)(b)[(i) + 3] << 24);                                                         \
    }
#endif

#ifndef IOT_MD5_PUT_UINT32_LE
#define IOT_MD5_PUT_UINT32_LE(n, b, i)                      \
    {                                                       \
        (b)[(i)]     = (unsigned char)(((n)) & 0xFF);       \
        (b)[(i) + 1] = (unsigned char)(((n) >> 8) & 0xFF);  \
        (b)[(i) + 2] = (unsigned char)(((n) >> 16) & 0xFF); \
        (b)[(i) + 3] = (unsigned char)(((n) >> 24) & 0xFF); \
    }
#endif

void utils_md5_init(iot_md5_context *ctx)
{
    memset(ctx, 0, sizeof(iot_md5_context));
}

void utils_md5_free(iot_md5_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    _utils_md5_zeroize(ctx, sizeof(iot_md5_context));
}

void utils_md5_clone(iot_md5_context *dst, const iot_md5_context *src)
{
    *dst = *src;
}

/*
 * MD5 context setup
 */
void utils_md5_starts(iot_md5_context *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

void utils_md5_process(iot_md5_context *ctx, const unsigned char data[64])
{
    uint32_t X[16], A, B, C, D;

    IOT_MD5_GET_UINT32_LE(X[0], data, 0);
    IOT_MD5_GET_UINT32_LE(X[1], data, 4);
    IOT_MD5_GET_UINT32_LE(X[2], data, 8);
    IOT_MD5_GET_UINT32_LE(X[3], data, 12);
    IOT_MD5_GET_UINT32_LE(X[4], data, 16);
    IOT_MD5_GET_UINT32_LE(X[5], data, 20);
    IOT_MD5_GET_UINT32_LE(X[6], data, 24);
    IOT_MD5_GET_UINT32_LE(X[7], data, 28);
    IOT_MD5_GET_UINT32_LE(X[8], data, 32);
    IOT_MD5_GET_UINT32_LE(X[9], data, 36);
    IOT_MD5_GET_UINT32_LE(X[10], data, 40);
    IOT_MD5_GET_UINT32_LE(X[11], data, 44);
    IOT_MD5_GET_UINT32_LE(X[12], data, 48);
    IOT_MD5_GET_UINT32_LE(X[13], data, 52);
    IOT_MD5_GET_UINT32_LE(X[14], data, 56);
    IOT_MD5_GET_UINT32_LE(X[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a, b, c, d, k, s, t)      \
    {                               \
        a += F(b, c, d) + X[k] + t; \
        a = S(a, s) + b;            \
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];

#define F(x, y, z) (z ^ (x & (y ^ z)))

    P(A, B, C, D, 0, 7, 0xD76AA478);
    P(D, A, B, C, 1, 12, 0xE8C7B756);
    P(C, D, A, B, 2, 17, 0x242070DB);
    P(B, C, D, A, 3, 22, 0xC1BDCEEE);
    P(A, B, C, D, 4, 7, 0xF57C0FAF);
    P(D, A, B, C, 5, 12, 0x4787C62A);
    P(C, D, A, B, 6, 17, 0xA8304613);
    P(B, C, D, A, 7, 22, 0xFD469501);
    P(A, B, C, D, 8, 7, 0x698098D8);
    P(D, A, B, C, 9, 12, 0x8B44F7AF);
    P(C, D, A, B, 10, 17, 0xFFFF5BB1);
    P(B, C, D, A, 11, 22, 0x895CD7BE);
    P(A, B, C, D, 12, 7, 0x6B901122);
    P(D, A, B, C, 13, 12, 0xFD987193);
    P(C, D, A, B, 14, 17, 0xA679438E);
    P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x, y, z) (y ^ (z & (x ^ y)))

    P(A, B, C, D, 1, 5, 0xF61E2562);
    P(D, A, B, C, 6, 9, 0xC040B340);
    P(C, D, A, B, 11, 14, 0x265E5A51);
    P(B, C, D, A, 0, 20, 0xE9B6C7AA);
    P(A, B, C, D, 5, 5, 0xD62F105D);
    P(D, A, B, C, 10, 9, 0x02441453);
    P(C, D, A, B, 15, 14, 0xD8A1E681);
    P(B, C, D, A, 4, 20, 0xE7D3FBC8);
    P(A, B, C, D, 9, 5, 0x21E1CDE6);
    P(D, A, B, C, 14, 9, 0xC33707D6);
    P(C, D, A, B, 3, 14, 0xF4D50D87);
    P(B, C, D, A, 8, 20, 0x455A14ED);
    P(A, B, C, D, 13, 5, 0xA9E3E905);
    P(D, A, B, C, 2, 9, 0xFCEFA3F8);
    P(C, D, A, B, 7, 14, 0x676F02D9);
    P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x, y, z) (x ^ y ^ z)

    P(A, B, C, D, 5, 4, 0xFFFA3942);
    P(D, A, B, C, 8, 11, 0x8771F681);
    P(C, D, A, B, 11, 16, 0x6D9D6122);
    P(B, C, D, A, 14, 23, 0xFDE5380C);
    P(A, B, C, D, 1, 4, 0xA4BEEA44);
    P(D, A, B, C, 4, 11, 0x4BDECFA9);
    P(C, D, A, B, 7, 16, 0xF6BB4B60);
    P(B, C, D, A, 10, 23, 0xBEBFBC70);
    P(A, B, C, D, 13, 4, 0x289B7EC6);
    P(D, A, B, C, 0, 11, 0xEAA127FA);
    P(C, D, A, B, 3, 16, 0xD4EF3085);
    P(B, C, D, A, 6, 23, 0x04881D05);
    P(A, B, C, D, 9, 4, 0xD9D4D039);
    P(D, A, B, C, 12, 11, 0xE6DB99E5);
    P(C, D, A, B, 15, 16, 0x1FA27CF8);
    P(B, C, D, A, 2, 23, 0xC4AC5665);

#undef F

#define F(x, y, z) (y ^ (x | ~z))

    P(A, B, C, D, 0, 6, 0xF4292244);
    P(D, A, B, C, 7, 10, 0x432AFF97);
    P(C, D, A, B, 14, 15, 0xAB9423A7);
    P(B, C, D, A, 5, 21, 0xFC93A039);
    P(A, B, C, D, 12, 6, 0x655B59C3);
    P(D, A, B, C, 3, 10, 0x8F0CCC92);
    P(C, D, A, B, 10, 15, 0xFFEFF47D);
    P(B, C, D, A, 1, 21, 0x85845DD1);
    P(A, B, C, D, 8, 6, 0x6FA87E4F);
    P(D, A, B, C, 15, 10, 0xFE2CE6E0);
    P(C, D, A, B, 6, 15, 0xA3014314);
    P(B, C, D, A, 13, 21, 0x4E0811A1);
    P(A, B, C, D, 4, 6, 0xF7537E82);
    P(D, A, B, C, 11, 10, 0xBD3AF235);
    P(C, D, A, B, 2, 15, 0x2AD7D2BB);
    P(B, C, D, A, 9, 21, 0xEB86D391);

#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
}

/*
 * MD5 process buffer
 */
void utils_md5_update(iot_md5_context *ctx, const unsigned char *input, unsigned int ilen)
{
    uint32_t fill;
    uint32_t left;

    if (ilen == 0) {
        return;
    }

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t)ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (uint32_t)ilen) {
        ctx->total[1]++;
    }

    if (left && ilen >= fill) {
        memcpy((void *)(ctx->buffer + left), input, fill);
        utils_md5_process(ctx, ctx->buffer);
        input += fill;
        ilen -= fill;
        left = 0;
    }

    while (ilen >= 64) {
        utils_md5_process(ctx, input);
        input += 64;
        ilen -= 64;
    }

    if (ilen > 0) {
        memcpy((void *)(ctx->buffer + left), input, ilen);
    }
}

static const unsigned char iot_md5_padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                                 };

/*
 * MD5 final digest
 */
void utils_md5_finish(iot_md5_context *ctx, unsigned char output[16])
{
    uint32_t      last, padn;
    uint32_t      high, low;
    unsigned char msglen[8];

    high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
    low  = (ctx->total[0] << 3);

    IOT_MD5_PUT_UINT32_LE(low, msglen, 0);
    IOT_MD5_PUT_UINT32_LE(high, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    utils_md5_update(ctx, iot_md5_padding, padn);
    utils_md5_update(ctx, msglen, 8);

    IOT_MD5_PUT_UINT32_LE(ctx->state[0], output, 0);
    IOT_MD5_PUT_UINT32_LE(ctx->state[1], output, 4);
    IOT_MD5_PUT_UINT32_LE(ctx->state[2], output, 8);
    IOT_MD5_PUT_UINT32_LE(ctx->state[3], output, 12);
}

/*
 * output = MD5( input buffer )
 */
void utils_md5(const unsigned char *input, unsigned int ilen, unsigned char output[16])
{
    iot_md5_context ctx;

    utils_md5_init(&ctx);
    utils_md5_starts(&ctx);
    utils_md5_update(&ctx, input, ilen);
    utils_md5_finish(&ctx, output);
    utils_md5_free(&ctx);
}

#endif // (THIRD_PARTY_PROTOCOLS_SEL == LL_SYNC_EN)

#ifdef __cplusplus
}
#endif
