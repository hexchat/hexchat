/* XChat-WDK
 * Copyright (c) 2010-2011 Berke Viktor.
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
#define DEFAULT_MAX_HASH_SIZE 268435456						/* default size is 256 MB */
#define FILE_BUF_SIZE 512

#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef stat64
#define stat64 _stat64
#endif

static xchat_plugin *ph;									/* plugin handle */
static const char name[] = "Checksum";
static const char desc[] = "Calculate checksum for DCC file transfers";
static const char version[] = "2.0";
static int config_fail;										/* variable for config availability */

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
init ()
{
	/* check whether the config file exists, if it doesn't, try to create it */
	FILE * file_in;
	FILE * file_out;
	char buffer[FILE_BUF_SIZE];

	config_fail = 0;
	snprintf (buffer, sizeof (buffer), "%s/checksum.conf", xchat_get_info (ph, "xchatdirfs"));

	if ((file_in = fopen (buffer, "r")) == NULL)
	{
		if ((file_out = fopen (buffer, "w")) == NULL)
		{
			config_fail = 1;
		} else
		{
			fprintf (file_out, "%llu\n", (unsigned long long) DEFAULT_MAX_HASH_SIZE);
			fclose (file_out);
		}
	} else
	{
		fclose (file_in);
	}

	/* nasty easter egg: if FILE_BUF_SIZE is set to 1024 and you build for x86, you can do fclose ()
	   at the end of init (), which is plain wrong as it will only work if fopen () != 0. */
}

static unsigned long long
get_max_hash_size ()
{
	FILE * file_in;
	char buffer[FILE_BUF_SIZE];
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
	char buffer[FILE_BUF_SIZE];

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
	char buffer[FILE_BUF_SIZE];

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

	result = stat64 (word[3], &buffer);
	if (result == 0)										/* stat returns 0 on success */
	{
		if (buffer.st_size <= get_max_hash_size ())
		{
			sha256_file (word[3], sum);						/* word[3] is the full filename */
			xchat_commandf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s", word[2], word[1], sum);
		} else
		{
			xchat_set_context (ph, xchat_find_context (ph, NULL, word[3]));
			xchat_printf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): (size limit reached, no checksum calculated)", word[2], word[1]);
		}
	} else
	{
		xchat_printf (ph, "File access error\n");
	}

	return XCHAT_EAT_NONE;
}

static void
checksum (char *word[], void *userdata)
{
	if (!stricmp ("GET", word[2]))
	{
		print_size ();
	} else if (!stricmp ("INC", word[2]))
	{
		increase_max_hash_size ();
	} else if (!stricmp ("DEC", word[2]))
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

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	init ();

	xchat_hook_command (ph, "CHECKSUM", XCHAT_PRI_NORM, checksum, "Usage: /CHECKSUM GET|INC|DEC", 0);
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
