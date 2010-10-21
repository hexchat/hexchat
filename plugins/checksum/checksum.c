/* XChat-WDK
 * Copyright (c) 2010 Berke Viktor.
 *
 * Use of OpenSSL SHA256 interface: http://adamlamers.com/?p=5
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <openssl/sha.h>

#include "xchat-plugin.h"

#define BUFSIZE 32768

static xchat_plugin *ph;   /* plugin handle */

void
sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
	int i = 0;
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}

void
sha256 (char *string, char outputBuffer[65])
{
	int i;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;

	SHA256_Init (&sha256);
	SHA256_Update (&sha256, string, strlen(string));
	SHA256_Final (hash, &sha256);

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf (outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}

int
sha256_file (char *path, char outputBuffer[65])
{
	int bytesRead;
	unsigned char *buffer;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;

	FILE *file = fopen (path, "rb");
	if (!file)
	{
		return -534;
	}

	SHA256_Init (&sha256);
	buffer = malloc (BUFSIZE);
	bytesRead = 0;

	if (!buffer)
	{
		return ENOMEM;
	}

	while ((bytesRead = fread (buffer, 1, BUFSIZE, file)))
	{
		SHA256_Update (&sha256, buffer, bytesRead);
	}

	SHA256_Final (hash, &sha256);
	sha256_hash_string (hash, outputBuffer);

	fclose (file);
	free (buffer);
	return 0;
}

int
dccrecv_cb(char *word[], void *userdata)
{
	unsigned char sum[65];

	sha256_file (word[2], sum);
	/* try to print the checksum in the privmsg tab of the sender */
	xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
	xchat_printf (ph, "SHA-256 checksum for %s (local):  %s\n", word[1], sum);

	return XCHAT_EAT_NONE;
}

int
dccoffer_cb(char *word[], void *userdata)
{
	unsigned char sum[65];

	sha256_file (word[3], sum);
	xchat_commandf (ph, "raw PRIVMSG %s :SHA-256 checksum for %s (remote): %s", word[2], word[1], sum);

	return XCHAT_EAT_NONE;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "Checksum";
	*plugin_desc = "Calculate checksum for DCC file transfers";
	*plugin_version = "1.1";
	
	xchat_hook_print(ph, "DCC RECV Complete", XCHAT_PRI_NORM, dccrecv_cb, NULL);
	xchat_hook_print(ph, "DCC Offer", XCHAT_PRI_NORM, dccoffer_cb, NULL);

	xchat_print (ph, "Checksum plugin loaded\n");

	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_print (ph, "Checksum plugin unloaded\n");
	return 1;
}
