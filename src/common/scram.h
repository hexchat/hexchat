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
#ifndef HEXCHAT_SCRAM_H
#define HEXCHAT_SCRAM_H

#include "config.h"
#ifdef USE_OPENSSL
#include <openssl/evp.h>

typedef struct
{
	const EVP_MD *digest;
	size_t digest_size;
	char *username;
	char *password;
	char *client_nonce_b64;
	char *client_first_message_bare;
	unsigned char *salted_password;
	char *auth_message;
	char *error;
	int step;
} scram_session;

typedef enum
{
	SCRAM_ERROR = 0,
	SCRAM_IN_PROGRESS,
	SCRAM_SUCCESS
} scram_status;

scram_session *scram_session_create (const char *digset, const char *username, const char *password);
void scram_session_free (scram_session *session);
scram_status scram_process (scram_session *session, const char *input, char **output, size_t *output_len);

#endif
#endif