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


// Unit Test
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
// Libs
#include <stdio.h>
#include <glib.h>
// Project Libs
#include "../fish.h"
#include "../utils.h"


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

static void __base64_len(void **state) {
    char *b64 = NULL;
    int i, message_len = 0;
    char message[1000];

    for (i = 0; i < 50; ++i) {

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = g_base64_encode((const unsigned char *) message, message_len);
            assert_false(b64 == NULL);
            assert_true(strlen(b64) == base64_len(message_len));
            g_free(b64);
        }
    }
}

static void __base64_fish_len(void **state) {
    char *b64 = NULL;
    int i, message_len = 0;
    char message[1000];

    for (i = 0; i < 50; ++i) {

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = fish_base64_encode(message, message_len);
            assert_false(b64 == NULL);
            assert_true(strlen(b64) == base64_fish_len(message_len));
            g_free(b64);
        }
    }
}

static void __base64_cbc_len(void **state) {
    char *b64 = NULL;
    int i, message_len = 0;
    char key[33];
    char message[1000];

    random_string(key, 32);

    for (i = 0; i < 25; ++i) {

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = fish_encrypt(key, 32, message, message_len, FISH_CBC_MODE);
            assert_false(b64 == NULL);
            assert_true(strlen(b64) == cbc_len(message_len));
            g_free(b64);
        }
    }
}

static void __base64_ecb_len(void **state) {
    char *b64 = NULL;
    int i, message_len = 0;
    char key[33];
    char message[1000];

    random_string(key, 32);

    for (i = 0; i < 25; ++i) {

        for (message_len = 1; message_len < 1000; ++message_len) {
            random_string(message, message_len);
            b64 = fish_encrypt(key, 32, message, message_len, FISH_ECB_MODE);
            //printf("msg: %s, b64: %s, len: %ld, calculated len: %ld\n", message, b64, strlen(b64), ecb_len(message_len));
            assert_false(b64 == NULL);
            assert_true(strlen(b64) == ecb_len(message_len));
            g_free(b64);
        }
    }
}

int main(int argc, char *argv[]) {

    const struct CMUnitTest tests[] = {
            cmocka_unit_test(__base64_len),
            cmocka_unit_test(__base64_fish_len),
            cmocka_unit_test(__base64_cbc_len),
            cmocka_unit_test(__base64_ecb_len),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

#endif //PLUGIN_HEXCHAT_FISHLIM_TEST_H