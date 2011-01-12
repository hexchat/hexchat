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
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/sha.h>

#include "xchat-plugin.h"

#define BUFSIZE 32768
#define DEFAULT_MAX_HASH_SIZE 536870912						/* default size is 512 MB */

#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef stat64
#define stat64 _stat64
#endif

static xchat_plugin *ph;									/* plugin handle */
static int config_fail;										/* variable for config availability */

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
init ()
{
	/* check whether the config file exists, if it doesn't, try to create it */
	FILE * file_in;
	FILE * file_out;
	char buffer[512];

	snprintf (buffer, sizeof (buffer), "%s/checksum.conf", xchat_get_info (ph, "xchatdirfs"));

	if ((file_in = fopen (buffer, "r")) == NULL)
	{
		if ((file_out = fopen (buffer, "w")) == NULL)
		{
			config_fail = 1;
		} else
		{
			config_fail = 0;
			fprintf (file_out, "%llu\n", (unsigned long long) DEFAULT_MAX_HASH_SIZE);
		}
	}

	fclose (file_in);
	fclose (file_out);
}

static unsigned long long
get_max_hash_size ()
{
	FILE * file_in;
	char buffer[512];
	unsigned long long max_hash_size;

	if (config_fail)
	{
		return (unsigned long long) DEFAULT_MAX_HASH_SIZE;
	} else
	{
		snprintf (buffer, sizeof (buffer), "%s/checksum.conf", xchat_get_info (ph, "xchatdirfs"));
		file_in = fopen (buffer, "r");
		fscanf (file_in, "%llu", &max_hash_size);

		fclose (file_in);
		return max_hash_size;
	}
}

static void
print_size ()
{
	unsigned long long size;
	char suffix[3];
	
	size = get_max_hash_size ();
	
	if (size >= 1073741824)
	{
		size /= 1073741824;
		snprintf (suffix, sizeof (suffix), "GB");
	} else if (size >= 1048576)
	{
		size /= 1048576;
		snprintf (suffix, sizeof (suffix), "MB");
	} else if (size >= 1024)
	{
		size /= 1024;
		snprintf (suffix, sizeof (suffix), "kB");
	} else
	{
		snprintf (suffix, sizeof (suffix), "B");
	}
	xchat_printf (ph, "File size limit for checksums: %llu %s\n", size, suffix);
}

static void
increase_max_hash_size ()
{
	unsigned long long size;
	FILE * file_out;
	char buffer[512];

	if (config_fail)
	{
		xchat_printf (ph, "Config file is unavailable, falling back to the default value\n");
		print_size ();
	} else
	{
		size = get_max_hash_size ();
		if (size <= ULLONG_MAX/2)
		{
			size *= 2;
		}
		
		snprintf (buffer, sizeof (buffer), "%s/checksum.conf", xchat_get_info (ph, "xchatdirfs"));
		file_out = fopen (buffer, "w");
		fprintf (file_out, "%llu\n", size);
		fclose (file_out);
		print_size ();
	}
}

static void
decrease_max_hash_size ()
{
	unsigned long long size;
	FILE * file_out;
	char buffer[512];

	if (config_fail)
	{
		xchat_printf (ph, "Config file is unavailable, falling back to the default value\n");
		print_size ();
	} else
	{
		size = get_max_hash_size ();
		if (size >= 2)
		{
			size /= 2;
		}
		
		snprintf (buffer, sizeof (buffer), "%s/checksum.conf", xchat_get_info (ph, "xchatdirfs"));
		file_out = fopen (buffer, "w");
		fprintf (file_out, "%llu\n", size);
		fclose (file_out);
		print_size ();
	}
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
		if (buffer.st_size <= get_max_hash_size ())
		{
			sha256_file (word[2], sum);						/* word[2] is the full filename */
			/* try to print the checksum in the privmsg tab of the sender */
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "SHA-256 checksum for %s (local):  %s\n", word[1], sum);
		} else
		{
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "SHA-256 checksum for %s (local):  (size limit reached, no checksum calculated, you can increase it with /CHECKSUM INC)\n", word[1]);
		}
	} else
	{
		xchat_printf (ph, "File access error\n");
	}

	return XCHAT_EAT_NONE;
}

static int
dccoffer_cb (char *word[], void *userdata)
{
	int result;
	struct stat64 buffer;									/* buffer for storing file info */
	char sum[65];											/* buffer for checksum */

	result = stat64 (word[2], &buffer);
	if (result == 0)										/* stat returns 0 on success */
	{
		if (buffer.st_size <= get_max_hash_size ())
		{
			sha256_file (word[3], sum);						/* word[3] is the full filename */
			xchat_commandf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s", word[2], word[1], sum);
		} else
		{
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "SHA-256 checksum for %s (local):  (size limit reached, no checksum calculated, you can increase it with /CHECKSUM INC)\n", word[1]);
		}
	} else
	{
		xchat_printf (ph, "File access error\n");
	}

	return XCHAT_EAT_NONE;
}

static void
checksum (char *userdata[])
{
	if (!stricmp ("GET", userdata[2]))
	{
		print_size ();
	} else if (!stricmp ("INC", userdata[2]))
	{
		increase_max_hash_size ();
	} else if (!stricmp ("DEC", userdata[2]))
	{
		decrease_max_hash_size ();
	} else
	{
		xchat_printf (ph, "Usage: /CHECKSUM GET|INC|DEC\n");
		xchat_printf (ph, "  GET - print the maximum file size to be hashed\n");
		xchat_printf (ph, "  INC - double the maximum file size to be hashed\n");
		xchat_printf (ph, "  DEC - halve the maximum file size to be hashed\n");
	}
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "Checksum";
	*plugin_desc = "Calculate checksum for DCC file transfers";
	*plugin_version = "2.0";

	init ();

	xchat_hook_command (ph, "CHECKSUM", XCHAT_PRI_NORM, checksum, "Usage: /CHECKSUM GET|INC|DEC", 0);
	xchat_hook_print (ph, "DCC RECV Complete", XCHAT_PRI_NORM, dccrecv_cb, NULL);
	xchat_hook_print (ph, "DCC Offer", XCHAT_PRI_NORM, dccoffer_cb, NULL);

	xchat_print (ph, "Checksum plugin loaded\n");
	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_print (ph, "Checksum plugin unloaded\n");
	return 1;
}
