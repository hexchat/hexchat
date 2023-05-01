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

#include <gio/gio.h>

#include "hexchat-plugin.h"

static hexchat_plugin *ph;									/* plugin handle */
static char name[] = "Checksum";
static char desc[] = "Calculate checksum for DCC file transfers";
static char version[] = "4.0";


typedef struct {
	gboolean send_message;
	char *servername;
	char *channel;
} ChecksumCallbackInfo;


static void
print_sha256_result (ChecksumCallbackInfo *info, const char *checksum, const char *filename, GError *error)
{
	// So then we get the next best available channel, since we always want to print at least somewhere, it's fine
	hexchat_context *ctx = hexchat_find_context(ph, info->servername, info->channel);
	if (!ctx) {
		// before we print a private message to the wrong channel, we exit early
		if (info->send_message) {
			return;
		}

		// if the context isn't found the first time, we search in the server
		ctx = hexchat_find_context(ph, info->servername, NULL);
		if (!ctx) {
			// The second time we exit early, since printing in another server isn't desireable
			return;
		}
	}

	hexchat_set_context(ph, ctx);

	if (error) {
		hexchat_printf (ph, "Failed to create checksum for %s: %s\n", filename, error->message);
	} else if (info->send_message) {
		hexchat_commandf (ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s", hexchat_get_info (ph, "channel"), filename, checksum);
	} else {
		hexchat_printf (ph, "SHA-256 checksum for %s (local): %s\n", filename, checksum);
	}
}

static void
file_sha256_complete (GFile *file, GAsyncResult *result, gpointer user_data)
{	
	ChecksumCallbackInfo * callback_info = user_data;
	GError *error = NULL;
	char *sha256 = NULL;
	const char *filename = g_task_get_task_data (G_TASK (result));

	sha256 = g_task_propagate_pointer (G_TASK (result), &error);
	print_sha256_result (callback_info, sha256, filename, error);

	g_free(callback_info->servername);
	g_free(callback_info->channel);
	g_free(callback_info);
	g_free (sha256);
	g_clear_error (&error);
}

static void
thread_sha256_file (GTask *task, GFile *file, gpointer task_data, GCancellable *cancellable)
{
	GChecksum *checksum;
	GFileInputStream *istream;
	guint8 buffer[32768];
	GError *error = NULL;
	gssize ret;

	istream = g_file_read (file, cancellable, &error);
	if (error) {
		g_task_return_error (task, error);
		return;
	}

	checksum = g_checksum_new (G_CHECKSUM_SHA256);

	while ((ret = g_input_stream_read (G_INPUT_STREAM (istream), buffer, sizeof(buffer), cancellable, &error)) > 0)
		g_checksum_update (checksum, buffer, ret);

	if (error) {
		g_checksum_free (checksum);
		g_task_return_error (task, error);
		return;
	}

	g_task_return_pointer (task, g_strdup (g_checksum_get_string (checksum)), g_free);
	g_checksum_free (checksum);
}

static int
dccrecv_cb (char *word[], void *userdata)
{
	GTask *task;
	char *filename_fs;
	GFile *file;
	const char *dcc_completed_dir;
	char *filename;

	if (hexchat_get_prefs (ph, "dcc_completed_dir", &dcc_completed_dir, NULL) == 1 && dcc_completed_dir[0] != '\0')
		filename = g_build_filename (dcc_completed_dir, word[1], NULL);
	else
		filename = g_strdup (word[2]);

	filename_fs = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
	if (!filename_fs) {
		hexchat_printf (ph, "Checksum: Invalid filename (%s)\n", filename);
		g_free (filename);
		return HEXCHAT_EAT_NONE;
	}

	ChecksumCallbackInfo *callback_data = g_new (ChecksumCallbackInfo, 1);
	callback_data->servername = g_strdup(hexchat_get_info(ph, "server"));
	callback_data->channel = g_strdup(hexchat_get_info(ph, "channel"));
	callback_data->send_message = FALSE;


	file = g_file_new_for_path (filename_fs);
	task = g_task_new (file, NULL, (GAsyncReadyCallback) file_sha256_complete, (gpointer)callback_data);
	g_task_set_task_data (task, filename, g_free);
	g_task_run_in_thread (task, (GTaskThreadFunc) thread_sha256_file);

	g_free (filename_fs);
	g_object_unref (file);
	g_object_unref (task);

	return HEXCHAT_EAT_NONE;
}

static int
dccoffer_cb (char *word[], void *userdata)
{
	GFile *file;
	GTask *task;
	char *filename;

	ChecksumCallbackInfo *callback_data = g_new (ChecksumCallbackInfo, 1);
	callback_data->servername = g_strdup(hexchat_get_info(ph, "server"));
	callback_data->channel = g_strdup(hexchat_get_info(ph, "channel"));
	callback_data->send_message = TRUE;

	filename = g_strdup (word[3]);
	file = g_file_new_for_path (filename);
	task = g_task_new (file, NULL, (GAsyncReadyCallback) file_sha256_complete, (gpointer)callback_data);
	g_task_set_task_data (task, filename, g_free);
	g_task_run_in_thread (task, (GTaskThreadFunc) thread_sha256_file);

	g_object_unref (file);
	g_object_unref (task);

	return HEXCHAT_EAT_NONE;
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

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
