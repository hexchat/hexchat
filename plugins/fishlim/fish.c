/*

  Copyright (c) 2010 Samuel Lidén Borell <samuel@kodafritt.se>
  Copyright (c) 2019 <bakasura@protonmail.ch>

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
#include <openssl/evp.h>
#include <openssl/blowfish.h>

#include "keystore.h"
#include "fish.h"

#define IB 64
static const char fish_base64[64] = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const signed char fish_unbase64[256] = {
    IB,IB,IB,IB,IB,IB,IB,IB,  IB,IB,IB,IB,IB,IB,IB,IB,
    IB,IB,IB,IB,IB,IB,IB,IB,  IB,IB,IB,IB,IB,IB,IB,IB,
/*      !  "  #  $  %  &  '  (    )  *  +  ,  -  .  / */
    IB,IB,IB,IB,IB,IB,IB,IB,  IB,IB,IB,IB,IB,IB, 0, 1,
/*   0  1  2  3  4  5  6  7    8  9  :  ;  <  =  >  ? */
     2, 3, 4, 5, 6, 7, 8, 9,  10,11,IB,IB,IB,IB,IB,IB,
/*   @  A  B  C  D  E  F  G    H  I  J  K  L  M  N  O */
    IB,38,39,40,41,42,43,44,  45,46,47,48,49,50,51,52,
/*   P  Q  R  S  T  U  V  W    X  Y  Z  [  \  ]  ^  _*/
    53,54,55,56,57,58,59,60,  61,62,63,IB,IB,IB,IB,IB,
/*   `  a  b  c  d  e  f  g    h  i  j  k  l  m  n  o */
    IB,12,13,14,15,16,17,18,  19,20,21,22,23,24,25,26,
/*   p  q  r  s  t  u  v  w    x  y  z  {  |  }  ~  <del> */
    27,28,29,30,31,32,33,34,  35,36,37,IB,IB,IB,IB,IB,
};

#define GET_BYTES(dest, source) do { \
    *((dest)++) = ((source) >> 24) & 0xFF; \
    *((dest)++) = ((source) >> 16) & 0xFF; \
    *((dest)++) = ((source) >> 8) & 0xFF; \
    *((dest)++) = (source) & 0xFF; \
} while (0);

#define GET_LONG(dest, source) do { \
    dest = ((uint8_t)*((source)++) << 24); \
    dest |= ((uint8_t)*((source)++) << 16); \
    dest |= ((uint8_t)*((source)++) << 8); \
    dest |= (uint8_t)*((source)++); \
} while (0);


/**
 * Encode ECB FiSH Base64
 *
 * @param [in] message      Bytes to encode
 * @param [in] message_len  Size of bytes to encode
 * @return Array of char with encoded string
 */
char *fish_base64_encode(const char *message, size_t message_len) {
    BF_LONG left = 0, right = 0;
    int i, j;
    char *encoded = NULL;
    char *end = NULL;
    char *msg = NULL;

    if (message_len == 0)
        return NULL;

    encoded = g_malloc(((message_len - 1) / 8) * 12 + 12 + 1); /* each 8-byte block becomes 12 bytes */
    end = encoded;

    for (j = 0; j < message_len; j += 8) {
        msg = (char *) message;

        GET_LONG(left, msg);
        GET_LONG(right, msg);

        for (i = 0; i < 6; ++i) {
            *end++ = fish_base64[right & 0x3fu];
            right = (right >> 6u);
        }

        for (i = 0; i < 6; ++i) {
            *end++ = fish_base64[left & 0x3fu];
            left = (left >> 6u);
        }

        message += 8;
    }

    *end = '\0';
    return encoded;
}

/**
 * Decode ECB FiSH Base64
 *
 * @param [in] message     Base64 encoded string
 * @param [out] final_len  Real length of message
 * @return Array of char with decoded message
 */
char *fish_base64_decode(const char *message, size_t *final_len) {
    BF_LONG left, right;
    int i;
    char *bytes = NULL;
    char *msg = NULL;
    char *byt = NULL;
    size_t message_len;

    message_len = strlen(message);

    if (message_len == 0 || message_len % 12 != 0 || strspn(message, fish_base64) != message_len)
        return NULL;

    *final_len = ((message_len - 1) / 12) * 8 + 8 + 1; /* Each 12 bytes becomes 8-byte block */
    (*final_len)--; /* We support binary data */
    bytes = (char *) g_malloc0(*final_len);
    byt = bytes;

    msg = (char *) message;

    while (*msg) {
        right = 0;
        left = 0;
        for (i = 0; i < 6; i++) right |= (uint8_t) fish_unbase64[*msg++] << (i * 6u);
        for (i = 0; i < 6; i++) left |= (uint8_t) fish_unbase64[*msg++] << (i * 6u);
        GET_BYTES(byt, left);
        GET_BYTES(byt, right);
    }

    return bytes;
}

/**
 * Encrypt or decrypt data with Blowfish cipher, support binary data.
 *
 * Good documentation for EVP:
 *
 * - https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
 *
 * - https://stackoverflow.com/questions/5727646/what-is-the-length-parameter-of-aes-evp-decrypt
 *
 * - https://stackoverflow.com/questions/26345175/correct-way-to-free-allocate-the-context-in-the-openssl
 *
 * - https://stackoverflow.com/questions/29874150/working-with-evp-and-openssl-coding-in-c
 *
 * @param [in] plaintext        Bytes to encrypt or descrypt
 * @param [in] plaintext_len    Size of plaintext
 * @param [in] key              Bytes of key
 * @param [in] keylen           Size of key
 * @param [in] encode           1 or encrypt 0 for decrypt
 * @param [out] ciphertext_len  The bytes writen
 * @return Array of char with data crypted or uncrypted
 */
char *fish_cipher(const char *plaintext, size_t plaintext_len, const char *key, size_t keylen, int encode, size_t *ciphertext_len) {
    EVP_CIPHER_CTX *ctx;
    int bytes_written = 0;
    unsigned char *ciphertext = NULL;
    size_t block_size = 0;

    if(plaintext_len == 0 || keylen == 0 || encode < 0 || encode > 1)
        return NULL;

    /* Zero Padding */
    block_size = plaintext_len;

    if (block_size % 8 != 0) {
        block_size = block_size + 8 - (block_size % 8);
    }

    ciphertext = (unsigned char *) g_malloc0(block_size);
    memcpy(ciphertext, plaintext, plaintext_len);

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        return NULL;

    /* Initialise the cipher operation only with mode  */
    if (!EVP_CipherInit_ex(ctx, EVP_bf_ecb(), NULL, NULL, NULL, encode))
        return NULL;

    /* Set custom key length */
    if (!EVP_CIPHER_CTX_set_key_length(ctx, keylen))
        return NULL;

    /* Finish the initiation the cipher operation */
    if (1 != EVP_CipherInit_ex(ctx, NULL, NULL, (const unsigned char *) key, NULL, encode))
        return NULL;

    /* We will manage this */
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    /* Do cipher operation */
    if (1 != EVP_CipherUpdate(ctx, ciphertext, &bytes_written, ciphertext, block_size))
        return NULL;

    *ciphertext_len = bytes_written;

    /* Finalise the cipher. Further ciphertext bytes may be written at this stage */
    if (1 != EVP_CipherFinal_ex(ctx, ciphertext + bytes_written, &bytes_written))
        return NULL;

    *ciphertext_len += bytes_written;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return (char *) ciphertext;
}


char *fish_encrypt(const char *key, size_t keylen, const char *message, size_t message_len) {
    size_t ciphertext_len = 0;
    char *ciphertext = NULL;
    char *b64 = NULL;

    if(keylen == 0 || message_len == 0)
        return NULL;

    ciphertext = fish_cipher(message, message_len, key, keylen, 1, &ciphertext_len);

    if(ciphertext == NULL || ciphertext_len == 0)
        return NULL;

    b64 = fish_base64_encode((const char *) ciphertext, ciphertext_len);
    g_free(ciphertext);

    if (b64 == NULL)
        return NULL;

    return b64;
}


char *fish_decrypt(const char *key, size_t keylen, const char *data) {
    size_t ciphertext_len = 0;
    char *ciphertext = NULL;
    char *plaintext = NULL;
    char *plaintext_str = NULL;

    if(keylen == 0 || strlen(data) == 0)
        return NULL;

    ciphertext = fish_base64_decode(data, &ciphertext_len);

    if (ciphertext == NULL || ciphertext_len == 0)
        return NULL;

    plaintext = fish_cipher(ciphertext, ciphertext_len, key, keylen, 0, &ciphertext_len);
    g_free(ciphertext);

    if (ciphertext_len == 0)
        return NULL;

    plaintext_str = g_malloc0(ciphertext_len + 1);
    memcpy(plaintext_str, plaintext, ciphertext_len);
    g_free(plaintext);

    return plaintext_str;
}


/**
 * Encrypts a message (see fish_decrypt). The key is searched for in the
 * key store.
 */
char *fish_encrypt_for_nick(const char *nick, const char *data) {
    char *key;
    char *encrypted;

    /* Look for key */
    key = keystore_get_key(nick);
    if (!key) return NULL;
    
    /* Encrypt */
    encrypted = fish_encrypt(key, strlen(key), data, strlen(data));
    
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
    /* Look for key */
    key = keystore_get_key(nick);
    if (!key) return NULL;
    
    /* Decrypt */
    decrypted = fish_decrypt(key, strlen(key), data);
    
    g_free(key);
    return decrypted;
}


