/*

  Copyright (c) 2010 Samuel Lidén Borell <samuel@kodafritt.se>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

*/

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "irc.h"
#include "fish.h"
#include "misc.h"
#include "keystore.h"
#include "plugin_xchat.h"


static char *keystore_password = NULL;


/**
 * Opens the key store file: ~/.xchat2/blow.ini
 */
static GKeyFile *getConfigFile() {
    gchar *filename = get_config_filename();
    
    GKeyFile *keyfile = g_key_file_new();
    g_key_file_load_from_file(keyfile, filename,
                              G_KEY_FILE_KEEP_COMMENTS |
                              G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    
    g_free(filename);
    return keyfile;
}


/**
 * Returns the key store password, or the default.
 */
static const char *get_keystore_password() {
    return (keystore_password != NULL ?
        keystore_password :
        // Silly default value...
        "blowinikey");
}


/**
 * Gets a value for a nick/channel from blow.ini. Unlike
 * g_key_file_get_string, this function is case insensitive.
 */
static gchar *get_nick_value(GKeyFile *keyfile, const char *nick, const char *item) {
    gchar **group;
    gchar **groups = g_key_file_get_groups(keyfile, NULL);
    gchar *result = NULL;
    
    for (group = groups; *group != NULL; group++) {
        if (!irc_nick_cmp(*group, nick)) {
            result = g_key_file_get_string(keyfile, *group, item, NULL);
            break;
        }
    }
    
    g_strfreev(groups);
    return result;
}


/**
 * Extracts a key from the key store file.
 */
char *keystore_get_key(const char *nick) {
    // Get the key
    GKeyFile *keyfile = getConfigFile();
    gchar *value = get_nick_value(keyfile, nick, "key");
    g_key_file_free(keyfile);
    if (!value) return NULL;
    
    if (strncmp(value, "+OK ", 4) != 0) {
        // Key is stored in plaintext
        return import_glib_string(value);
    } else {
        // Key is encrypted
        const char *encrypted = value+4;
        const char *password = get_keystore_password();
        char *decrypted = fish_decrypt(password, strlen(password), encrypted);
        g_free(value);
        return decrypted;
    }
}

/**
 * Deletes a nick and the associated key in the key store file.
 */
static bool delete_nick(GKeyFile *keyfile, const char *nick) {
    gchar **group;
    gchar **groups = g_key_file_get_groups(keyfile, NULL);
    bool ok = false;
    
    for (group = groups; *group != NULL; group++) {
        if (!irc_nick_cmp(*group, nick)) {
            ok = g_key_file_remove_group(keyfile, *group, NULL);
            break;
        }
    }
    
    g_strfreev(groups);
    return ok;
}

/**
 * Writes the key store file to disk.
 */
static bool save_keystore(GKeyFile *keyfile) {
    char *filename;
    bool ok;
    // Serialize
    gsize file_length;
    gchar *file_data = g_key_file_to_data(keyfile, &file_length, NULL);
    if (!file_data) return false;
    
    // Write to file
    filename = get_config_filename();
    ok = g_file_set_contents(filename, file_data, file_length, NULL);
    g_free(filename);
    g_free(file_data);
    return ok;
}

/**
 * Sets a key in the key store file.
 */
bool keystore_store_key(const char *nick, const char *key) {
    const char *password;
    char *encrypted;
    char *wrapped;
    bool ok = false;
    GKeyFile *keyfile = getConfigFile();
    
    // Remove old key
    delete_nick(keyfile, nick);
    
    // Add new key
    password = get_keystore_password();
    if (password) {
        // Encrypt the password
        encrypted = fish_encrypt(password, strlen(password), key);
        if (!encrypted) goto end;
        
        // Prepend "+OK "
        wrapped = g_strconcat("+OK ", encrypted, NULL);
        g_free(encrypted);
        
        // Store encrypted in file
        g_key_file_set_string(keyfile, nick, "key", wrapped);
        free(wrapped);
    } else {
        // Store unencrypted in file
        g_key_file_set_string(keyfile, nick, "key", key);
    }
    
    // Save key store file
    ok = save_keystore(keyfile);
    
  end:
    g_key_file_free(keyfile);
    return ok;
}

/**
 * Deletes a nick from the key store.
 */
bool keystore_delete_nick(const char *nick) {
    GKeyFile *keyfile = getConfigFile();
    
    // Delete entry
    bool ok = delete_nick(keyfile, nick);
    
    // Save
    if (ok) save_keystore(keyfile);
    
    g_key_file_free(keyfile);
    return ok;
}


void keystore_secure_free(void *ptr, size_t size) {
    secure_erase(ptr, size);
    free(ptr);
}


