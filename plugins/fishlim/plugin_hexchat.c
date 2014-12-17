/*

  Copyright (c) 2010-2011 Samuel Lidén Borell <samuel@kodafritt.se>

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
#include "keystore.h"
#include "irc.h"

static const char plugin_name[] = "FiSHLiM";
static const char plugin_desc[] = "Encryption plugin for the FiSH protocol. Less is More!";
static const char plugin_version[] = "0.0.17";

static const char usage_setkey[] = "Usage: SETKEY [<nick or #channel>] <password>, sets the key for a channel or nick";
static const char usage_delkey[] = "Usage: DELKEY <nick or #channel>, deletes the key for a channel or nick";

static hexchat_plugin *ph;


/**
 * Returns the path to the key store file.
 */
gchar *get_config_filename() {
    char *filename_fs, *filename_utf8;

    filename_utf8 = g_build_filename(hexchat_get_info(ph, "configdir"), "addon_fishlim.conf", NULL);
    filename_fs = g_filename_from_utf8 (filename_utf8, -1, NULL, NULL, NULL);

    g_free (filename_utf8);
    return filename_fs;
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
    
    /* Add handlers */
    hexchat_hook_command(ph, "", HEXCHAT_PRI_NORM, handle_outgoing, NULL, NULL);
    hexchat_hook_server_attrs(ph, "NOTICE", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    hexchat_hook_server_attrs(ph, "PRIVMSG", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    /* hexchat_hook_server(ph, "RAW LINE", HEXCHAT_PRI_NORM, handle_debug, NULL); */
    hexchat_hook_server_attrs(ph, "TOPIC", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    hexchat_hook_server_attrs(ph, "332", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    
    hexchat_printf(ph, "%s plugin loaded\n", plugin_name);
    /* Return success */
    return 1;
}

int hexchat_plugin_deinit(void) {
    hexchat_printf(ph, "%s plugin unloaded\n", plugin_name);
    return 1;
}

