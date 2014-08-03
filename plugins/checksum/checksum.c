/* HexChat
 * Copyright (c) 2010-2012 Berke Viktor.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef __APPLE__
#define __AVAILABILITYMACROS__
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <glib.h>

#ifdef WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#define stat _stat64
#else
/* for INT_MAX */
#include <limits.h>
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "hexchat-plugin.h"

#define BUFSIZE 32768
#define DEFAULT_LIMIT 256									/* default size is 256 MiB */

static hexchat_plugin *ph;									/* plugin handle */
static char name[] = "Checksum";
static char desc[] = "Calculate checksum for DCC file transfers";
static char version[] = "3.1";

/* Use of OpenSSL SHA256 interface: http://adamlamers.com/?p=5 */
static void
sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
	int i;
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf (outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}

#if 0
static void
sha256 (char *string, char outputBuffer[65])
{
	int i;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;

	SHA256_Init (&sha256);
	SHA256_Update (&sha256, string, strlen (string));
	SHA256_Final (hash, &sha256);

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf (outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}
#endif

static int
sha256_file (const char *path, char outputBuffer[65])
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
		fclose (file);
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

static void
set_limit (const char* size)
{
	int buffer = atoi (size);

	if (buffer > 0 && buffer < INT_MAX)
	{
		if (hexchat_pluginpref_set_int (ph, "limit", buffer))
		{
			hexchat_printf (ph, "File size limit has successfully been set to: %d MiB\n", buffer);
		}
		else
		{
			hexchat_printf (ph, "File access error while saving!\n");
		}
	}
	else
	{
		hexchat_printf (ph, "Invalid input!\n");
	}
}

static int
get_limit ()
{
	int size = hexchat_pluginpref_get_int (ph, "limit");

	if (size <= -1 || size >= INT_MAX)
	{
		return DEFAULT_LIMIT;
	}
	else
	{
		return size;
	}
}

static void
print_limit ()
{
	hexchat_printf (ph, "File size limit for checksums: %d MiB", get_limit ());
}

static int
dccrecv_cb(const char *const word[], void *userdata)
{
	int result;
	struct stat buffer;									/* buffer for storing file info */
	char sum[65];											/* buffer for checksum */
	const char *file;
	char *cfile;

	if (hexchat_get_prefs (ph, "dcc_completed_dir", &file, NULL) == 1 && file[0] != 0)
	{
		cfile = g_strconcat (file, G_DIR_SEPARATOR_S, word[1], NULL);
	}
	else
	{
		cfile = g_strdup(word[2]);
	}

	result = stat (cfile, &buffer);
	if (result == 0)										/* stat returns 0 on success */
	{
		if (buffer.st_size <= (unsigned long long) get_limit () * 1048576)
		{
			sha256_file (cfile, sum);						/* file is the full filename even if completed dir set */
			/* try to print the checksum in the privmsg tab of the sender */
			hexchat_set_context (ph, hexchat_find_context (ph, NULL, word[3]));
			hexchat_printf (ph, "SHA-256 checksum for %s (local):  %s\n", word[1], sum);
		}
		else
		{
			hexchat_set_context (ph, hexchat_find_context (ph, NULL, word[3]));
			hexchat_printf (ph, "SHA-256 checksum for %s (local):  (size limit reached, no checksum calculated, you can increase it with /CHECKSUM INC)\n", word[1]);
		}
	}
	else
	{
		hexchat_printf (ph, "File access error!\n");
	}

	g_free (cfile);
	return HEXCHAT_EAT_NONE;
}

static int
dccoffer_cb(const char *const word[], void *userdata)
{
	int result;
	struct stat buffer;									/* buffer for storing file info */
	char sum[65];											/* buffer for checksum */

	result = stat (word[3], &buffer);
	if (result == 0)										/* stat returns 0 on success */
	{
		if (buffer.st_size <= (unsigned long long) get_limit () * 1048576)
		{
			sha256_file (word[3], sum);						/* word[3] is the full filename */
			hexchat_commandf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s", word[2], word[1], sum);
		}
		else
		{
			hexchat_set_context (ph, hexchat_find_context (ph, NULL, word[3]));
			hexchat_printf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): (size limit reached, no checksum calculated)", word[2], word[1]);
		}
	}
	else
	{
		hexchat_printf (ph, "File access error!\n");
	}

	return HEXCHAT_EAT_NONE;
}

static int
checksum(const char *const word[], const char *const word_eol[], void *userdata)
{
	if (!g_ascii_strcasecmp ("GET", word[2]))
	{
		print_limit ();
	}
	else if (!g_ascii_strcasecmp ("SET", word[2]))
	{
		set_limit (word[3]);
	}
	else
	{
		hexchat_printf (ph, "Usage: /CHECKSUM GET|SET\n");
		hexchat_printf (ph, "  GET - print the maximum file size (in MiB) to be hashed\n");
		hexchat_printf (ph, "  SET <filesize> - set the maximum file size (in MiB) to be hashed\n");
	}

	return HEXCHAT_EAT_NONE;
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	/* this is required for the very first run */
	if (hexchat_pluginpref_get_int (ph, "limit") == -1)
	{
		hexchat_pluginpref_set_int (ph, "limit", DEFAULT_LIMIT);
	}

	hexchat_hook_command (ph, "CHECKSUM", HEXCHAT_PRI_NORM, checksum, "Usage: /CHECKSUM GET|SET", 0);
	hexchat_hook_print (ph, "DCC RECV Complete", HEXCHAT_PRI_NORM, dccrecv_cb, NULL);
	hexchat_hook_print (ph, "DCC Offer", HEXCHAT_PRI_NORM, dccoffer_cb, NULL);

	hexchat_printf (ph, "%s plugin loaded\n", name);
	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
