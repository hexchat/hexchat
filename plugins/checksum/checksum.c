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

#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "hexchat-plugin.h"

#define BUFSIZE 32768
#define DEFAULT_LIMIT 256									/* default size is 256 MiB */
#define SHA256_DIGEST_LENGTH 32
#define SHA256_BUFFER_LENGTH 65

static hexchat_plugin *ph;									/* plugin handle */
static char name[] = "Checksum";
static char desc[] = "Calculate checksum for DCC file transfers";
static char version[] = "3.1";

static void
set_limit (char *size)
{
	int limit = atoi (size);

	if (limit > 0 && limit < INT_MAX)
	{
		if (hexchat_pluginpref_set_int (ph, "limit", limit))
			hexchat_printf (ph, "Checksum: File size limit has successfully been set to: %d MiB\n", limit);
		else
			hexchat_printf (ph, "Checksum: File access error while saving!\n");
	}
	else
	{
		hexchat_printf (ph, "Checksum: Invalid input!\n");
	}
}

static int
get_limit (void)
{
	int size = hexchat_pluginpref_get_int (ph, "limit");

	if (size <= 0 || size >= INT_MAX)
		return DEFAULT_LIMIT;
	else
		return size;
}

static gboolean
check_limit (GFile *file)
{
	GFileInfo *file_info;
	goffset file_size;

	file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE,
									NULL, NULL);

	if (!file_info)
		return FALSE;

	file_size = g_file_info_get_size (file_info);
	g_object_unref (file_info);

	if (file_size > get_limit () * 1048576ll)
		return FALSE;

	return TRUE;
}

static gboolean
sha256_from_stream (GFileInputStream *file_stream, char out_buf[])
{
	GChecksum *checksum;
	gssize bytes_read;
	guint8 digest[SHA256_DIGEST_LENGTH];
	gsize digest_len = sizeof(digest);
	guchar buffer[BUFSIZE];
	gsize i;

	checksum = g_checksum_new (G_CHECKSUM_SHA256);

	while ((bytes_read = g_input_stream_read (G_INPUT_STREAM (file_stream), buffer, sizeof (buffer), NULL, NULL)))
	{
		if (bytes_read == -1)
		{
			g_checksum_free (checksum);
			return FALSE;
		}

		g_checksum_update (checksum, buffer, bytes_read);
	}

	g_checksum_get_digest (checksum, digest, &digest_len);
	g_checksum_free (checksum);

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		/* out_buf will be exactly SHA256_BUFFER_LENGTH including null */
		g_sprintf (out_buf + (i * 2), "%02x", digest[i]);
	}

	return TRUE;
}

static gboolean
sha256_from_file (char *filename, char out_buf[])
{
	GFileInputStream *file_stream;
	char *filename_fs;
	GFile *file;

	filename_fs = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
	if (!filename_fs)
	{
		hexchat_printf (ph, "Checksum: Invalid filename (%s)\n", filename);
		return FALSE;
	}

	file = g_file_new_for_path (filename_fs);
	g_free (filename_fs);
	if (!file)
	{
		hexchat_printf (ph, "Checksum: Failed to open %s\n", filename);
		return FALSE;
	}

	if (!check_limit (file))
	{
		hexchat_printf (ph, "Checksum: %s is larger than size limit. You can increase it with /CHECKSUM SET.\n", filename);
		g_object_unref (file);
		return FALSE;
	}

	file_stream = g_file_read (file, NULL, NULL);
	if (!file_stream)
	{
		hexchat_printf (ph, "Checksum: Failed to read file %s\n", filename);
		g_object_unref (file);
		return FALSE;
	}

	if (!sha256_from_stream (file_stream, out_buf))
	{
		hexchat_printf (ph, "Checksum: Failed to generate checksum for %s\n", filename);
		g_object_unref (file_stream);
		g_object_unref (file);
		return FALSE;
	}

	g_object_unref (file_stream);
	g_object_unref (file);
	return TRUE;
}

static int
dccrecv_cb (char *word[], void *userdata)
{
	const char *dcc_completed_dir;
	char *filename, checksum[SHA256_BUFFER_LENGTH];

	/* Print in the privmsg tab of the sender */
	hexchat_set_context (ph, hexchat_find_context (ph, NULL, word[3]));

	if (hexchat_get_prefs (ph, "dcc_completed_dir", &dcc_completed_dir, NULL) == 1 && dcc_completed_dir[0] != '\0')
		filename = g_build_filename (dcc_completed_dir, word[1], NULL);
	else
		filename = g_strdup (word[2]);

	if (sha256_from_file (filename, checksum))
	{
		hexchat_printf (ph, "SHA-256 checksum for %s (local): %s\n", word[1], checksum);
	}

	g_free (filename);
	return HEXCHAT_EAT_NONE;
}

static int
dccoffer_cb (char *word[], void *userdata)
{
	char checksum[SHA256_BUFFER_LENGTH];

	/* Print in the privmsg tab of the receiver */
	hexchat_set_context (ph, hexchat_find_context (ph, NULL, word[3]));

	if (sha256_from_file (word[3], checksum))
	{
		hexchat_commandf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s", word[2], word[1], checksum);
	}

	return HEXCHAT_EAT_NONE;
}

static int
checksum (char *word[], char *word_eol[], void *userdata)
{
	if (!g_ascii_strcasecmp ("GET", word[2]))
	{
		hexchat_printf (ph, "File size limit for checksums: %d MiB", get_limit ());
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

	return HEXCHAT_EAT_ALL;
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

	hexchat_hook_command (ph, "CHECKSUM", HEXCHAT_PRI_NORM, checksum, "Usage: /CHECKSUM GET|SET", NULL);
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
