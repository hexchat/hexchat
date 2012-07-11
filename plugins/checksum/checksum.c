/* XChat-WDK
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

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/sha.h>

#include "xchat-plugin.h"

#define BUFSIZE 32768
#define DEFAULT_LIMIT 256									/* default size is 256 MiB */

#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef stat64
#define stat64 _stat64
#endif

static xchat_plugin *ph;									/* plugin handle */
static const char name[] = "Checksum";
static const char desc[] = "Calculate checksum for DCC file transfers";
static const char version[] = "3.0";

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

static int
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

static void
set_limit (char* size)
{
	int buffer = atoi (size);

	if (buffer > 0 && buffer < INT_MAX)
	{
		if (xchat_pluginpref_set_int (ph, "limit", buffer))
		{
			xchat_printf (ph, "File size limit has successfully been set to: %d MiB\n", buffer);
		}
		else
		{
			xchat_printf (ph, "File access error while saving!\n");
		}
	}
	else
	{
		xchat_printf (ph, "Invalid input!\n");
	}
}

static int
get_limit ()
{
	int size = xchat_pluginpref_get_int (ph, "limit");

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
	xchat_printf (ph, "File size limit for checksums: %d MiB", get_limit ());
}

static int
dccrecv_cb (char *word[], void *userdata)
{
	int result;
	struct stat64 buffer;									/* buffer for storing file info */
	char sum[65];											/* buffer for checksum */

	result = stat64 (word[2], &buffer);
	if (result == 0)										/* stat returns 0 on success */
	{
		if (buffer.st_size <= (unsigned long long) get_limit () * 1048576)
		{
			sha256_file (word[2], sum);						/* word[2] is the full filename */
			/* try to print the checksum in the privmsg tab of the sender */
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "SHA-256 checksum for %s (local):  %s\n", word[1], sum);
		}
		else
		{
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "SHA-256 checksum for %s (local):  (size limit reached, no checksum calculated, you can increase it with /CHECKSUM INC)\n", word[1]);
		}
	}
	else
	{
		xchat_printf (ph, "File access error!\n");
	}

	return XCHAT_EAT_NONE;
}

static int
dccoffer_cb (char *word[], void *userdata)
{
	int result;
	struct stat64 buffer;									/* buffer for storing file info */
	char sum[65];											/* buffer for checksum */

	result = stat64 (word[3], &buffer);
	if (result == 0)										/* stat returns 0 on success */
	{
		if (buffer.st_size <= (unsigned long long) get_limit () * 1048576)
		{
			sha256_file (word[3], sum);						/* word[3] is the full filename */
			xchat_commandf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s", word[2], word[1], sum);
		}
		else
		{
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): (size limit reached, no checksum calculated)", word[2], word[1]);
		}
	}
	else
	{
		xchat_printf (ph, "File access error!\n");
	}

	return XCHAT_EAT_NONE;
}

static void
checksum (char *word[], void *userdata)
{
	if (!stricmp ("GET", word[2]))
	{
		print_limit ();
	}
	else if (!stricmp ("SET", word[2]))
	{
		set_limit (word[3]);
	}
	else
	{
		xchat_printf (ph, "Usage: /CHECKSUM GET|INC|DEC\n");
		xchat_printf (ph, "  GET - print the maximum file size (in MiB) to be hashed\n");
		xchat_printf (ph, "  SET <filesize> - set the maximum file size (in MiB) to be hashed\n");
	}
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	/* this is required for the very first run */
	if (xchat_pluginpref_get_int (ph, "limit") == -1)
	{
		xchat_pluginpref_set_int (ph, "limit", DEFAULT_LIMIT);
	}

	xchat_hook_command (ph, "CHECKSUM", XCHAT_PRI_NORM, checksum, "Usage: /CHECKSUM GET|SET", 0);
	xchat_hook_print (ph, "DCC RECV Complete", XCHAT_PRI_NORM, dccrecv_cb, NULL);
	xchat_hook_print (ph, "DCC Offer", XCHAT_PRI_NORM, dccoffer_cb, NULL);

	xchat_printf (ph, "%s plugin loaded\n", name);
	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
