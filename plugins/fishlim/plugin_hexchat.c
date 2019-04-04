/*

  Copyright (c) 2010-2011 Samuel Lidén Borell <samuel@kodafritt.se>
  Copyright (c) 2015 <the.cypher@gmail.com>

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
#define HEXCHAT_MAX_WORDS 32

#include "fish.h"
#include "dh1080.h"
#include "keystore.h"
#include "irc.h"

static const char plugin_name[] = "FiSHLiM";
static const char plugin_desc[] = "Encryption plugin for the FiSH protocol. Less is More!";
static const char plugin_version[] = "0.1.0";

static const char usage_setkey[] = "Usage: SETKEY [<nick or #channel>] <password>, sets the key for a channel or nick";
static const char usage_delkey[] = "Usage: DELKEY <nick or #channel>, deletes the key for a channel or nick";
static const char usage_keyx[] = "Usage: KEYX [<nick>], performs DH1080 key-exchange with <nick>";
static const char usage_topic[] = "Usage: TOPIC+ <topic>, sets a new encrypted topic for the current channel";
static const char usage_notice[] = "Usage: NOTICE+ <nick or #channel> <notice>";
static const char usage_msg[] = "Usage: MSG+ <nick or #channel> <message>";


static hexchat_plugin *ph;
static GHashTable *pending_exchanges;


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

        if (chan_id == id && chan_name && hexchat_nickcmp (ph, chan_name, name) == 0) {
            ret = (hexchat_context*)hexchat_list_str(ph, channels, "context");
            break;
        }
    };

    hexchat_list_free(ph, channels);
    return ret;
}

int irc_nick_cmp(const char *a, const char *b) {
	return hexchat_nickcmp (ph, a, b);
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
    const char *own_nick;
    /* Encrypt the message if possible */
    const char *channel = hexchat_get_info(ph, "channel");
    char *encrypted = fish_encrypt_for_nick(channel, word_eol[1]);
    if (!encrypted) return HEXCHAT_EAT_NONE;
    
    /* Display message */
    own_nick = hexchat_get_info(ph, "nick");
    hexchat_emit_print(ph, "Your Message", own_nick, word_eol[1], NULL);
    
    /* Send message */
    hexchat_commandf(ph, "PRIVMSG %s :+OK %s", channel, encrypted);
    
    g_free(encrypted);
    return HEXCHAT_EAT_HEXCHAT;
}

/**
 * Called when a channel message or private message is received.
 */
static int handle_incoming(char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata) {
    const char *prefix;
    const char *command;
    const char *recipient;
    const char *encrypted;
    const char *peice;
    char *sender_nick;
    char *decrypted;
    size_t w;
    size_t ew;
    size_t uw;
    char prefix_char = 0;
    GString *message;

    if (!irc_parse_message((const char **)word, &prefix, &command, &w))
        return HEXCHAT_EAT_NONE;
    
    /* Topic (command 332) has an extra parameter */
    if (!strcmp(command, "332")) w++;
    
    /* Look for encrypted data */
    for (ew = w+1; ew < HEXCHAT_MAX_WORDS-1; ew++) {
        const char *s = (ew == w+1 ? word[ew]+1 : word[ew]);
        if (*s && (s[1] == '+' || s[1] == 'm')) { prefix_char = *(s++); }
        else { prefix_char = 0; }
        if (strcmp(s, "+OK") == 0 || strcmp(s, "mcps") == 0) goto has_encrypted_data;
    }
    return HEXCHAT_EAT_NONE;
  has_encrypted_data: ;
    /* Extract sender nick and recipient nick/channel */
    sender_nick = irc_prefix_get_nick(prefix);
    recipient = word[w];
    
    /* Try to decrypt with these (the keys are searched for in the key store) */
    encrypted = word[ew+1];
    decrypted = fish_decrypt_from_nick(recipient, encrypted);
    if (!decrypted) decrypted = fish_decrypt_from_nick(sender_nick, encrypted);
    
    /* Check for error */
    if (!decrypted) goto decrypt_error;
    
    /* Build unecrypted message */
    message = g_string_sized_new (100); /* TODO: more accurate estimation of size */
    g_string_append (message, "RECV");

    if (attrs->server_time_utc)
    {
        GTimeVal tv = { (glong)attrs->server_time_utc, 0 };
        char *timestamp = g_time_val_to_iso8601 (&tv);

       g_string_append (message, " @time=");
       g_string_append (message, timestamp);
       g_free (timestamp);
    }

    for (uw = 1; uw < HEXCHAT_MAX_WORDS; uw++) {
        if (word[uw][0] != '\0')
            g_string_append_c (message, ' ');
        
        if (uw == ew) {
            /* Add the encrypted data */
            peice = decrypted;
            uw++; /* Skip "OK+" */
            
            if (ew == w+1) {
                /* Prefix with colon, which gets stripped out otherwise */
                g_string_append_c (message, ':');
            }

            if (prefix_char) {
                g_string_append_c (message, prefix_char);
            }

        } else {
            /* Add unencrypted data (for example, a prefix from a bouncer or bot) */
            peice = word[uw];
        }

        g_string_append (message, peice);
    }
    g_free(decrypted);

    /* Simulate unencrypted message */
    /* hexchat_printf(ph, "simulating: %s\n", message->str); */
    hexchat_command(ph, message->str);

    g_string_free (message, TRUE);
    g_free(sender_nick);
    return HEXCHAT_EAT_HEXCHAT;

  decrypt_error:
    g_free(decrypted);
    g_free(sender_nick);
    return HEXCHAT_EAT_NONE;
}

static int handle_keyx_notice(char *word[], char *word_eol[], void *userdata) {
    const char *dh_message = word[4];
    const char *dh_pubkey = word[5];
    hexchat_context *query_ctx;
    const char *prefix;
    gboolean cbc;
    char *sender, *secret_key, *priv_key = NULL;

    if (!*dh_message || !*dh_pubkey || strlen(dh_pubkey) != 181)
        return HEXCHAT_EAT_NONE;

    if (!irc_parse_message((const char**)word, &prefix, NULL, NULL) || !prefix)
        return HEXCHAT_EAT_NONE;

    sender = irc_prefix_get_nick(prefix);
    query_ctx = find_context_on_network(sender);
    if (query_ctx)
        hexchat_set_context(ph, query_ctx);

    dh_message++; /* : prefix */
    if (*dh_message == '+' || *dh_message == '-')
        dh_message++; /* identify-msg */

    cbc = g_strcmp0 (word[6], "CBC") == 0;

    if (!strcmp(dh_message, "DH1080_INIT")) {
        char *pub_key;

        if (cbc) {
            hexchat_print(ph, "Received key exchange for CBC mode which is not supported.");
            goto cleanup;
        }

        hexchat_printf(ph, "Received public key from %s, sending mine...", sender);
        if (dh1080_generate_key(&priv_key, &pub_key)) {
            hexchat_commandf(ph, "quote NOTICE %s :DH1080_FINISH %s", sender, pub_key);
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

        if (cbc) {
            hexchat_print(ph, "Received key exchange for CBC mode which is not supported.");
            goto cleanup;
        }

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
        keystore_store_key(sender, secret_key);
        hexchat_printf(ph, "Stored new key for %s", sender);
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

    /* Set password */
    if (keystore_store_key(nick, key)) {
        hexchat_printf(ph, "Stored key for %s\n", nick);
    } else {
        hexchat_printf(ph, "\00305Failed to store key in addon_fishlim.conf\n");
    }

    return HEXCHAT_EAT_HEXCHAT;
}

/**
 * Command handler for /delkey
 */
static int handle_delkey(char *word[], char *word_eol[], void *userdata) {
    const char *nick;

    /* Check syntax */
    if (*word[2] == '\0' || *word[3] != '\0') {
        hexchat_printf(ph, "%s\n", usage_delkey);
        return HEXCHAT_EAT_HEXCHAT;
    }

    nick = g_strstrip (word_eol[2]);

    /* Delete the given nick from the key store */
    if (keystore_delete_nick(nick)) {
        hexchat_printf(ph, "Deleted key for %s\n", nick);
    } else {
        hexchat_printf(ph, "\00305Failed to delete key in addon_fishlim.conf!\n");
    }

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
        hexchat_set_context(ph, query_ctx);
        ctx_type = hexchat_list_int(ph, NULL, "type");
    }

    if ((query_ctx && ctx_type != 3) || (!query_ctx && !irc_is_query(target))) {
        hexchat_print(ph, "You can only exchange keys with individuals");
        return HEXCHAT_EAT_ALL;
    }

    if (dh1080_generate_key(&priv_key, &pub_key)) {
        g_hash_table_replace (pending_exchanges, g_ascii_strdown(target, -1), priv_key);

        hexchat_commandf(ph, "quote NOTICE %s :DH1080_INIT %s", target, pub_key);
        hexchat_printf(ph, "Sent public key to %s, waiting for reply...", target);

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
    char *buf;

    if (!*topic) {
        hexchat_print(ph, usage_topic);
        return HEXCHAT_EAT_ALL;
    }

    if (hexchat_list_int(ph, NULL, "type") != 2) {
        hexchat_printf(ph, "Please change to the channel window where you want to set the topic!");
        return HEXCHAT_EAT_ALL;
    }

    target = hexchat_get_info(ph, "channel");
    buf = fish_encrypt_for_nick(target, topic);
    if (buf == NULL) {
        hexchat_printf(ph, "/topic+ error, no key found for %s", target);
        return HEXCHAT_EAT_ALL;
    }

    hexchat_commandf(ph, "TOPIC %s +OK %s", target, buf);
    g_free(buf);
    return HEXCHAT_EAT_ALL;
    }

/**
 * Command handler for /notice+
 */
static int handle_crypt_notice(char *word[], char *word_eol[], void *userdata)
{
    const char *target = word[2];
    const char *notice = word_eol[3];
    char *buf;

    if (!*target || !*notice) {
        hexchat_print(ph, usage_notice);
        return HEXCHAT_EAT_ALL;
    }

    buf = fish_encrypt_for_nick(target, notice);
    if (buf == NULL) {
        hexchat_printf(ph, "/notice+ error, no key found for %s.", target);
        return HEXCHAT_EAT_ALL;
    }

    hexchat_commandf(ph, "quote NOTICE %s :+OK %s", target, buf);
    hexchat_emit_print(ph, "Notice Sent", target, notice);
    g_free(buf);

    return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /msg+
 */
static int handle_crypt_msg(char *word[], char *word_eol[], void *userdata)
{
    const char *target = word[2];
    const char *message = word_eol[3];
    hexchat_context *query_ctx;
    char *buf;

    if (!*target || !*message) {
        hexchat_print(ph, usage_msg);
        return HEXCHAT_EAT_ALL;
    }

    buf = fish_encrypt_for_nick(target, message);
    if (buf == NULL) {
        hexchat_printf(ph, "/msg+ error, no key found for %s", target);
        return HEXCHAT_EAT_ALL;
    }

    hexchat_commandf(ph, "PRIVMSG %s :+OK %s", target, buf);

    query_ctx = find_context_on_network(target);
    if (query_ctx) {
	    hexchat_set_context(ph, query_ctx);

        /* FIXME: Mode char */
        hexchat_emit_print(ph, "Your Message", hexchat_get_info(ph, "nick"),
                           message, "", "");
    }
    else {
        hexchat_emit_print(ph, "Message Send", target, message);
    }

    g_free(buf);
    return HEXCHAT_EAT_ALL;
}

static int handle_crypt_me(char *word[], char *word_eol[], void *userdata) {
	const char *channel = hexchat_get_info(ph, "channel");
	char *buf;

    buf = fish_encrypt_for_nick(channel, word_eol[2]);
	if (!buf)
        return HEXCHAT_EAT_NONE;

	hexchat_commandf(ph, "PRIVMSG %s :\001ACTION +OK %s \001", channel, buf);
	hexchat_emit_print(ph, "Your Action", hexchat_get_info(ph, "nick"),
                       word_eol[2], NULL);

	g_free(buf);
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

    hexchat_printf(ph, "%s plugin unloaded\n", plugin_name);
    return 1;
}

