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
#include <glib.h>
#include <glib/gerror.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#include "hexchat-plugin.h"

#define BUFSIZE 32768
#define DEFAULT_LIMIT 256 /* default size is 256 MiB */
#define SHA256_DIGEST_LENGTH 32
#define SHA256_BUFFER_LENGTH 65

static hexchat_plugin *ph; /* plugin handle */
static char name[] = "Checksum";
static char desc[] = "Calculate checksum for DCC file transfers";
static char version[] = "3.3";

static void set_limit(char *size) {
  int limit = atoi(size);

  if (limit > 0 && limit < INT_MAX) {
    if (hexchat_pluginpref_set_int(ph, "limit", limit))
      hexchat_printf(
          ph,
          "Checksum: File size limit has successfully been set to: %d MiB\n",
          limit);
    else
      hexchat_printf(ph, "Checksum: File access error while saving!\n");
  } else {
    hexchat_printf(ph, "Checksum: Invalid input!\n");
  }
}

static int get_limit(void) {
  int size = hexchat_pluginpref_get_int(ph, "limit");

  if (size <= 0 || size >= INT_MAX)
    return DEFAULT_LIMIT;
  else
    return size;
}

static gboolean check_limit(GFile *file) {
  GFileInfo *file_info;
  goffset file_size;

  file_info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                G_FILE_QUERY_INFO_NONE, NULL, NULL);

  if (!file_info)
    return FALSE;

  file_size = g_file_info_get_size(file_info);
  g_object_unref(file_info);

  if (file_size > get_limit() * 1048576ll)
    return FALSE;

  return TRUE;
}

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
  free(user_data->buffer_size);
  free(user_data->buffer);
  g_checksum_free(user_data->checksum);
  free(user_data);
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
  char *out_buf =  bound_vars->out_buf;
  async_result_cb *cb = bound_vars->result_callback;
  free_user_data(user_data);

  gsize i;
  for (i= 0; i < SHA256_DIGEST_LENGTH; i++) {
    /* out_buf will be exactly SHA256_BUFFER_LENGTH including null */
    g_sprintf(out_buf + (i * 2), "%02x", digest[i]);
  }

  cb(TRUE, bound_data);
}

static void sha256_from_stream_async(GFileInputStream *file_stream,
                                     char out_buf[], async_result_cb *result_cb,
                                     gpointer bound_data) {

  size_t *buffer_size = malloc(sizeof(size_t));
  if (buffer_size == NULL) {
    return result_cb(FALSE, bound_data);
  }

  *buffer_size = BUFSIZ * sizeof(guchar);
  guchar *buffer = malloc(*buffer_size);
  if (buffer == NULL) {
    free(buffer_size);
    return result_cb(FALSE, bound_data);
  }

  GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
  if (checksum == NULL) {
    free(buffer_size);
    free(buffer);
    return result_cb(FALSE, bound_data);
  }

  stream_async_cb_data *user_data = malloc(sizeof(stream_async_cb_data));
  if (user_data == NULL) {
    free(buffer_size);
    free(buffer);
    g_checksum_free(checksum);
    return result_cb(FALSE, bound_data);
  }

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
  free(bound_vars);

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

  if (!check_limit(file)) {
    hexchat_printf(ph,
                   "Checksum: %s is larger than size limit. You can increase "
                   "it with /CHECKSUM SET.\n",
                   filename);
    g_object_unref(file);
    return result_cb(FALSE, bound_data);
  }

  file_stream = g_file_read(file, NULL, NULL);
  if (!file_stream) {
    hexchat_printf(ph, "Checksum: Failed to read file %s\n", filename);
    g_object_unref(file);
    return result_cb(FALSE, bound_data);
  }

  file_async_cb_data *user_data = malloc(sizeof(file_async_cb_data));
  if (user_data == NULL) {
    hexchat_printf(ph, "Checksum: Failed to malloc needed memory\n");
    g_object_unref(file);
    return result_cb(FALSE, bound_data);
  }

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

  free(bound_vars->word_1);
  free(bound_vars->checksum);
  g_free(bound_vars->filename);

  free(bound_vars);
}

static int dccrecv_cb(char *word[], void *userdata) {
  const char *dcc_completed_dir;
  char *filename;
  char *checksum = malloc(SHA256_BUFFER_LENGTH);
  if (checksum == NULL) {
    hexchat_commandf(ph, "Checksum: Failed to malloc needed memory\n");
    return HEXCHAT_EAT_NONE;
  }

  /* Print in the privmsg tab of the sender */
  hexchat_set_context(ph, hexchat_find_context(ph, NULL, word[3]));

  if (hexchat_get_prefs(ph, "dcc_completed_dir", &dcc_completed_dir, NULL) ==
          1 &&
      dcc_completed_dir[0] != '\0') {
    filename = g_build_filename(dcc_completed_dir, word[1], NULL);
  } else {
    filename = g_strdup(word[2]);
  }

  dccrecv_async_cb_data *user_data = malloc(sizeof(dccrecv_async_cb_data));
  if (user_data == NULL) {
    hexchat_printf(ph, "Checksum: Failed to malloc needed memory\n");
    free(checksum);
    return HEXCHAT_EAT_NONE;
  }

  user_data->checksum = checksum;

  char *word_1 = malloc(strlen(word[1]) + 1);
  if (word_1 == NULL) {
    hexchat_printf(ph, "Checksum: Failed to malloc needed memory\n");
    free(checksum);
    free(user_data);
    return HEXCHAT_EAT_NONE;
  }

  strcpy(word_1, word[1]);

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

  free(bound_vars->word_2);
  free(bound_vars->word_1);
  free(bound_vars->checksum);

  free(bound_vars);
}

static int dccoffer_cb(char *word[], void *userdata) {

  /* Print in the privmsg tab of the receiver */
  hexchat_set_context(ph, hexchat_find_context(ph, NULL, word[3]));

  char *checksum = malloc(SHA256_BUFFER_LENGTH);
  if (checksum == NULL) {
    hexchat_commandf(ph, "Checksum: Failed to malloc needed memory\n");
    return HEXCHAT_EAT_NONE;
  }

  dccoffer_async_cb_data *user_data = malloc(sizeof(dccoffer_async_cb_data));
  if (user_data == NULL) {
    hexchat_printf(ph, "Checksum: Failed to malloc needed memory\n");
    free(checksum);
    return HEXCHAT_EAT_NONE;
  }

  user_data->checksum = checksum;

  char *word_1 = malloc(strlen(word[1]) + 1);
  if (word_1 == NULL) {
    hexchat_printf(ph, "Checksum: Failed to malloc needed memory\n");
    free(checksum);
    free(user_data);
    return HEXCHAT_EAT_NONE;
  }

  char *word_2 = malloc(strlen(word[2]) + 1);
  if (word_2 == NULL) {
    hexchat_printf(ph, "Checksum: Failed to malloc needed memory\n");
    free(checksum);
    free(user_data);
    free(word_1);
    return HEXCHAT_EAT_NONE;
  }

  strcpy(word_1, word[1]);
  strcpy(word_2, word[2]);

  user_data->word_1 = word_1;
  user_data->word_2 = word_2;

  sha256_from_file_async(word[3], checksum, dccoffer_async_cb,
                         (void *)user_data);
  return HEXCHAT_EAT_NONE;
}

static int checksum(char *word[], char *word_eol[], void *userdata) {
  if (!g_ascii_strcasecmp("GET", word[2])) {
    hexchat_printf(ph, "File size limit for checksums: %d MiB", get_limit());
  } else if (!g_ascii_strcasecmp("SET", word[2])) {
    set_limit(word[3]);
  } else {
    hexchat_printf(ph, "Usage: /CHECKSUM GET|SET\n");
    hexchat_printf(
        ph, "  GET - print the maximum file size (in MiB) to be hashed\n");
    hexchat_printf(
        ph,
        "  SET <filesize> - set the maximum file size (in MiB) to be hashed\n");
  }

  return HEXCHAT_EAT_ALL;
}

int hexchat_plugin_init(hexchat_plugin *plugin_handle, char **plugin_name,
                        char **plugin_desc, char **plugin_version, char *arg) {
  ph = plugin_handle;

  *plugin_name = name;
  *plugin_desc = desc;
  *plugin_version = version;

  /* this is required for the very first run */
  if (hexchat_pluginpref_get_int(ph, "limit") == -1) {
    hexchat_pluginpref_set_int(ph, "limit", DEFAULT_LIMIT);
  }

  hexchat_hook_command(ph, "CHECKSUM", HEXCHAT_PRI_NORM, checksum,
                       "Usage: /CHECKSUM GET|SET", NULL);
  hexchat_hook_print(ph, "DCC RECV Complete", HEXCHAT_PRI_NORM, dccrecv_cb,
                     NULL);
  hexchat_hook_print(ph, "DCC Offer", HEXCHAT_PRI_NORM, dccoffer_cb, NULL);

  hexchat_printf(ph, "%s plugin loaded\n", name);
  return 1;
}

int hexchat_plugin_deinit(void) {
  hexchat_printf(ph, "%s plugin unloaded\n", name);
  return 1;
}
