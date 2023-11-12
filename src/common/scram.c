/* HexChat
 * Copyright (C) 2023 Patrick Okraku
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

#include "hexchat.h"

#ifdef USE_OPENSSL

#include "scram.h"
#include <openssl/hmac.h>
#include <openssl/rand.h>

#define NONCE_LENGTH 18
#define CLIENT_KEY "Client Key"
#define SERVER_KEY "Server Key"

// EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were renamed in OpenSSL 1.1.0
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
#define EVP_MD_CTX_new(ctx) EVP_MD_CTX_create(ctx)
#define EVP_MD_CTX_free(ctx) EVP_MD_CTX_destroy(ctx)
#endif

scram_session
*scram_session_create (const char *digest, const char *username, const char *password)
{
	scram_session *session;
	const EVP_MD *md;
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
	OpenSSL_add_all_algorithms ();
#endif
	md = EVP_get_digestbyname (digest);

	if (md == NULL)
	{
		// Unknown message digest
		return NULL;
	}

	session = g_new0 (scram_session, 1);
	session->digest = md;
	session->digest_size = EVP_MD_size (md);
	session->username = g_strdup (username);
	session->password = g_strdup (password);
	return session;
}

void
scram_session_free (scram_session *session)
{
	if (session == NULL)
	{
		return;
	}

	g_free (session->username);
	g_free (session->password);
	g_free (session->client_nonce_b64);
	g_free (session->client_first_message_bare);
	g_free (session->salted_password);
	g_free (session->auth_message);
	g_free (session->error);

	g_free (session);
}

static int
create_nonce (void *buffer, size_t length)
{
	return RAND_bytes (buffer, length);
}

static int
create_SHA (scram_session *session, const unsigned char *input, size_t input_len,
			unsigned char *output, unsigned int *output_len)
{
	EVP_MD_CTX *md_ctx = EVP_MD_CTX_new ();

	if (!EVP_DigestInit_ex (md_ctx, session->digest, NULL))
	{
		session->error = g_strdup ("Message digest initialization failed");
		EVP_MD_CTX_free (md_ctx);
		return SCRAM_ERROR;
	}

	if (!EVP_DigestUpdate (md_ctx, input, input_len))
	{
		session->error = g_strdup ("Message digest update failed");
		EVP_MD_CTX_free (md_ctx);
		return SCRAM_ERROR;
	}

	if (!EVP_DigestFinal_ex (md_ctx, output, output_len))
	{
		session->error = g_strdup ("Message digest finalization failed");
		EVP_MD_CTX_free (md_ctx);
		return SCRAM_ERROR;
	}

	EVP_MD_CTX_free (md_ctx);
	return SCRAM_IN_PROGRESS;
}

static scram_status
process_client_first (scram_session *session, char **output, size_t *output_len)
{
	char nonce[NONCE_LENGTH];

	if (!create_nonce (nonce, NONCE_LENGTH))
	{
		session->error = g_strdup ("Could not create client nonce");
		return SCRAM_ERROR;
	}

	session->client_nonce_b64 = g_base64_encode ((guchar *) nonce, NONCE_LENGTH);
	*output = g_strdup_printf ("n,,n=%s,r=%s", session->username, session->client_nonce_b64);
	*output_len = strlen (*output);
	session->client_first_message_bare = g_strdup (*output + 3);
	session->step++;
	return SCRAM_IN_PROGRESS;
}

static scram_status
process_server_first (scram_session *session, const char *data, char **output,
					  size_t *output_len)
{
	char **params, *client_final_message_without_proof, *salt, *server_nonce_b64,
			*client_proof_b64;
	unsigned char *client_key, stored_key[EVP_MAX_MD_SIZE], *client_signature, *client_proof;
	unsigned int i, param_count, iteration_count, client_key_len, stored_key_len;
	gsize salt_len = 0;
	size_t client_nonce_len;

	params = g_strsplit (data, ",", -1);
	param_count = g_strv_length (params);

	if (param_count < 3)
	{
		session->error = g_strdup_printf ("Invalid server-first-message: %s", data);
		g_strfreev (params);
		return SCRAM_ERROR;
	}

	server_nonce_b64 = NULL;
	salt = NULL;
	iteration_count = 0;

	for (i = 0; i < param_count; i++)
	{
		if (!strncmp (params[i], "r=", 2))
		{
			g_free (server_nonce_b64);
			server_nonce_b64 = g_strdup (params[i] + 2);
		}
		else if (!strncmp (params[i], "s=", 2))
		{
			g_free (salt);
			salt = g_strdup (params[i] + 2);
		}
		else if (!strncmp (params[i], "i=", 2))
		{
			iteration_count = strtoul (params[i] + 2, NULL, 10);
		}
	}

	g_strfreev (params);

	if (server_nonce_b64 == NULL || *server_nonce_b64 == '\0' || salt == NULL ||
		*salt == '\0' || iteration_count == 0)
	{
		session->error = g_strdup_printf ("Invalid server-first-message: %s", data);
		g_free (server_nonce_b64);
		g_free (salt);
		return SCRAM_ERROR;
	}

	client_nonce_len = strlen (session->client_nonce_b64);

	// The server can append his nonce to the client's nonce
	if (strlen (server_nonce_b64) < client_nonce_len ||
		strncmp (server_nonce_b64, session->client_nonce_b64, client_nonce_len))
	{
		session->error = g_strdup_printf ("Invalid server nonce: %s", server_nonce_b64);
		return SCRAM_ERROR;
	}

	g_base64_decode_inplace ((gchar *) salt, &salt_len);

	// SaltedPassword := Hi(Normalize(password), salt, i)
	session->salted_password = g_malloc (session->digest_size);

	PKCS5_PBKDF2_HMAC (session->password, strlen (session->password), (unsigned char *) salt,
					   salt_len, iteration_count, session->digest, session->digest_size,
					   session->salted_password);

	// AuthMessage := client-first-message-bare + "," +
	//                server-first-message + "," +
	//                client-final-message-without-proof
	client_final_message_without_proof = g_strdup_printf ("c=biws,r=%s", server_nonce_b64);

	session->auth_message = g_strdup_printf ("%s,%s,%s", session->client_first_message_bare,
											 data, client_final_message_without_proof);

	// ClientKey := HMAC(SaltedPassword, "Client Key")
	client_key = g_malloc0 (session->digest_size);

	HMAC (session->digest, session->salted_password, session->digest_size,
		  (unsigned char *) CLIENT_KEY, strlen (CLIENT_KEY), client_key, &client_key_len);

	// StoredKey := H(ClientKey)
	if (!create_SHA (session, client_key, session->digest_size, stored_key, &stored_key_len))
	{
		g_free (client_final_message_without_proof);
		g_free (server_nonce_b64);
		g_free (salt);
		g_free (client_key);
		return SCRAM_ERROR;
	}

	// ClientSignature := HMAC(StoredKey, AuthMessage)
	client_signature = g_malloc0 (session->digest_size);
	HMAC (session->digest, stored_key, stored_key_len, (unsigned char *) session->auth_message,
		  strlen ((char *) session->auth_message), client_signature, NULL);

	// ClientProof := ClientKey XOR ClientSignature
	client_proof = g_malloc0 (client_key_len);

	for (i = 0; i < client_key_len; i++)
	{
		client_proof[i] = client_key[i] ^ client_signature[i];
	}

	client_proof_b64 = g_base64_encode ((guchar *) client_proof, client_key_len);

	*output = g_strdup_printf ("%s,p=%s", client_final_message_without_proof, client_proof_b64);
	*output_len = strlen (*output);

	g_free (server_nonce_b64);
	g_free (salt);
	g_free (client_final_message_without_proof);
	g_free (client_key);
	g_free (client_signature);
	g_free (client_proof);
	g_free (client_proof_b64);

	session->step++;
	return SCRAM_IN_PROGRESS;
}

static scram_status
process_server_final (scram_session *session, const char *data)
{
	char *verifier;
	unsigned char *server_key, *server_signature;
	unsigned int server_key_len = 0, server_signature_len = 0;
	gsize verifier_len = 0;

	if (strlen (data) < 3 || (data[0] != 'v' && data[1] != '='))
	{
		return SCRAM_ERROR;
	}

	verifier = g_strdup (data + 2);
	g_base64_decode_inplace (verifier, &verifier_len);

	// ServerKey := HMAC(SaltedPassword, "Server Key")
	server_key = g_malloc0 (session->digest_size);
	HMAC (session->digest, session->salted_password, session->digest_size,
		  (unsigned char *) SERVER_KEY, strlen (SERVER_KEY), server_key, &server_key_len);

	// ServerSignature := HMAC(ServerKey, AuthMessage)
	server_signature = g_malloc0 (session->digest_size);
	HMAC (session->digest, server_key, session->digest_size,
		  (unsigned char *) session->auth_message, strlen ((char *) session->auth_message),
		  server_signature, &server_signature_len);

	if (verifier_len == server_signature_len &&
		memcmp (verifier, server_signature, verifier_len) == 0)
	{
		g_free (verifier);
		g_free (server_key);
		g_free (server_signature);
		return SCRAM_SUCCESS;
	}
	else
	{
		g_free (verifier);
		g_free (server_key);
		g_free (server_signature);
		return SCRAM_ERROR;
	}
}

scram_status
scram_process (scram_session *session, const char *input, char **output, size_t *output_len)
{
	scram_status status;

	switch (session->step)
	{
		case 0:
			status = process_client_first (session, output, output_len);
			break;
		case 1:
			status = process_server_first (session, input, output, output_len);
			break;
		case 2:
			status = process_server_final (session, input);
			break;
		default:
			*output = NULL;
			*output_len = 0;
			status = SCRAM_ERROR;
			break;
	}

	return status;
}

#endif