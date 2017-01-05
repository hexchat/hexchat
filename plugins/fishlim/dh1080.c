/* HexChat
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/

/*
 * For Diffie-Hellman key-exchange a 1080bit germain prime is used, the
 * generator g=2 renders a field Fp from 1 to p-1. Therefore breaking it
 * means to solve a discrete logarithm problem with no less than 1080bit.
 *
 * Base64 format is used to send the public keys over IRC.
 *
 * The calculated secret key is hashed with SHA-256, the result is converted
 * to base64 for final use with blowfish.
 */

#include "config.h"
#include "dh1080.h"

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/sha.h>

#include <string.h>
#include <glib.h>

#define DH1080_PRIME_BITS 1080
#define DH1080_PRIME_BYTES 135
#define SHA256_DIGEST_LENGTH 32
#define B64ABC "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
#define MEMZERO(x) memset(x, 0x00, sizeof(x))

/* All clients must use the same prime number to be able to keyx */
static const guchar prime1080[DH1080_PRIME_BYTES] =
{
	0xFB, 0xE1, 0x02, 0x2E, 0x23, 0xD2, 0x13, 0xE8, 0xAC, 0xFA, 0x9A, 0xE8, 0xB9, 0xDF, 0xAD, 0xA3, 0xEA,
	0x6B, 0x7A, 0xC7, 0xA7, 0xB7, 0xE9, 0x5A, 0xB5, 0xEB, 0x2D, 0xF8, 0x58, 0x92, 0x1F, 0xEA, 0xDE, 0x95,
	0xE6, 0xAC, 0x7B, 0xE7, 0xDE, 0x6A, 0xDB, 0xAB, 0x8A, 0x78, 0x3E, 0x7A, 0xF7, 0xA7, 0xFA, 0x6A, 0x2B,
	0x7B, 0xEB, 0x1E, 0x72, 0xEA, 0xE2, 0xB7, 0x2F, 0x9F, 0xA2, 0xBF, 0xB2, 0xA2, 0xEF, 0xBE, 0xFA, 0xC8,
	0x68, 0xBA, 0xDB, 0x3E, 0x82, 0x8F, 0xA8, 0xBA, 0xDF, 0xAD, 0xA3, 0xE4, 0xCC, 0x1B, 0xE7, 0xE8, 0xAF,
	0xE8, 0x5E, 0x96, 0x98, 0xA7, 0x83, 0xEB, 0x68, 0xFA, 0x07, 0xA7, 0x7A, 0xB6, 0xAD, 0x7B, 0xEB, 0x61,
	0x8A, 0xCF, 0x9C, 0xA2, 0x89, 0x7E, 0xB2, 0x8A, 0x61, 0x89, 0xEF, 0xA0, 0x7A, 0xB9, 0x9A, 0x8A, 0x7F,
	0xA9, 0xAE, 0x29, 0x9E, 0xFA, 0x7B, 0xA6, 0x6D, 0xEA, 0xFE, 0xFB, 0xEF, 0xBF, 0x0B, 0x7D, 0x8B
};

static DH *g_dh;

int
dh1080_init (void)
{
	g_return_val_if_fail (g_dh == NULL, 0);

	if ((g_dh = DH_new()))
	{
		int codes;
		BIGNUM *p, *g;

		p = BN_bin2bn (prime1080, DH1080_PRIME_BYTES, NULL);
		g = BN_new ();

		if (p == NULL || g == NULL)
			return 1;

		BN_set_word (g, 2);

#ifndef HAVE_DH_SET0_PQG
		g_dh->p = p;
		g_dh->g = g;
#else
		if (!DH_set0_pqg (g_dh, p, NULL, g))
			return 1;
#endif

		if (DH_check (g_dh, &codes))
			return codes == 0;
	}

	return 0;
}

void
dh1080_deinit (void)
{
	g_clear_pointer (&g_dh, DH_free);
}

static inline int
DH_verifyPubKey (BIGNUM *pk)
{
	int codes;
	return DH_check_pub_key (g_dh, pk, &codes) && codes == 0;
}

static guchar *
dh1080_decode_b64 (const char *data, gsize *out_len)
{
	GString *str = g_string_new (data);
	guchar *ret;

	if (str->len % 4 == 1 && str->str[str->len - 1] == 'A')
		g_string_truncate (str, str->len - 1);

	while (str->len % 4 != 0)
		g_string_append_c (str, '=');

	ret = g_base64_decode_inplace (str->str, out_len);
	g_string_free (str, FALSE);
  	return ret;
}

static char *
dh1080_encode_b64 (const guchar *data, gsize data_len)
{
	char *ret = g_base64_encode (data, data_len);
	char *p;

	if (!(p = strchr (ret, '=')))
	{
		char *new_ret = g_new(char, strlen(ret) + 2);
		strcpy (new_ret, ret);
		strcat (new_ret, "A");
		g_free (ret);
		ret = new_ret;
	}
	else
	{
		*p = '\0';
	}

  	return ret;
}

int
dh1080_generate_key (char **priv_key, char **pub_key)
{
	guchar buf[DH1080_PRIME_BYTES];
	int len;
	DH *dh;
	const BIGNUM *dh_priv_key, *dh_pub_key;

  	g_assert (priv_key != NULL);
	g_assert (pub_key != NULL);

  	dh = DHparams_dup (g_dh);
	if (!dh)
		return 0;

	if (!DH_generate_key (dh))
	{
		DH_free (dh);
		return 0;
	}

#ifndef HAVE_DH_GET0_KEY
	dh_pub_key = dh->pub_key;
	dh_priv_key = dh->priv_key;
#else
	DH_get0_key (dh, &dh_pub_key, &dh_priv_key);
#endif

	MEMZERO (buf);
	len = BN_bn2bin (dh_priv_key, buf);
	*priv_key = dh1080_encode_b64 (buf, len);

	MEMZERO (buf);
	len = BN_bn2bin (dh_pub_key, buf);
	*pub_key = dh1080_encode_b64 (buf, len);

	OPENSSL_cleanse (buf, sizeof (buf));
	DH_free (dh);
	return 1;
}

int
dh1080_compute_key (const char *priv_key, const char *pub_key, char **secret_key)
{
	char *pub_key_data;
	gsize pub_key_len;
	BIGNUM *pk;
	DH *dh;

  	g_assert (secret_key != NULL);

	/* Verify base64 strings */
	if (strspn (priv_key, B64ABC) != strlen (priv_key)
	    || strspn (pub_key, B64ABC) != strlen (pub_key))
		return 0;

	dh = DHparams_dup (g_dh);

	pub_key_data = dh1080_decode_b64 (pub_key, &pub_key_len);
	pk = BN_bin2bn (pub_key_data, pub_key_len, NULL);

	if (DH_verifyPubKey (pk))
	{
		guchar shared_key[DH1080_PRIME_BYTES] = { 0 };
		guchar sha256[SHA256_DIGEST_LENGTH] = { 0 };
		char *priv_key_data;
		gsize priv_key_len;
		int shared_len;
		BIGNUM *priv_key_num;

	  	priv_key_data = dh1080_decode_b64 (priv_key, &priv_key_len);
		priv_key_num = BN_bin2bn(priv_key_data, priv_key_len, NULL);
#ifndef HAVE_DH_SET0_KEY
		dh->priv_key = priv_key_num;
#else
		DH_set0_key (dh, NULL, priv_key_num);
#endif

		shared_len = DH_compute_key (shared_key, pk, dh);
		SHA256(shared_key, shared_len, sha256);
		*secret_key = dh1080_encode_b64 (sha256, sizeof(sha256));

		OPENSSL_cleanse (priv_key_data, priv_key_len);
	  	g_free (priv_key_data);
	}

	BN_free (pk);
	DH_free (dh);
	g_free (pub_key_data);
	return 1;
}
