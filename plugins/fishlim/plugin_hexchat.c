/*

  Copyright (c) 2010-2011 Samuel Lidén Borell <samuel@kodafritt.se>
  Copyright (c) 2015 <the.cypher@gmail.com>
  Copyright (c) 2019-2020 <bakasura@protonmail.ch>

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

#include "config.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "hexchat-plugin.h"

#include "fish.h"
#include "dh1080.h"
#include "keystore.h"
#include "irc.h"

static const char *fish_modes[] = {"", "ECB", "CBC", NULL};

static const char plugin_name[] = "FiSHLiM";
static const char plugin_desc[] = "Encryption plugin for the FiSH protocol. Less is More!";
static const char plugin_version[] = "1.0.0";

static const char usage_setkey[] = "Usage: SETKEY [<nick or #channel>] [<mode>:]<password>, sets the key for a channel or nick. Modes: ECB, CBC";
static const char usage_delkey[] = "Usage: DELKEY [<nick or #channel>], deletes the key for a channel or nick";
static const char usage_keyx[] = "Usage: KEYX [<nick>], performs DH1080 key-exchange with <nick>";
static const char usage_topic[] = "Usage: TOPIC+ <topic>, sets a new encrypted topic for the current channel";
static const char usage_notice[] = "Usage: NOTICE+ <nick or #channel> <notice>";
static const char usage_msg[] = "Usage: MSG+ <nick or #channel> <message>";


static hexchat_plugin *ph;
static GHashTable *pending_exchanges;


/**
 * Compare two nicks using the current plugin
 */
int irc_nick_cmp(const char *a, const char *b) {
    return hexchat_nickcmp (ph, a, b);
}

/**
 * Returns the path to the key store file.
 */
gchar *get_config_filename(void) {
    char *filename_fs, *filename_utf8;

    filename_utf8 = g_build_filename(hexchat_get_info(ph, "configdir"), "addon_fishlim.conf", NULL);
    filename_fs = g_filename_from_utf8 (filename_utf8, -1, NULL, NULL, NULL);

    g_free (filename_utf8);
    return filename_fs;
}

static inline gboolean irc_is_query (const char *name) {
    const char *chantypes = hexchat_list_str (ph, NULL, "chantypes");

    return strchr (chantypes, name[0]) == NULL;
}

static hexchat_context *find_context_on_network (const char *name) {
    hexchat_list *channels;
    hexchat_context *ret = NULL;
    int id;

    if (hexchat_get_prefs(ph, "id", NULL, &id) != 2)
        return NULL;

    channels = hexchat_list_get(ph, "channels");
    if (!channels)
        return NULL;

    while (hexchat_list_next(ph, channels)) {
        int chan_id = hexchat_list_int(ph, channels, "id");
        const char *chan_name = hexchat_list_str(ph, channels, "channel");

        if (chan_id == id && chan_name && irc_nick_cmp (chan_name, name) == 0) {
            ret = (hexchat_context*)hexchat_list_str(ph, channels, "context");
            break;
        }
    };

    hexchat_list_free(ph, channels);
    return ret;
}

/**
 * Retrive the field for own user in current context
 * @return the field value
 */
char *get_my_info(const char *field, gboolean find_in_other_context) {
    char *result = NULL;
    const char *own_nick;
    hexchat_list *list;
    hexchat_context *ctx_current, *ctx_channel;

    /* Display message */
    own_nick = hexchat_get_info(ph, "nick");

    if (!own_nick)
        return NULL;

    /* Get field for own nick if any */
    list = hexchat_list_get(ph, "users");
    if (list) {
        while (hexchat_list_next(ph, list)) {
            if (irc_nick_cmp(own_nick, hexchat_list_str(ph, list, "nick")) == 0)
                result = g_strdup(hexchat_list_str(ph, list, field));
        }
        hexchat_list_free(ph, list);
    }

    if (result) {
        return result;
    }

    /* Try to get from a channel (we are outside a channel)  */
    if (!find_in_other_context) {
        return NULL;
    }

    list = hexchat_list_get(ph, "channels");
    if (list) {
        ctx_current = hexchat_get_context(ph);
        while (hexchat_list_next(ph, list)) {
            ctx_channel = (hexchat_context *) hexchat_list_str(ph, list, "context");

            hexchat_set_context(ph, ctx_channel);
            result = get_my_info(field, FALSE);
            hexchat_set_context(ph, ctx_current);

            if (result) {
                break;
            }
        }
        hexchat_list_free(ph, list);
    }

    return result;
}

/**
 * Retrive the prefix character for own nick in current context
 * @return @ or + or NULL
 */
char *get_my_own_prefix(void) {
    return get_my_info("prefix", FALSE);
}

/**
 * Retrive the mask for own nick in current context
 * @return Host name in the form: user@host (or NULL if not known)
 */
char *get_my_own_host(void) {
    return get_my_info("host", TRUE);
}

/**
 * Calculate the length of prefix for current user in current context
 *
 * @return Length of prefix
 */
int get_prefix_length(void) {
    char *own_host;
    int prefix_len = 0;

    /* ':! ' + 'nick' + 'ident@host', e.g. ':user!~name@mynet.com ' */
    prefix_len = 3 + strlen(hexchat_get_info(ph, "nick"));
    own_host = get_my_own_host();
    if (own_host) {
        prefix_len += strlen(own_host);
    } else {
        /* https://stackoverflow.com/questions/8724954/what-is-the-maximum-number-of-characters-for-a-host-name-in-unix */
        prefix_len += 64;
    }
    g_free(own_host);

    return prefix_len;
}

/**
 * Try to decrypt the first occurrence of fish message
 * 
 * @param message  Message to decrypt
 * @param key     Key of message
 * @return Array of char with decrypted message or NULL. The returned string 
 * should be freed with g_free() when no longer needed.
 */
char *decrypt_raw_message(const char *message, const char *key) {
    const char *prefixes[] = {"+OK ", "mcps ", NULL};
    char *start = NULL, *end = NULL;
    char *left = NULL, *right = NULL;
    char *encrypted = NULL, *decrypted = NULL;
    int length = 0;
    int index_prefix;
    enum fish_mode mode;
    GString *message_decrypted;
    char *result = NULL;

    if (message == NULL || key == NULL)
        return NULL;

    for (index_prefix = 0; index_prefix < 2; index_prefix++) {
        start = g_strstr_len(message, strlen(message), prefixes[index_prefix]);
        if (start) {
            /* Length ALWAYS will be less that original message
             * add '[CBC] ' length */
            message_decrypted = g_string_sized_new(strlen(message) + 6);

            /* Left part of message */
            left = g_strndup(message, start - message);
            g_string_append(message_decrypted, left);
            g_free(left);

            /* Encrypted part */
            start += strlen(prefixes[index_prefix]);
            end = g_strstr_len(start, strlen(message), " ");
            if (end) {
                length = end - start;
                right = end;
            }

            if (length > 0) {
                encrypted = g_strndup(start, length);
            } else {
                encrypted = g_strdup(start);
            }
            decrypted = fish_decrypt_from_nick(key, encrypted, &mode);
            g_free(encrypted);

            if (decrypted == NULL) {
                g_string_free(message_decrypted, TRUE);
                return NULL;
            }

            /* Add encrypted flag */
            g_string_append(message_decrypted, "[");
            g_string_append(message_decrypted, fish_modes[mode]);
            g_string_append(message_decrypted, "] ");
            /* Decrypted message */
            g_string_append(message_decrypted, decrypted);
            g_free(decrypted);

            /* Right part of message */
            if (right) {
                g_string_append(message_decrypted, right);
            }

            result = message_decrypted->str;
            g_string_free(message_decrypted, FALSE);
            return result;
        }
    }

    return NULL;
}

/*static int handle_debug(char *word[], char *word_eol[], void *userdata) {
    hexchat_printf(ph, "debug incoming: ");
    for (size_t i = 1; word[i] != NULL && word[i][0] != '\0'; i++) {
        hexchat_printf(ph, ">%s< ", word[i]);
    }
    hexchat_printf(ph, "\n");
    return HEXCHAT_EAT_NONE;
}*/

/**
 * Called when a message is to be sent.
 */
static int handle_outgoing(char *word[], char *word_eol[], void *userdata) {
    char *prefix;
    enum fish_mode mode;
    char *message;
    GString *command;
    GSList *encrypted_list, *encrypted_item;

    const char *channel = hexchat_get_info(ph, "channel");

    /* Check if we can encrypt */
    if (!fish_nick_has_key(channel)) return HEXCHAT_EAT_NONE;

    command = g_string_new("");
    g_string_printf(command, "PRIVMSG %s :+OK ", channel);

    encrypted_list = fish_encrypt_for_nick(channel, word_eol[1], &mode, get_prefix_length() + command->len);
    if (!encrypted_list) {
        g_string_free(command, TRUE);
        return HEXCHAT_EAT_NONE;
    }

    /* Get prefix for own nick if any */
    prefix = get_my_own_prefix();

    /* Add encrypted flag */
    message = g_strconcat("[", fish_modes[mode], "] ", word_eol[1], NULL);

    /* Display message */
    hexchat_emit_print(ph, "Your Message", hexchat_get_info(ph, "nick"), message, prefix, NULL);
    g_free(message);

    /* Send encrypted messages */
    encrypted_item = encrypted_list;
    while (encrypted_item)
    {
        hexchat_commandf(ph, "%s%s", command->str, (char *)encrypted_item->data);

        encrypted_item = encrypted_item->next;
    }

    g_free(prefix);
    g_string_free(command, TRUE);
    g_slist_free_full(encrypted_list, g_free);

    return HEXCHAT_EAT_HEXCHAT;
}

/**
 * Called when a channel message or private message is received.
 */
static int handle_incoming(char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata) {
    const char *prefix;
    const char *command;
    const char *recipient;
    const char *raw_message = word_eol[1];
    char *sender_nick;
    char *decrypted;
    size_t parameters_offset;
    GString *message;

    if (!irc_parse_message((const char **)word, &prefix, &command, &parameters_offset))
        return HEXCHAT_EAT_NONE;

    /* Topic (command 332) has an extra parameter */
    if (!strcmp(command, "332"))
        parameters_offset++;

    /* Extract sender nick and recipient nick/channel and try to decrypt */
    recipient = word[parameters_offset];
    decrypted = decrypt_raw_message(raw_message, recipient);
    if (decrypted == NULL) {
        sender_nick = irc_prefix_get_nick(prefix);
        decrypted = decrypt_raw_message(raw_message, sender_nick);
        g_free(sender_nick);
    }

    /* Nothing to decrypt */
    if (decrypted == NULL)
        return HEXCHAT_EAT_NONE;

    /* Build decrypted message */

    /* decrypted + 'RECV ' + '@time=YYYY-MM-DDTHH:MM:SS.fffffZ ' */
    message = g_string_sized_new (strlen(decrypted) + 5 + 33);
    g_string_append (message, "RECV ");

    if (attrs->server_time_utc)
    {
        GTimeVal tv = { (glong)attrs->server_time_utc, 0 };
        char *timestamp = g_time_val_to_iso8601 (&tv);

       g_string_append (message, "@time=");
       g_string_append (message, timestamp);
       g_string_append (message, " ");
       g_free (timestamp);
    }

    g_string_append (message, decrypted);
    g_free(decrypted);

    /* Fake server message
     * RECV command will throw this function again, if message have multiple
     * encrypted data, we will decrypt all */
    hexchat_command(ph, message->str);
    g_string_free (message, TRUE);

    return HEXCHAT_EAT_HEXCHAT;
}

static int handle_keyx_notice(char *word[], char *word_eol[], void *userdata) {
    const char *dh_message = word[4];
    const char *dh_pubkey = word[5];
    hexchat_context *query_ctx;
    const char *prefix;
    char *sender, *secret_key, *priv_key = NULL;
    enum fish_mode mode = FISH_ECB_MODE;

    if (!*dh_message || !*dh_pubkey || strlen(dh_pubkey) != 181)
        return HEXCHAT_EAT_NONE;

    if (!irc_parse_message((const char**)word, &prefix, NULL, NULL) || !prefix)
        return HEXCHAT_EAT_NONE;

    sender = irc_prefix_get_nick(prefix);
    query_ctx = find_context_on_network(sender);
    if (query_ctx)
        g_assert(hexchat_set_context(ph, query_ctx) == 1);

    dh_message++; /* : prefix */

    if (g_strcmp0 (word[6], "CBC") == 0)
        mode = FISH_CBC_MODE;

    if (!strcmp(dh_message, "DH1080_INIT")) {
        char *pub_key;

        hexchat_printf(ph, "Received public key from %s (%s), sending mine...", sender, fish_modes[mode]);
        if (dh1080_generate_key(&priv_key, &pub_key)) {
            hexchat_commandf(ph, "quote NOTICE %s :DH1080_FINISH %s%s", sender, pub_key, (mode == FISH_CBC_MODE) ? " CBC" : "");
            g_free(pub_key);
        } else {
            hexchat_print(ph, "Failed to generate keys");
            goto cleanup;
        }
    } else if (!strcmp (dh_message, "DH1080_FINISH")) {
        char *sender_lower = g_ascii_strdown(sender, -1);
        /* FIXME: Properly respect irc casing */
        priv_key = g_hash_table_lookup(pending_exchanges, sender_lower);
        g_hash_table_steal(pending_exchanges, sender_lower);
        g_free(sender_lower);

        if (!priv_key) {
            hexchat_printf(ph, "Received a key exchange response for unknown user: %s", sender);
            goto cleanup;
        }
    } else {
        /* Regular notice */
        g_free(sender);
        return HEXCHAT_EAT_NONE;
    }

    if (dh1080_compute_key(priv_key, dh_pubkey, &secret_key)) {
        keystore_store_key(sender, secret_key, mode);
        hexchat_printf(ph, "Stored new key for %s (%s)", sender, fish_modes[mode]);
        g_free(secret_key);
    } else {
        hexchat_print(ph, "Failed to create secret key!");
    }

cleanup:
    g_free(sender);
    g_free(priv_key);
    return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /setkey
 */
static int handle_setkey(char *word[], char *word_eol[], void *userdata) {
    const char *nick;
    const char *key;
    enum fish_mode mode;

    /* Check syntax */
    if (*word[2] == '\0') {
        hexchat_printf(ph, "%s\n", usage_setkey);
        return HEXCHAT_EAT_HEXCHAT;
    }

    if (*word[3] == '\0') {
        /* /setkey password */
        nick = hexchat_get_info(ph, "channel");
        key = word_eol[2];
    } else {
        /* /setkey #channel password */
        nick = word[2];
        key = word_eol[3];
    }

    mode = FISH_ECB_MODE;
    if (g_ascii_strncasecmp("cbc:", key, 4) == 0) {
        key = key+4;
        mode = FISH_CBC_MODE;
    } else if (g_ascii_strncasecmp("ecb:", key, 4) == 0) {
        key = key+4;
    }

    /* Set password */
    if (keystore_store_key(nick, key, mode)) {
        hexchat_printf(ph, "Stored key for %s (%s)\n", nick, fish_modes[mode]);
    } else {
        hexchat_printf(ph, "\00305Failed to store key in addon_fishlim.conf\n");
    }

    return HEXCHAT_EAT_HEXCHAT;
}

/**
 * Command handler for /delkey
 */
static int handle_delkey(char *word[], char *word_eol[], void *userdata) {
    char *nick = NULL;
    int ctx_type = 0;

    /* Delete key from input */
    if (*word[2] != '\0') {
        nick = g_strstrip(g_strdup(word_eol[2]));
    } else { /* Delete key from current context */
        nick = g_strdup(hexchat_get_info(ph, "channel"));
        ctx_type = hexchat_list_int(ph, NULL, "type");

        /* Only allow channel or dialog */
        if (ctx_type < 2 || ctx_type > 3) {
            hexchat_printf(ph, "%s\n", usage_delkey);
            return HEXCHAT_EAT_HEXCHAT;
        }
    }

    /* Delete the given nick from the key store */
    if (keystore_delete_nick(nick)) {
        hexchat_printf(ph, "Deleted key for %s\n", nick);
    } else {
        hexchat_printf(ph, "\00305Failed to delete key in addon_fishlim.conf!\n");
    }
    g_free(nick);

    return HEXCHAT_EAT_HEXCHAT;
}

static int handle_keyx(char *word[], char *word_eol[], void *userdata) {
    const char *target = word[2];
    hexchat_context *query_ctx = NULL;
    char *pub_key, *priv_key;
    int ctx_type;

    if (*target)
        query_ctx = find_context_on_network(target);
    else {
        target = hexchat_get_info(ph, "channel");
        query_ctx = hexchat_get_context (ph);
    }

    if (query_ctx) {
        g_assert(hexchat_set_context(ph, query_ctx) == 1);
        ctx_type = hexchat_list_int(ph, NULL, "type");
    }

    if ((query_ctx && ctx_type != 3) || (!query_ctx && !irc_is_query(target))) {
        hexchat_print(ph, "You can only exchange keys with individuals");
        return HEXCHAT_EAT_ALL;
    }

    if (dh1080_generate_key(&priv_key, &pub_key)) {
        g_hash_table_replace (pending_exchanges, g_ascii_strdown(target, -1), priv_key);

        hexchat_commandf(ph, "quote NOTICE %s :DH1080_INIT %s CBC", target, pub_key);
        hexchat_printf(ph, "Sent public key to %s (CBC), waiting for reply...", target);

        g_free(pub_key);
    } else {
        hexchat_print(ph, "Failed to generate keys");
    }

    return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /topic+
 */
static int handle_crypt_topic(char *word[], char *word_eol[], void *userdata) {
    const char *target;
    const char *topic = word_eol[2];
    enum fish_mode mode;
    GString *command;
    GSList *encrypted_list;

    if (!*topic) {
        hexchat_print(ph, usage_topic);
        return HEXCHAT_EAT_ALL;
    }

    if (hexchat_list_int(ph, NULL, "type") != 2) {
        hexchat_printf(ph, "Please change to the channel window where you want to set the topic!");
        return HEXCHAT_EAT_ALL;
    }

    target = hexchat_get_info(ph, "channel");

    /* Check if we can encrypt */
    if (!fish_nick_has_key(target)) {
        hexchat_printf(ph, "/topic+ error, no key found for %s", target);
        return HEXCHAT_EAT_ALL;
    }

    command = g_string_new("");
    g_string_printf(command, "TOPIC %s +OK ", target);

    encrypted_list = fish_encrypt_for_nick(target, topic, &mode, get_prefix_length() + command->len);
    if (!encrypted_list) {
        g_string_free(command, TRUE);
        hexchat_printf(ph, "/topic+ error, can't encrypt %s", target);
        return HEXCHAT_EAT_ALL;
    }

    hexchat_commandf(ph, "%s%s", command->str, (char *) encrypted_list->data);

    g_string_free(command, TRUE);
    g_slist_free_full(encrypted_list, g_free);

    return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /notice+
 */
static int handle_crypt_notice(char *word[], char *word_eol[], void *userdata) {
    const char *target = word[2];
    const char *notice = word_eol[3];
    char *notice_flag;
    enum fish_mode mode;
    GString *command;
    GSList *encrypted_list, *encrypted_item;

    if (!*target || !*notice) {
        hexchat_print(ph, usage_notice);
        return HEXCHAT_EAT_ALL;
    }

    /* Check if we can encrypt */
    if (!fish_nick_has_key(target)) {
        hexchat_printf(ph, "/notice+ error, no key found for %s.", target);
        return HEXCHAT_EAT_ALL;
    }

    command = g_string_new("");
    g_string_printf(command, "quote NOTICE %s :+OK ", target);

    encrypted_list = fish_encrypt_for_nick(target, notice, &mode, get_prefix_length() + command->len);
    if (!encrypted_list) {
        g_string_free(command, TRUE);
        hexchat_printf(ph, "/notice+ error, can't encrypt %s", target);
        return HEXCHAT_EAT_ALL;
    }

    notice_flag = g_strconcat("[", fish_modes[mode], "] ", notice, NULL);
    hexchat_emit_print(ph, "Notice Send", target, notice_flag);

    /* Send encrypted messages */
    encrypted_item = encrypted_list;
    while (encrypted_item) {
        hexchat_commandf(ph, "%s%s", command->str, (char *) encrypted_item->data);

        encrypted_item = encrypted_item->next;
    }

    g_free(notice_flag);
    g_string_free(command, TRUE);
    g_slist_free_full(encrypted_list, g_free);

    return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /msg+
 */
static int handle_crypt_msg(char *word[], char *word_eol[], void *userdata) {
    const char *target = word[2];
    const char *message = word_eol[3];
    char *message_flag;
    char *prefix;
    hexchat_context *query_ctx;
    enum fish_mode mode;
    GString *command;
    GSList *encrypted_list, *encrypted_item;

    if (!*target || !*message) {
        hexchat_print(ph, usage_msg);
        return HEXCHAT_EAT_ALL;
    }

    /* Check if we can encrypt */
    if (!fish_nick_has_key(target)) {
        hexchat_printf(ph, "/msg+ error, no key found for %s", target);
        return HEXCHAT_EAT_ALL;
    }

    command = g_string_new("");
    g_string_printf(command, "PRIVMSG %s :+OK ", target);

    encrypted_list = fish_encrypt_for_nick(target, message, &mode, get_prefix_length() + command->len);
    if (!encrypted_list) {
        g_string_free(command, TRUE);
        hexchat_printf(ph, "/msg+ error, can't encrypt %s", target);
        return HEXCHAT_EAT_ALL;
    }

    /* Send encrypted messages */
    encrypted_item = encrypted_list;
    while (encrypted_item) {
        hexchat_commandf(ph, "%s%s", command->str, (char *) encrypted_item->data);

        encrypted_item = encrypted_item->next;
    }

    g_string_free(command, TRUE);
    g_slist_free_full(encrypted_list, g_free);

    query_ctx = find_context_on_network(target);
    if (query_ctx) {
        g_assert(hexchat_set_context(ph, query_ctx) == 1);

        prefix = get_my_own_prefix();

        /* Add encrypted flag */
        message_flag = g_strconcat("[", fish_modes[mode], "] ", message, NULL);
        hexchat_emit_print(ph, "Your Message", hexchat_get_info(ph, "nick"), message_flag, prefix, NULL);
        g_free(prefix);
        g_free(message_flag);
    } else {
        hexchat_emit_print(ph, "Message Send", target, message);
    }

    return HEXCHAT_EAT_ALL;
}

static int handle_crypt_me(char *word[], char *word_eol[], void *userdata) {
    const char *channel = hexchat_get_info(ph, "channel");
    enum fish_mode mode;
    GString *command;
    GSList *encrypted_list, *encrypted_item;

    /* Check if we can encrypt */
    if (!fish_nick_has_key(channel)) {
        return HEXCHAT_EAT_NONE;
    }

    command = g_string_new("");
    g_string_printf(command, "PRIVMSG %s :\001ACTION +OK ", channel);

    /* 2 = ' \001' */
    encrypted_list = fish_encrypt_for_nick(channel, word_eol[2], &mode, get_prefix_length() + command->len + 2);
    if (!encrypted_list) {
        g_string_free(command, TRUE);
        hexchat_printf(ph, "/me error, can't encrypt %s", channel);
        return HEXCHAT_EAT_ALL;
    }

    hexchat_emit_print(ph, "Your Action", hexchat_get_info(ph, "nick"), word_eol[2], NULL);

    /* Send encrypted messages */
    encrypted_item = encrypted_list;
    while (encrypted_item) {
        hexchat_commandf(ph, "%s%s \001", command->str, (char *) encrypted_item->data);

        encrypted_item = encrypted_item->next;
    }

    g_string_free(command, TRUE);
    g_slist_free_full(encrypted_list, g_free);

    return HEXCHAT_EAT_ALL;
}

/**
 * Returns the plugin name version information.
 */
void hexchat_plugin_get_info(const char **name, const char **desc,
                           const char **version, void **reserved) {
    *name = plugin_name;
    *desc = plugin_desc;
    *version = plugin_version;
}

/**
 * Plugin entry point.
 */
int hexchat_plugin_init(hexchat_plugin *plugin_handle,
                      const char **name,
                      const char **desc,
                      const char **version,
                      char *arg) {
    ph = plugin_handle;

    /* Send our info to HexChat */
    *name = plugin_name;
    *desc = plugin_desc;
    *version = plugin_version;

    /* Register commands */
    hexchat_hook_command(ph, "SETKEY", HEXCHAT_PRI_NORM, handle_setkey, usage_setkey, NULL);
    hexchat_hook_command(ph, "DELKEY", HEXCHAT_PRI_NORM, handle_delkey, usage_delkey, NULL);
    hexchat_hook_command(ph, "KEYX", HEXCHAT_PRI_NORM, handle_keyx, usage_keyx, NULL);
    hexchat_hook_command(ph, "TOPIC+", HEXCHAT_PRI_NORM, handle_crypt_topic, usage_topic, NULL);
    hexchat_hook_command(ph, "NOTICE+", HEXCHAT_PRI_NORM, handle_crypt_notice, usage_notice, NULL);
    hexchat_hook_command(ph, "MSG+", HEXCHAT_PRI_NORM, handle_crypt_msg, usage_msg, NULL);
    hexchat_hook_command(ph, "ME", HEXCHAT_PRI_NORM, handle_crypt_me, NULL, NULL);

    /* Add handlers */
    hexchat_hook_command(ph, "", HEXCHAT_PRI_NORM, handle_outgoing, NULL, NULL);
    hexchat_hook_server(ph, "NOTICE", HEXCHAT_PRI_HIGHEST, handle_keyx_notice, NULL);
    hexchat_hook_server_attrs(ph, "NOTICE", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    hexchat_hook_server_attrs(ph, "PRIVMSG", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    /* hexchat_hook_server(ph, "RAW LINE", HEXCHAT_PRI_NORM, handle_debug, NULL); */
    hexchat_hook_server_attrs(ph, "TOPIC", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    hexchat_hook_server_attrs(ph, "332", HEXCHAT_PRI_NORM, handle_incoming, NULL);

    if (!fish_init())
        return 0;

    if (!dh1080_init())
        return 0;

    pending_exchanges = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    hexchat_printf(ph, "%s plugin loaded\n", plugin_name);
    /* Return success */
    return 1;
}

int hexchat_plugin_deinit(void) {
    g_clear_pointer(&pending_exchanges, g_hash_table_destroy);
    dh1080_deinit();
    fish_deinit();

    hexchat_printf(ph, "%s plugin unloaded\n", plugin_name);
    return 1;
}

