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

#ifndef PLUGIN_HEXCHAT_FISHLIM_UTILS_H
#define PLUGIN_HEXCHAT_FISHLIM_UTILS_H

#include <stddef.h>
#include "fish.h"

unsigned long base64_len(size_t plaintext_len);
unsigned long base64_fish_len(size_t plaintext_len);
unsigned long cbc_len(size_t plaintext_len);
unsigned long ecb_len(size_t plaintext_len);
unsigned long encoded_len(size_t plaintext_len, enum fish_mode mode);
int max_text_command_len(size_t max_len, enum fish_mode mode);
const char *foreach_utf8_data_chunks(const char *data, int max_chunk_len, int *chunk_len);

#endif