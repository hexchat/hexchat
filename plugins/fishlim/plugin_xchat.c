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

#include <glib.h>
#include <stdlib.h>
#include <string.h>

// #pragma GCC visibility push(default)
#ifdef _MSC_VER
#include "xchat-plugin.h"
#else
#include <xchat/xchat-plugin.h>
#endif
#define XCHAT_MAX_WORDS 32
// #pragma GCC visibility pop

//#define EXPORT __attribute((visibility("default")))
//#define EXPORT

#include "fish.h"
#include "keystore.h"
#include "irc.h"

static const char plugin_name[] = "FiSHLiM";
static const char plugin_desc[] = "Encryption plugin for the FiSH protocol. Less is More!";
static const char plugin_version[] = "0.0.16";

static const char usage_setkey[] = "Usage: SETKEY [<nick or #channel>] <password>, sets the key for a channel or nick";
static const char usage_delkey[] = "Usage: DELKEY <nick or #channel>, deletes the key for a channel or nick";

static xchat_plugin *ph;


/**
 * Returns the path to the key store file.
 */
gchar *get_config_filename() {
    return g_build_filename(xchat_get_info(ph, "xchatdirfs"), "blow.ini", NULL);
}

/**
 * Appends data to a string. Returns true if there was sufficient memory.
 * Frees *s and returns false if an error occurs.
 */
static bool append(char **s, size_t *length, const char *data) {
    size_t datalen = strlen(data);
    char *extended = realloc(*s, *length + datalen + 1);
    if (!extended) {
        free(*s);
        return false;
    }
    memcpy(extended + *length, data, datalen + 1);
    *s = extended;
    *length += datalen;
    return true;
}


/*static int handle_debug(char *word[], char *word_eol[], void *userdata) {
    xchat_printf(ph, "debug incoming: ");
    for (size_t i = 1; word[i] != NULL && word[i][0] != '\0'; i++) {
        xchat_printf(ph, ">%s< ", word[i]);
    }
    xchat_printf(ph, "\n");
    return XCHAT_EAT_NONE;
}*/

/**
 * Called when a message is to be sent.
 */
static int handle_outgoing(char *word[], char *word_eol[], void *userdata) {
    const char *own_nick;
    // Encrypt the message if possible
    const char *channel = xchat_get_info(ph, "channel");
    char *encrypted = fish_encrypt_for_nick(channel, word_eol[1]);
    if (!encrypted) return XCHAT_EAT_NONE;
    
    // Display message
    own_nick = xchat_get_info(ph, "nick");
    xchat_emit_print(ph, "Your Message", own_nick, word_eol[1], NULL);
    
    // Send message
    xchat_commandf(ph, "PRIVMSG %s :+OK %s", channel, encrypted);
    
    free(encrypted);
    return XCHAT_EAT_XCHAT;
}

/**
 * Called when a channel message or private message is received.
 */
static int handle_incoming(char *word[], char *word_eol[], void *userdata) {
    const char *prefix;
    const char *command;
    const char *recipient;
    const char *encrypted;
    const char *peice;
    char *sender_nick;
    char *decrypted;
    char *message;
    size_t w;
    size_t ew;
    size_t uw;
    size_t length;

    if (!irc_parse_message((const char **)word, &prefix, &command, &w))
        return XCHAT_EAT_NONE;
    
    // Topic (command 332) has an extra parameter
    if (!strcmp(command, "332")) w++;
    
    // Look for encrypted data
    for (ew = w+1; ew < XCHAT_MAX_WORDS-1; ew++) {
        const char *s = (ew == w+1 ? word[ew]+1 : word[ew]);
        if (strcmp(s, "+OK") == 0 || strcmp(s, "mcps") == 0) goto has_encrypted_data;
    }
    return XCHAT_EAT_NONE;
  has_encrypted_data: ;
    // Extract sender nick and recipient nick/channel
    sender_nick = irc_prefix_get_nick(prefix);
    recipient = word[w];
    
    // Try to decrypt with these (the keys are searched for in the key store)
    encrypted = word[ew+1];
    decrypted = fish_decrypt_from_nick(recipient, encrypted);
    if (!decrypted) decrypted = fish_decrypt_from_nick(sender_nick, encrypted);
    
    // Check for error
    if (!decrypted) goto decrypt_error;
    
    // Build unecrypted message
    message = NULL;
    length = 0;
    if (!append(&message, &length, "RECV")) goto decrypt_error;
    
    for (uw = 1; uw < XCHAT_MAX_WORDS; uw++) {
        if (word[uw][0] != '\0' && !append(&message, &length, " ")) goto decrypt_error;
        
        if (uw == ew) {
            // Add the encrypted data
            peice = decrypted;
            uw++; // Skip "OK+"
            
            if (ew == w+1) {
                // Prefix with colon, which gets stripped out otherwise
                if (!append(&message, &length, ":")) goto decrypt_error;
            }
            
        } else {
            // Add unencrypted data (for example, a prefix from a bouncer or bot)
            peice = word[uw];
        }
        
        if (!append(&message, &length, peice)) goto decrypt_error;
    }
    free(decrypted);
    
    // Simulate unencrypted message
    //xchat_printf(ph, "simulating: %s\n", message);
    xchat_command(ph, message);
    
    free(message);
    free(sender_nick);
    return XCHAT_EAT_XCHAT;
  
  decrypt_error:
    free(decrypted);
    free(sender_nick);
    return XCHAT_EAT_NONE;
}

/**
 * Command handler for /setkey
 */
static int handle_setkey(char *word[], char *word_eol[], void *userdata) {
    const char *nick;
    const char *key;
    
    // Check syntax
    if (*word[2] == '\0') {
        xchat_printf(ph, "%s\n", usage_setkey);
        return XCHAT_EAT_XCHAT;
    }
    
    if (*word[3] == '\0') {
        // /setkey password
        nick = xchat_get_info(ph, "channel");
        key = word_eol[2];
    } else {
        // /setkey #channel password
        nick = word[2];
        key = word_eol[3];
    }
    
    // Set password
    if (keystore_store_key(nick, key)) {
        xchat_printf(ph, "Stored key for %s\n", nick);
    } else {
        xchat_printf(ph, "\00305Failed to store key in blow.ini\n", nick, key);
    }
    
    return XCHAT_EAT_XCHAT;
}

/**
 * Command handler for /delkey
 */
static int handle_delkey(char *word[], char *word_eol[], void *userdata) {
    const char *nick;
    
    // Check syntax
    if (*word[2] == '\0' || *word[3] != '\0') {
        xchat_printf(ph, "%s\n", usage_delkey);
        return XCHAT_EAT_XCHAT;
    }
    
    nick = word_eol[2];
    
    // Delete the given nick from the key store
    if (keystore_delete_nick(nick)) {
        xchat_printf(ph, "Deleted key for %s\n", nick);
    } else {
        xchat_printf(ph, "\00305Failed to delete key in blow.ini!\n", nick);
    }
    
    return XCHAT_EAT_XCHAT;
}

/**
 * Returns the plugin name version information.
 */
void xchat_plugin_get_info(const char **name, const char **desc,
                           const char **version, void **reserved) {
    *name = plugin_name;
    *desc = plugin_desc;
    *version = plugin_version;
}

/**
 * Plugin entry point.
 */
int xchat_plugin_init(xchat_plugin *plugin_handle,
                      const char **name,
                      const char **desc,
                      const char **version,
                      char *arg) {
    ph = plugin_handle;
    
    /* Send our info to XChat */
    *name = plugin_name;
    *desc = plugin_desc;
    *version = plugin_version;
    
    /* Register commands */
    xchat_hook_command(ph, "SETKEY", XCHAT_PRI_NORM, handle_setkey, usage_setkey, NULL);
    xchat_hook_command(ph, "DELKEY", XCHAT_PRI_NORM, handle_delkey, usage_delkey, NULL);
    
    /* Add handlers */
    xchat_hook_command(ph, "", XCHAT_PRI_NORM, handle_outgoing, NULL, NULL);
    xchat_hook_server(ph, "NOTICE", XCHAT_PRI_NORM, handle_incoming, NULL);
    xchat_hook_server(ph, "PRIVMSG", XCHAT_PRI_NORM, handle_incoming, NULL);
    //xchat_hook_server(ph, "RAW LINE", XCHAT_PRI_NORM, handle_debug, NULL);
    xchat_hook_server(ph, "TOPIC", XCHAT_PRI_NORM, handle_incoming, NULL);
    xchat_hook_server(ph, "332", XCHAT_PRI_NORM, handle_incoming, NULL);
    
    xchat_printf(ph, "%s plugin loaded\n", plugin_name);
    /* Return success */
    return 1;
}

int xchat_plugin_deinit(void) {
    xchat_printf(ph, "%s plugin unloaded\n", plugin_name);
    return 1;
}

