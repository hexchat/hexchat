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
#include <glib/gerror.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "hexchat-plugin.h"

#define BUFSIZE 32768
#define SHA256_DIGEST_LENGTH 32
#define SHA256_BUFFER_LENGTH 65

static hexchat_plugin *ph;									/* plugin handle */
static char name[] = "Checksum";
static char desc[] = "Calculate checksum for DCC file transfers";
static char version[] = "3.2";

typedef void async_result_cb(gboolean, gpointer);

typedef struct {
  GChecksum *checksum;
  async_result_cb *result_callback;
  guchar *buffer;
  size_t *buffer_size;
  char *out_buf;
  gpointer bound_data;
} stream_async_cb_data;

static void free_user_data(stream_async_cb_data *user_data) {
  g_free(user_data->buffer_size);
  g_free(user_data->buffer);
  g_checksum_free(user_data->checksum);
  g_free(user_data);
}

static void stream_async_cb(GObject *source_object, GAsyncResult *res,
                            gpointer user_data) {
  stream_async_cb_data *bound_vars = (void *)user_data;

  GInputStream *stream = G_INPUT_STREAM(source_object);
  GError *error = NULL;
  gssize bytes_read = g_input_stream_read_finish(stream, res, &error);

  if (bytes_read != 0) {
    if (bytes_read == -1 || error) {
      async_result_cb *cb = bound_vars->result_callback;
      gpointer bound_data = bound_vars->bound_data;
      free_user_data(bound_vars);
      return cb(FALSE, bound_data);
    }

    g_checksum_update(bound_vars->checksum, bound_vars->buffer, bytes_read);

    return g_input_stream_read_async(
        stream, bound_vars->buffer, *bound_vars->buffer_size,
        G_PRIORITY_DEFAULT, NULL, stream_async_cb, user_data);
  }

  guint8 digest[SHA256_DIGEST_LENGTH];
  gsize digest_len = sizeof(digest);

  g_checksum_get_digest(bound_vars->checksum, digest, &digest_len);
  gpointer bound_data = bound_vars->bound_data;
  char *out_buf = bound_vars->out_buf;
  async_result_cb *cb = bound_vars->result_callback;
  free_user_data(user_data);

  gsize i;
  for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    /* out_buf will be exactly SHA256_BUFFER_LENGTH including null */
    g_sprintf(out_buf + (i * 2), "%02x", digest[i]);
  }

  cb(TRUE, bound_data);
}

static void sha256_from_stream_async(GFileInputStream *file_stream,
                                     char out_buf[], async_result_cb *result_cb,
                                     gpointer bound_data) {

  size_t *buffer_size = g_new(size_t, 1);

  *buffer_size = BUFSIZ * sizeof(guchar);

  guchar *buffer = g_malloc(*buffer_size);
  GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
  stream_async_cb_data *user_data = g_new(stream_async_cb_data, 1);

  user_data->checksum = checksum;
  user_data->result_callback = result_cb;
  user_data->buffer = buffer;
  user_data->buffer_size = buffer_size;
  user_data->out_buf = out_buf;
  user_data->bound_data = bound_data;

  g_input_stream_read_async(G_INPUT_STREAM(file_stream), buffer, *buffer_size,
                            G_PRIORITY_DEFAULT, NULL, stream_async_cb,
                            (void *)user_data);
}

typedef struct {
  async_result_cb *result_cb;
  GFileInputStream *file_stream;
  char *filename;
  GFile *file;
  gpointer bound_data;
} file_async_cb_data;

static void file_async_cb(gboolean result, gpointer user_data) {

  file_async_cb_data *bound_vars = (void *)user_data;

  if (!result) {
    hexchat_printf(ph, "Checksum: Failed to generate checksum for %s\n",
                   bound_vars->filename);
  }

  g_object_unref(bound_vars->file_stream);
  g_object_unref(bound_vars->file);
  async_result_cb *cb = bound_vars->result_cb;
  gpointer bound_data = bound_vars->bound_data;
  g_free(bound_vars);

  return cb(result, bound_data);
}

static void sha256_from_file_async(char *filename, char out_buf[],
                                   async_result_cb *result_cb,
                                   gpointer bound_data) {
  GFileInputStream *file_stream;
  char *filename_fs;
  GFile *file;

  filename_fs = g_filename_from_utf8(filename, -1, NULL, NULL, NULL);
  if (!filename_fs) {
    hexchat_printf(ph, "Checksum: Invalid filename (%s)\n", filename);
    return result_cb(FALSE, bound_data);
  }

  file = g_file_new_for_path(filename_fs);
  g_free(filename_fs);
  if (!file) {
    hexchat_printf(ph, "Checksum: Failed to open %s\n", filename);
    return result_cb(FALSE, bound_data);
  }


  file_stream = g_file_read(file, NULL, NULL);
  if (!file_stream) {
    hexchat_printf(ph, "Checksum: Failed to read file %s\n", filename);
    g_object_unref(file);
    return result_cb(FALSE, bound_data);
  }

  file_async_cb_data *user_data = g_new(file_async_cb_data, 1);


  user_data->result_cb = result_cb;
  user_data->file_stream = file_stream;
  user_data->filename = filename;
  user_data->file = file;
  user_data->bound_data = bound_data;

  sha256_from_stream_async(file_stream, out_buf, file_async_cb,
                           (void *)user_data);
}

typedef struct {
  char *checksum;
  char *word_1;
  char *filename;
} dccrecv_async_cb_data;

void dccrecv_async_cb(gboolean result, gpointer user_data) {

  dccrecv_async_cb_data *bound_vars = (void *)user_data;

  if (result) {
    hexchat_printf(ph, "SHA-256 checksum for %s (local): %s\n",
                   bound_vars->word_1, bound_vars->checksum);
  }

  g_free(bound_vars->word_1);
  g_free(bound_vars->checksum);
  g_free(bound_vars->filename);

  g_free(bound_vars);
}

static int dccrecv_cb(char *word[], void *userdata) {
  const char *dcc_completed_dir;
  char *filename;
  char *checksum = g_malloc(SHA256_BUFFER_LENGTH);

  /* Print in the privmsg tab of the sender */
  hexchat_set_context(ph, hexchat_find_context(ph, NULL, word[3]));

  if (hexchat_get_prefs(ph, "dcc_completed_dir", &dcc_completed_dir, NULL) ==
          1 &&
      dcc_completed_dir[0] != '\0') {
    filename = g_build_filename(dcc_completed_dir, word[1], NULL);
  } else {
    filename = g_strdup(word[2]);
  }

  dccrecv_async_cb_data *user_data = g_new(dccrecv_async_cb_data,1);

  user_data->checksum = checksum;

  char *word_1 = g_strdup(word[1]);

  user_data->word_1 = word_1;
  user_data->filename = filename;

  sha256_from_file_async(filename, checksum, dccrecv_async_cb,
                         (void *)user_data);

  return HEXCHAT_EAT_NONE;
}

typedef struct {
  char *checksum;
  char *word_1;
  char *word_2;
} dccoffer_async_cb_data;

void dccoffer_async_cb(gboolean result, gpointer user_data) {

  dccoffer_async_cb_data *bound_vars = (void *)user_data;

  if (result) {
    hexchat_commandf(
        ph, "quote PRIVMSG %s :SHA-256 checksum for %s (remote): %s",
        bound_vars->word_2, bound_vars->word_1, bound_vars->checksum);
  }

  g_free(bound_vars->word_2);
  g_free(bound_vars->word_1);
  g_free(bound_vars->checksum);

  g_free(bound_vars);
}

static int dccoffer_cb(char *word[], void *userdata) {

  /* Print in the privmsg tab of the receiver */
  hexchat_set_context(ph, hexchat_find_context(ph, NULL, word[3]));

  char *checksum = g_malloc(SHA256_BUFFER_LENGTH);

  dccoffer_async_cb_data *user_data = g_new(dccoffer_async_cb_data, 1);


  user_data->checksum = checksum;

   char *word_1 = g_strdup(word[1]);
   char *word_2 = g_strdup(word[2]);

  user_data->word_1 = word_1;
  user_data->word_2 = word_2;

  sha256_from_file_async(word[3], checksum, dccoffer_async_cb,
                         (void *)user_data);
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
