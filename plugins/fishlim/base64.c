/*

  MIT License

  Copyright (c) 2013 Barry Steyn
  Copyright (c) 2019 <bakasura@protonmail.ch>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#include "base64.h"

#include <stdio.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdint.h>

static const char base64_chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

size_t calcDecodeLength(const char *b64input) { //Calculates the length of a decoded string
    size_t len = strlen(b64input),
            padding = 0;

    if (b64input[len - 1] == '=' && b64input[len - 2] == '=') //last two chars are =
        padding = 2;
    else if (b64input[len - 1] == '=') //last char is =
        padding = 1;

    return (len * 3) / 4 - padding;
}

int openssl_base64_encode(const unsigned char *buffer, size_t length, char **b64text) { //Encodes a binary safe base 64 string
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
    BIO_set_close(bio, BIO_CLOSE);
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    *b64text = (char *) g_malloc0((*bufferPtr).length + 1);
    memcpy(*b64text, (*bufferPtr).data, (*bufferPtr).length);

    BIO_free_all(bio);

    return (0); //success
}

int openssl_base64_decode(const char *b64message, unsigned char **buffer, size_t *length) { //Decodes a base64 encoded string
    BIO *bio = NULL, *b64 = NULL;
    int decodeLen = 0;

    if (strspn(b64message, base64_chars) != strlen(b64message))
        return -1;

    decodeLen = calcDecodeLength(b64message);

    if (decodeLen == 0)
        return -1;

    *buffer = (unsigned char *) malloc(decodeLen + 1);
    (*buffer)[decodeLen] = '\0';

    bio = BIO_new_mem_buf(b64message, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    *length = BIO_read(bio, *buffer, strlen(b64message));
    BIO_free_all(bio);

    if (*length == decodeLen) {
        return 0;
    } else {
        *length = 0;
        free(*buffer);
        return -1;
    }
}
