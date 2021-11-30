/*

  Copyright (c) 2010 Samuel Lidén Borell <samuel@kodafritt.se>
  Copyright (c) 2019-2020 <bakasura@protonmail.ch>

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

#ifndef FISH_H
#define FISH_H

#include <stddef.h>

#include <glib.h>

enum fish_mode {
  FISH_ECB_MODE = 0x1,
  FISH_CBC_MODE = 0x2
};

int fish_init(void);
void fish_deinit(void);
char *fish_base64_encode(const char *message, size_t message_len);
char *fish_base64_decode(const char *message, size_t *final_len);
char *fish_encrypt(const char *key, size_t keylen, const char *message, size_t message_len, enum fish_mode mode);
char *fish_decrypt(const char *key, size_t keylen, const char *data, enum fish_mode mode, size_t *final_len);
char *fish_decrypt_str(const char *key, size_t keylen, const char *data, enum fish_mode mode);
gboolean fish_nick_has_key(const char *nick);
GSList *fish_encrypt_for_nick(const char *nick, const char *data, enum fish_mode *omode, size_t command_len);
char *fish_decrypt_from_nick(const char *nick, const char *data, enum fish_mode *omode);

#endif


