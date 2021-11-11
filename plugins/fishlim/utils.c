/*

  Copyright (c) 2020 <bakasura@protonmail.ch>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

*/

#include <string.h>

#include "utils.h"
#include "fish.h"

/**
 * Calculate the length of Base64-encoded string
 *
 * @param plaintext_len Size of clear text to encode
 * @return Size of encoded string
 */
unsigned long base64_len(size_t plaintext_len) {
    int length_unpadded = (4 * plaintext_len) / 3;
    /* Add padding */
    return length_unpadded % 4 != 0 ? length_unpadded + (4 - length_unpadded % 4) : length_unpadded;
}

/**
 * Calculate the length of BlowcryptBase64-encoded string
 *
 * @param plaintext_len Size of clear text to encode
 * @return Size of encoded string
 */
unsigned long base64_fish_len(size_t plaintext_len) {
    int length_unpadded = (12 * plaintext_len) / 8;
    /* Add padding */
    return length_unpadded % 12 != 0 ? length_unpadded + (12 - length_unpadded % 12) : length_unpadded;
}

/**
 * Calculate the length of fish-encrypted string in CBC mode
 *
 * @param plaintext_len  Size of clear text to encode
 * @return Size of encoded string
 */
unsigned long cbc_len(size_t plaintext_len) {
    /*IV + DATA + Zero Padding */
    return base64_len(8 + (plaintext_len % 8 != 0 ? plaintext_len + 8 - (plaintext_len % 8) : plaintext_len));
}

/**
 * Calculate the length of fish-encrypted string in ECB mode
 *
 * @param plaintext_len  Size of clear text to encode
 * @return Size of encoded string
 */
unsigned long ecb_len(size_t plaintext_len) {
    return base64_fish_len(plaintext_len);
}

/**
 * Calculate the length of encrypted string in 'mode' mode
 *
 * @param plaintext_len Length of plaintext
 * @param mode          Encryption mode
 * @return Size of encoded string
 */
unsigned long encoded_len(size_t plaintext_len, enum fish_mode mode) {
    switch (mode) {

        case FISH_CBC_MODE:
            return cbc_len(plaintext_len);
            break;

        case FISH_ECB_MODE:
            return ecb_len(plaintext_len);
    }

    return 0;
}

/**
 * Determine the maximum length of plaintext for a 'max_len' limit taking care the overload of encryption
 *
 * @param max_len   Limit for plaintext
 * @param mode      Encryption mode
 * @return Maximum allowed plaintext length
 */
int max_text_command_len(size_t max_len, enum fish_mode mode) {
    int len;

    for (len = max_len; encoded_len(len, mode) > max_len; --len);
    return len;
}

/**
 * Iterate over 'data' in chunks of 'max_chunk_len' taking care the UTF-8 characters
 *
 * @param data              Data to iterate
 * @param max_chunk_len     Size of biggest chunk
 * @param [out] chunk_len   Current chunk length
 * @return Pointer to current chunk position or NULL if not have more chunks
 */
const char *foreach_utf8_data_chunks(const char *data, int max_chunk_len, int *chunk_len) {
    int data_len, last_chunk_len = 0;

    if (!*data) {
        return NULL;
    }

    /* Last chunk of data */
    data_len = strlen(data);
    if (data_len <= max_chunk_len) {
        *chunk_len = data_len;
        return data;
    }

    *chunk_len = 0;
    const char *utf8_character = data;

    /* Not valid UTF-8, but maybe valid text, just split into max length */
    if (!g_utf8_validate(data, -1, NULL)) {
        *chunk_len = max_chunk_len;
        return utf8_character;
    }

    while (*utf8_character && *chunk_len <= max_chunk_len) {
        last_chunk_len = *chunk_len;
        *chunk_len = (g_utf8_next_char(utf8_character) - data) * sizeof(*utf8_character);
        utf8_character = g_utf8_next_char(utf8_character);
    }

    /* We need the previous length before overflow the limit */
    *chunk_len = last_chunk_len;

    return utf8_character;
}