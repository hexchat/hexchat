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

#ifdef __APPLE__
#define __AVAILABILITYMACROS__
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/blowfish.h>
#include <openssl/rand.h>

#include "keystore.h"
#include "fish.h"
#include "utils.h"

#define IB 64
/* rfc 2812; 512 - CR-LF = 510 */
static const int MAX_COMMAND_LENGTH = 510;
static const char fish_base64[] = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
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

/**
 * Convert Int to 4 Bytes (Big-endian)
 * 
 * @param int   source
 * @param char* dest
 */
#define GET_BYTES(dest, source) do { \
    *((dest)++) = ((source) >> 24) & 0xFF; \
    *((dest)++) = ((source) >> 16) & 0xFF; \
    *((dest)++) = ((source) >> 8) & 0xFF; \
    *((dest)++) = (source) & 0xFF; \
} while (0);

/**
 * Convert 4 Bytes to Int (Big-endian)
 * 
 * @param char* source
 * @param int   dest
 */
#define GET_LONG(dest, source) do { \
    dest = ((uint8_t)*((source)++) << 24); \
    dest |= ((uint8_t)*((source)++) << 16); \
    dest |= ((uint8_t)*((source)++) << 8); \
    dest |= (uint8_t)*((source)++); \
} while (0);

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/provider.h>
static OSSL_PROVIDER *legacy_provider;
static OSSL_PROVIDER *default_provider;
static OSSL_LIB_CTX *ossl_ctx;
#endif

int fish_init(void)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    ossl_ctx = OSSL_LIB_CTX_new();
    if (!ossl_ctx)
        return 0;

    legacy_provider = OSSL_PROVIDER_load(ossl_ctx, "legacy");
    if (!legacy_provider) {
        fish_deinit();
        return 0;
    }

    default_provider = OSSL_PROVIDER_load(ossl_ctx, "default");
    if (!default_provider) {
        fish_deinit();
        return 0;
    }
#endif
    return 1;
}

void fish_deinit(void)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (legacy_provider) {
        OSSL_PROVIDER_unload(legacy_provider);
        legacy_provider = NULL;
    }

    if (default_provider) {
        OSSL_PROVIDER_unload(default_provider);
        default_provider = NULL;
    }

    if (ossl_ctx) {
        OSSL_LIB_CTX_free(ossl_ctx);
        ossl_ctx = NULL;
    }
#endif
}

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

    /* Each 8-byte block becomes 12 bytes (fish base64 format) and add 1 byte for \0 */
    encoded = g_malloc(((message_len - 1) / 8) * 12 + 12 + 1);
    end = encoded;

    /* Iterate over each 8-byte block (Blowfish block size) */
    for (j = 0; j < message_len; j += 8) {
        msg = (char *) message;

        /* Set left and right longs */
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

        /* The previous for'd ensure fill all bytes of encoded, we don't need will with zeros */
        message += 8;
    }

    *end = '\0';
    return encoded;
}

/**
 * Decode ECB FiSH Base64
 *
 * @param [in]  message    Base64 encoded string
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

    /* Ensure blocks of 12 bytes each one and valid characters */
    if (message_len == 0 || message_len % 12 != 0 || strspn(message, fish_base64) != message_len)
        return NULL;

    /* Each 12 bytes becomes 8-byte block and add 1 byte for \0 */
    *final_len = ((message_len - 1) / 12) * 8 + 8 + 1;
    (*final_len)--; /* We support binary data */
    bytes = (char *) g_malloc0(*final_len);
    byt = bytes;

    msg = (char *) message;

    while (*msg) {
        right = 0;
        left = 0;
        for (i = 0; i < 6; i++) right |= (uint8_t) fish_unbase64[(int)*msg++] << (i * 6u);
        for (i = 0; i < 6; i++) left |= (uint8_t) fish_unbase64[(int)*msg++] << (i * 6u);
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
 * @param [in] plaintext        Bytes to encrypt or decrypt
 * @param [in] plaintext_len    Size of plaintext
 * @param [in] key              Bytes of key
 * @param [in] keylen           Size of key
 * @param [in] encode           1 or encrypt 0 for decrypt
 * @param [in] mode             EVP_CIPH_ECB_MODE or EVP_CIPH_CBC_MODE
 * @param [out] ciphertext_len  The bytes written
 * @return Array of char with data encrypted or decrypted
 */
char *fish_cipher(const char *plaintext, size_t plaintext_len, const char *key, size_t keylen, int encode, int mode, size_t *ciphertext_len) {
    EVP_CIPHER_CTX *ctx;
    EVP_CIPHER *cipher = NULL;
    int bytes_written = 0;
    unsigned char *ciphertext = NULL;
    unsigned char *iv_ciphertext = NULL;
    unsigned char *iv = NULL;
    size_t block_size = 0;

    *ciphertext_len = 0;

    if (plaintext_len == 0 || keylen == 0 || encode < 0 || encode > 1)
        return NULL;

    block_size = plaintext_len;

    if (mode == EVP_CIPH_CBC_MODE) {
        if (encode == 1) {
            iv = (unsigned char *) g_malloc0(8);
            RAND_bytes(iv, 8);
        } else {
            if (plaintext_len <= 8) /* IV + DATA */
                return NULL;

            iv = (unsigned char *) plaintext;
            block_size -= 8;
            plaintext += 8;
            plaintext_len -= 8;
        }

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        cipher = EVP_CIPHER_fetch(ossl_ctx, "BF-CBC", NULL);
#else
        cipher = (EVP_CIPHER *) EVP_bf_cbc();
#endif

    } else if (mode == EVP_CIPH_ECB_MODE) {

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        cipher = EVP_CIPHER_fetch(ossl_ctx, "BF-ECB", NULL);
#else
        cipher = (EVP_CIPHER *) EVP_bf_ecb();
#endif
    }

    /* Zero Padding */
    if (block_size % 8 != 0) {
        block_size = block_size + 8 - (block_size % 8);
    }

    ciphertext = (unsigned char *) g_malloc0(block_size);
    memcpy(ciphertext, plaintext, plaintext_len);

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        return NULL;

    /* Initialise the cipher operation only with mode */
    if (!EVP_CipherInit_ex(ctx, cipher, NULL, NULL, NULL, encode))
        return NULL;

    /* Set custom key length */
    if (!EVP_CIPHER_CTX_set_key_length(ctx, keylen))
        return NULL;

    /* Finish the initiation the cipher operation */
    if (1 != EVP_CipherInit_ex(ctx, NULL, NULL, (const unsigned char *) key, iv, encode))
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


    if (mode == EVP_CIPH_CBC_MODE && encode == 1) {
        /* Join IV + DATA */
        iv_ciphertext = g_malloc0(8 + *ciphertext_len);

        memcpy(iv_ciphertext, iv, 8);
        memcpy(&iv_ciphertext[8], ciphertext, *ciphertext_len);
        *ciphertext_len += 8;

        g_free(ciphertext);
        g_free(iv);

        return (char *) iv_ciphertext;
    } else {
        return (char *) ciphertext;
    }
}

/**
 * Return a fish or standard Base64 encoded string with data encrypted
 * is binary safe
 * 
 * @param [in] key          Bytes of key
 * @param [in] keylen       Size of key
 * @param [in] message      Bytes to encrypt
 * @param [in] message_len  Size of message
 * @param [in] mode         Chiper mode
 * @return Array of char with data encrypted
 */
char *fish_encrypt(const char *key, size_t keylen, const char *message, size_t message_len, enum fish_mode mode) {
    size_t ciphertext_len = 0;
    char *ciphertext = NULL;
    char *b64 = NULL;

    if (keylen == 0 || message_len == 0)
        return NULL;

    ciphertext = fish_cipher(message, message_len, key, keylen, 1, mode, &ciphertext_len);

    if (ciphertext == NULL || ciphertext_len == 0)
        return NULL;

    switch (mode) {
        case FISH_CBC_MODE:
            b64 = g_base64_encode((const unsigned char *) ciphertext, ciphertext_len);
            break;

        case FISH_ECB_MODE:
            b64 = fish_base64_encode((const char *) ciphertext, ciphertext_len);
    }

    g_free(ciphertext);

    if (b64 == NULL)
        return NULL;

    return b64;
}

/**
 * Return an array of bytes with data decrypted
 * is binary safe
 * 
 * @param [in] key         Bytes of key
 * @param [in] keylen      Size of key
 * @param [in] data        Fish or standard Base64 encoded string
 * @param [in] mode        Chiper mode
 * @param [out] final_len  Length of returned array
 * @return Array of char with data decrypted
 */
char *fish_decrypt(const char *key, size_t keylen, const char *data, enum fish_mode mode, size_t *final_len) {
    size_t ciphertext_len = 0;
    char *ciphertext = NULL;
    char *plaintext = NULL;

    *final_len = 0;

    if (keylen == 0 || strlen(data) == 0)
        return NULL;

    switch (mode) {
        case FISH_CBC_MODE:
            if (strspn(data, base64_chars) != strlen(data))
                return NULL;
            ciphertext = (char *) g_base64_decode(data, &ciphertext_len);
            break;

        case FISH_ECB_MODE:
            ciphertext = fish_base64_decode(data, &ciphertext_len);
    }

    if (ciphertext == NULL || ciphertext_len == 0)
        return NULL;

    plaintext = fish_cipher(ciphertext, ciphertext_len, key, keylen, 0, mode, final_len);
    g_free(ciphertext);

    if (*final_len == 0)
        return NULL;

    return plaintext;
}

/**
 * Similar to fish_decrypt, but pad with zeros any after the first zero in the decrypted data
 * 
 * @param [in] key         Bytes of key
 * @param [in] keylen      Size of key
 * @param [in] data        Fish or standard Base64 encoded string
 * @param [in] mode        Chiper mode
 * @return Array of char with data decrypted
 */
char *fish_decrypt_str(const char *key, size_t keylen, const char *data, enum fish_mode mode) {
    char *decrypted = NULL;
    char *plaintext_str = NULL;
    size_t decrypted_len = 0;

    decrypted = fish_decrypt(key, strlen(key), data, mode, &decrypted_len);

    if (decrypted == NULL || decrypted_len == 0)
        return NULL;

    plaintext_str = g_strndup(decrypted, decrypted_len);
    g_free(decrypted);

    return plaintext_str;
}

/**
 * Determine if a nick have a key
 *
 * @param [in] nick  Nickname
 * @return TRUE if have a key or FALSE if not
 */
gboolean fish_nick_has_key(const char *nick) {
    gboolean has_key = FALSE;
    char *key;
    enum fish_mode mode;

    key = keystore_get_key(nick, &mode);
    if (key) {
        has_key = TRUE;
        g_free(key);
    };

    return has_key;
}

/**
 * Encrypts a message (see fish_encrypt). The key is searched for in the key store
 *
 * @param [in] nick         Nickname
 * @param [in] data         Plaintext to encrypt
 * @param [out] omode       Mode of encryption
 * @param [in] command_len  Length of command to use without the message part
 * @return A list of encoded strings with the message encrypted or NULL if any error occurred
 */
GSList *fish_encrypt_for_nick(const char *nick, const char *data, enum fish_mode *omode, size_t command_len) {
    char *key;
    GSList *encrypted_list = NULL;
    char *encrypted = NULL;
    enum fish_mode mode;
    int max_len, max_chunks_len, chunks_len;

    /* Look for key */
    key = keystore_get_key(nick, &mode);
    if (!key) return NULL;

    *omode = mode;

    /* Calculate max length of each line */
    max_len = MAX_COMMAND_LENGTH - command_len;
    /* Add '*' */
    if (mode == FISH_CBC_MODE) max_len--;

    max_chunks_len = max_text_command_len(max_len, mode);

    const char *data_chunk = data;

    while(foreach_utf8_data_chunks(data_chunk, max_chunks_len, &chunks_len)) {
        encrypted = fish_encrypt(key, strlen(key), data_chunk, chunks_len, mode);

        if (mode == FISH_CBC_MODE) {
            /* Add '*' for CBC */
            encrypted_list = g_slist_append(encrypted_list, g_strdup_printf("*%s", encrypted));
            g_free(encrypted);
        } else {
            encrypted_list = g_slist_append(encrypted_list, encrypted);
        }

        /* Next chunk */
        data_chunk += chunks_len;
    }

    return encrypted_list;
}

/**
 * Decrypts a message (see fish_decrypt). The key is searched for in the key store
 *
 * @param [in] nick     Nickname
 * @param [in] data     Plaintext to encrypt
 * @param [out] omode   Mode of encryption
 * @return Plaintext message or NULL if any error occurred
 */
char *fish_decrypt_from_nick(const char *nick, const char *data, enum fish_mode *omode) {
    char *key;
    char *decrypted;
    enum fish_mode mode;

    /* Look for key */
    key = keystore_get_key(nick, &mode);
    if (!key) return NULL;

    *omode = mode;

    if (mode == FISH_CBC_MODE)
        ++data;

    /* Decrypt */
    decrypted = fish_decrypt_str(key, strlen(key), data, mode);
    g_free(key);

    return decrypted;
}


