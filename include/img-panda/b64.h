/* *
 * This Base64 implimentation is adapted from NodeJS.
 * */

#ifndef FA_B64_H
#define FA_B64_H

#include <stddef.h>
#include <inttypes.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum imp_b64_mode {
    FA_B64_MODE_NORMAL,
    FA_B64_MODE_URL
} imp_b64_mode;

static const char base64_table[] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "abcdefghijklmnopqrstuvwxyz"
                                    "0123456789+/";

static const char base64_table_url[] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789-_";

static inline const char* base64_select_table (imp_b64_mode mode) {
    switch (mode) {
        case FA_B64_MODE_NORMAL: return base64_table;
        case FA_B64_MODE_URL: return base64_table_url;
        default: return base64_table;
    }
}

// Use FA_B64_MODE_NORMAL as mode default
static inline size_t base64_encoded_size (size_t size, imp_b64_mode mode) {
    return mode == FA_B64_MODE_NORMAL ? ((size + 2) / 3 * 4) : ceil((size * 4.0) / 3.0);
}

// Doesn't check for padding at the end.  Can be 1-2 bytes over.
static inline size_t base64_decoded_size_fast (size_t size) {
    // 1-byte input cannot be decoded
    return size > 1 ? (size / 4) * 3 + (size % 4 + 1) / 2 : 0;
}

uint32_t ReadUint32BE(const unsigned char* p);

size_t base64_decoded_size (const char* src, size_t size);

size_t base64_decode (
    char* const dst, const size_t dstlen,
    const char* const src, const size_t srclen
);

// Use FA_B64_MODE_NORMAL as mode default
size_t base64_encode (
    const char* src,
    size_t slen,
    char* dst,
    size_t dlen,
    imp_b64_mode mode
); 

#ifdef __cplusplus
}
#endif
#endif