/*

  Copyright (c) 2010 Samuel Lidén Borell <samuel@kodafritt.se>

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

#ifdef __APPLE__
#define __AVAILABILITYMACROS__
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

#include <stdlib.h>
#include <string.h>
#include <openssl/blowfish.h>

#include "keystore.h"
#include "fish.h"

#define IB 64
static const char fish_base64[64] = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const signed char fish_unbase64[256] = {
    IB,IB,IB,IB,IB,IB,IB,IB,  IB,IB,IB,IB,IB,IB,IB,IB,
    IB,IB,IB,IB,IB,IB,IB,IB,  IB,IB,IB,IB,IB,IB,IB,IB,
//      !  "  #  $  %  &  '  (    )  *  +  ,  -  .  /
    IB,IB,IB,IB,IB,IB,IB,IB,  IB,IB,IB,IB,IB,IB, 0, 1,
//   0  1  2  3  4  5  6  7    8  9  :  ;  <  =  >  ?
     2, 3, 4, 5, 6, 7, 8, 9,  10,11,IB,IB,IB,IB,IB,IB,
//   @  A  B  C  D  E  F  G    H  I  J  K  L  M  N  O
    IB,38,39,40,41,42,43,44,  45,46,47,48,49,50,51,52,
//   P  Q  R  S  T  U  V  W    X  Y  Z  [  \  ]  ^  _
    53,54,55,56,57,58,59,60,  61,62,63,IB,IB,IB,IB,IB,
//   `  a  b  c  d  e  f  g    h  i  j  k  l  m  n  o
    IB,12,13,14,15,16,17,18,  19,20,21,22,23,24,25,26,
//   p  q  r  s  t  u  v  w    x  y  z  {  |  }  ~  <del>
    27,28,29,30,31,32,33,34,  35,36,37,IB,IB,IB,IB,IB,
};

#define GET_BYTES(dest, source) do { \
    *((dest)++) = ((source) >> 24) & 0xFF; \
    *((dest)++) = ((source) >> 16) & 0xFF; \
    *((dest)++) = ((source) >> 8) & 0xFF; \
    *((dest)++) = (source) & 0xFF; \
} while (0);


char *fish_encrypt(const char *key, size_t keylen, const char *message) {
    BF_KEY bfkey;
    size_t messagelen;
    size_t i;
    int j;
    char *encrypted;
    char *end;
    unsigned char bit;
    unsigned char word;
    unsigned char d;
    BF_set_key(&bfkey, keylen, (const unsigned char*)key);
    
    messagelen = strlen(message);
    if (messagelen == 0) return NULL;
    encrypted = g_malloc(((messagelen - 1) / 8) * 12 + 12 + 1); // each 8-byte block becomes 12 bytes
    end = encrypted;
     
    while (*message) {
        // Read 8 bytes (a Blowfish block)
        BF_LONG binary[2] = { 0, 0 };
        unsigned char c;
        for (i = 0; i < 8; i++) {
            c = message[i];
            binary[i >> 2] |= c << 8*(3 - (i&3));
            if (c == '\0') break;
        }
        message += 8;
        
        // Encrypt block
        BF_encrypt(binary, &bfkey);
        
        // Emit FiSH-BASE64
        bit = 0;
        word = 1;
        for (j = 0; j < 12; j++) {
            d = fish_base64[(binary[word] >> bit) & 63];
            *(end++) = d;
            bit += 6;
            if (j == 5) {
                bit = 0;
                word = 0;
            }
        }
        
        // Stop if a null terminator was found
        if (c == '\0') break;
    }
    *end = '\0';
    return encrypted;
}


char *fish_decrypt(const char *key, size_t keylen, const char *data) {
    BF_KEY bfkey;
    size_t i;
    char *decrypted;
    char *end;
    unsigned char bit;
    unsigned char word;
    unsigned char d;
    BF_set_key(&bfkey, keylen, (const unsigned char*)key);
    
    decrypted = g_malloc(strlen(data) + 1);
    end = decrypted;
    
    while (*data) {
        // Convert from FiSH-BASE64
        BF_LONG binary[2] = { 0, 0 };
        bit = 0;
        word = 1;
        for (i = 0; i < 12; i++) {
            d = fish_unbase64[(const unsigned char)*(data++)];
            if (d == IB) goto decrypt_end;
            binary[word] |= (unsigned long)d << bit;
            bit += 6;
            if (i == 5) {
                bit = 0;
                word = 0;
            }
        }
        
        // Decrypt block
        BF_decrypt(binary, &bfkey);
        
        // Copy to buffer
        GET_BYTES(end, binary[0]);
        GET_BYTES(end, binary[1]);
    }
    
  decrypt_end:
    *end = '\0';
    return decrypted;
}

/**
 * Encrypts a message (see fish_decrypt). The key is searched for in the
 * key store.
 */
char *fish_encrypt_for_nick(const char *nick, const char *data) {
    char *key;
    char *encrypted;

    // Look for key
    key = keystore_get_key(nick);
    if (!key) return NULL;
    
    // Encrypt
    encrypted = fish_encrypt(key, strlen(key), data);
    
    g_free(key);
    return encrypted;
}

/**
 * Decrypts a message (see fish_decrypt). The key is searched for in the
 * key store.
 */
char *fish_decrypt_from_nick(const char *nick, const char *data) {
    char *key;
    char *decrypted;
    // Look for key
    key = keystore_get_key(nick);
    if (!key) return NULL;
    
    // Decrypt
    decrypted = fish_decrypt(key, strlen(key), data);
    
    g_free(key);
    return decrypted;
}


