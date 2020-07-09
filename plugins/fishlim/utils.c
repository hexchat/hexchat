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

#include "utils.h"

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
 * Calculate the length of Base64-encoded string
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