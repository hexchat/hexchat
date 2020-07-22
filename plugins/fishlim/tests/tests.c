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

#ifndef PLUGIN_HEXCHAT_FISHLIM_TEST_H
#define PLUGIN_HEXCHAT_FISHLIM_TEST_H

// Libs
#include <glib.h>
// Project Libs
#include "../fish.h"
#include "../utils.h"
#include "old_version/fish.h"

/**
 * Auxiliary function: Generate a random string
 * @param out Preallocated string to fill
 * @param len Size of bytes to fill
 */
void random_string(char *out, size_t len) {
    GRand *rand = NULL;
    int i = 0;

    rand = g_rand_new();
    for (i = 0; i < len; ++i) {
        out[i] = g_rand_int_range(rand, 1, 256);
    }

    out[len] = 0;

    g_rand_free(rand);
}


/**
 * Check encrypt and decrypt in ECB mode and compare with old implementation
 */
void __ecb(void) {
    char *bo64 = NULL, *b64 = NULL;
    char *deo = NULL, *de = NULL;
    int key_len, message_len = 0;
    char key[57];
    char message[1000];

    /* Generate key 32–448 bits (Yes, I start with 8 bits) */
    for (key_len = 1; key_len < 57; ++key_len) {

        random_string(key, key_len);

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);

            /* Encrypt */
            bo64 = __old_fish_encrypt(key, key_len, message);
            g_assert_nonnull(bo64);
            b64 = fish_encrypt(key, key_len, message, message_len, FISH_ECB_MODE);
            g_assert_nonnull(b64);
            g_assert_cmpuint(g_strcmp0(b64, bo64), == , 0);

            /* Decrypt */
            /* Linear */
            deo = __old_fish_decrypt(key, key_len, bo64);
            de = fish_decrypt_str(key, key_len, b64, FISH_ECB_MODE);
            g_assert_nonnull(deo);
            g_assert_nonnull(de);
            g_assert_cmpuint(g_strcmp0(de, message), == , 0);
            g_assert_cmpuint(g_strcmp0(deo, message), == , 0);
            g_assert_cmpuint(g_strcmp0(de, deo), == , 0);
            g_free(deo);
            g_free(de);
            /* Mixed */
            deo = __old_fish_decrypt(key, key_len, b64);
            de = fish_decrypt_str(key, key_len, bo64, FISH_ECB_MODE);
            g_assert_nonnull(deo);
            g_assert_nonnull(de);
            g_assert_cmpuint(g_strcmp0(de, message), == , 0);
            g_assert_cmpuint(g_strcmp0(deo, message), == , 0);
            g_assert_cmpuint(g_strcmp0(de, deo), == , 0);
            g_free(deo);
            g_free(de);

            /* Free */
            g_free(bo64);
            g_free(b64);
        }
    }
}

/**
 * Check encrypt and decrypt in CBC mode
 */
void __cbc(void) {
    char *b64 = NULL;
    char *de = NULL;
    int key_len, message_len = 0;
    char key[57];
    char message[1000];

    /* Generate key 32–448 bits (Yes, I start with 8 bits) */
    for (key_len = 1; key_len < 57; ++key_len) {

        random_string(key, key_len);

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);

            /* Encrypt */
            b64 = fish_encrypt(key, key_len, message, message_len, FISH_CBC_MODE);
            g_assert_nonnull(b64);

            /* Decrypt */
            /* Linear */
            de = fish_decrypt_str(key, key_len, b64, FISH_CBC_MODE);
            g_assert_nonnull(de);
            g_assert_cmpuint(g_strcmp0(de, message), == , 0);
            g_free(de);

            /* Free */
            g_free(b64);
        }
    }
}

/**
 * Check the calculation of final length from an encoded string in Base64
 */
void __base64_len(void) {
    char *b64 = NULL;
    int i, message_len = 0;
    char message[1000];

    for (i = 0; i < 10; ++i) {

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = g_base64_encode((const unsigned char *) message, message_len);
            g_assert_nonnull(b64);
            g_assert_cmpuint(strlen(b64), == , base64_len(message_len));
            g_free(b64);
        }
    }
}

/**
 * Check the calculation of final length from an encoded string in BlowcryptBase64
 */
void __base64_fish_len(void) {
    char *b64 = NULL;
    int i, message_len = 0;
    char message[1000];

    for (i = 0; i < 10; ++i) {

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = fish_base64_encode(message, message_len);
            g_assert_nonnull(b64);
            g_assert_cmpuint(strlen(b64), == , base64_fish_len(message_len));
            g_free(b64);
        }
    }
}


/**
 * Check the calculation of final length from an encrypted string in ECB mode
 */
void __base64_ecb_len(void) {
    char *b64 = NULL;
    int key_len, message_len = 0;
    char key[57];
    char message[1000];

    /* Generate key 32–448 bits (Yes, I start with 8 bits) */
    for (key_len = 1; key_len < 57; ++key_len) {

        random_string(key, key_len);

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = fish_encrypt(key, key_len, message, message_len, FISH_ECB_MODE);
            g_assert_nonnull(b64);
            g_assert_cmpuint(strlen(b64), == , ecb_len(message_len));
            g_free(b64);
        }
    }
}

/**
 * Check the calculation of final length from an encrypted string in CBC mode
 */
void __base64_cbc_len(void) {
    char *b64 = NULL;
    int key_len, message_len = 0;
    char key[57];
    char message[1000];

    /* Generate key 32–448 bits (Yes, I start with 8 bits) */
    for (key_len = 1; key_len < 57; ++key_len) {

        random_string(key, key_len);

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = fish_encrypt(key, key_len, message, message_len, FISH_CBC_MODE);
            g_assert_nonnull(b64);
            g_assert_cmpuint(strlen(b64), == , cbc_len(message_len));
            g_free(b64);
        }
    }
}

/**
 * Check the calculation of length limit for a plaintext in each encryption mode
 */
void __max_text_command_len(void) {
    int max_encoded_len, plaintext_len;
    enum fish_mode mode;

    for (max_encoded_len = 0; max_encoded_len < 10000; ++max_encoded_len) {
        for (mode = FISH_ECB_MODE; mode <= FISH_CBC_MODE; ++mode) {
            plaintext_len = max_text_command_len(max_encoded_len, mode);
            g_assert_cmpuint(encoded_len(plaintext_len, mode), <= , max_encoded_len);
        }
    }
}

/**
 * Check the calculation of length limit for a plaintext in each encryption mode
 */
void __foreach_utf8_data_chunks(void) {
    GRand *rand = NULL;
    GString *chunks = NULL;
    int tests, max_chunks_len, chunks_len;
    char ascii_message[1001];
    char *data_chunk = NULL;

    rand = g_rand_new();

    for (tests = 0; tests < 1000; ++tests) {

        max_chunks_len = g_rand_int_range(rand, 2, 301);
        random_string(ascii_message, 1000);

        data_chunk = ascii_message;

        chunks = g_string_new(NULL);

        while (foreach_utf8_data_chunks(data_chunk, max_chunks_len, &chunks_len)) {
            g_string_append(chunks, g_strndup(data_chunk, chunks_len));
            /* Next chunk */
            data_chunk += chunks_len;
        }
        /* Check data loss */
        g_assert_cmpstr(chunks->str, == , ascii_message);
        g_string_free(chunks, TRUE);
    }
}


int main(int argc, char *argv[]) {

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/fishlim/__ecb", __ecb);
    g_test_add_func("/fishlim/__cbc", __ecb);
    g_test_add_func("/fishlim/__base64_len", __base64_len);
    g_test_add_func("/fishlim/__base64_fish_len", __base64_fish_len);
    g_test_add_func("/fishlim/__base64_ecb_len", __base64_ecb_len);
    g_test_add_func("/fishlim/__base64_cbc_len", __base64_cbc_len);
    g_test_add_func("/fishlim/__max_text_command_len", __max_text_command_len);
    g_test_add_func("/fishlim/__foreach_utf8_data_chunks", __foreach_utf8_data_chunks);

    return g_test_run();
}

#endif //PLUGIN_HEXCHAT_FISHLIM_TEST_H