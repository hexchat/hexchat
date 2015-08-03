/*

  Copyright (c) 2010-2011 Samuel Lidén Borell <samuel@kodafritt.se> | /setkey /delkey and channel msg enc/decryption
				2015      <the.cypher@gmail.com>					| /keyx /topic+ /msg+ /notice+

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
#include "DH1080Wrapper.h"

static const char plugin_name[] = "FiSHLiM";
static const char plugin_desc[] = "Encryption plugin for the FiSH protocol. Less is More!";
static const char plugin_version[] = "0.1.0";

static const char usage_setkey[]	= "Usage: SETKEY [<nick or #channel>] <password>, sets the key for a channel or nick";
static const char usage_delkey[]	= "Usage: DELKEY <nick or #channel>, deletes the key for a channel or nick";
static const char usage_keyx[]		= "Usage: KEYX [<nick>], performs DH1080 key-exchange with <nick>";
static const char usage_topic[]		= "Usage: TOPIC+ <topic>, sets a new encrypted topic for the current channel";
static const char usage_notice[]	= "Usage: NOTICE+ <nick or #channel> <notice>";
static const char usage_msg[]		= "Usage: MSG+ <nick or #channel> <message>";

static hexchat_plugin *ph;

struct DH1080* dh;

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
            
if (ew == w + 1) {
	/* Prefix with colon, which gets stripped out otherwise */
	g_string_append_c(message, ':');
}

if (prefix_char) {
	g_string_append_c(message, prefix_char);
}

        } else {
	 /* Add unencrypted data (for example, a prefix from a bouncer or bot) */
	 peice = word[uw];
 }

 g_string_append(message, peice);
	}
	g_free(decrypted);

	/* Simulate unencrypted message */
	/* hexchat_printf(ph, "simulating: %s\n", message->str); */
	hexchat_command(ph, message->str);

	g_string_free(message, TRUE);
	g_free(sender_nick);
	return HEXCHAT_EAT_HEXCHAT;

decrypt_error:
	g_free(decrypted);
	g_free(sender_nick);
	return HEXCHAT_EAT_NONE;
}

/**
* Called when a server NOTICE is received
*/
static int handle_notice(char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata) {
	char* dh_msg = word[4];
	char* dh_pubkey = word[5];
	hexchat_context* query_ctx;
	const char* prefix;
	const char* command;
	char* sender_nick;
	size_t w;
	unsigned char contactName[25] = "";


	// verify we got sth
	if (dh_pubkey == NULL || *word[5] == '\0' || *word[4] == '\0' || *word[3] == '\0' || *word[1] == '\0')
		return HEXCHAT_EAT_NONE;

	// no DHINIT, forward to standard handler
	if (strcmp(word[4], ":+OK") == 0 || strcmp(word[4], ":mcps") == 0)
		return handle_incoming(word, word_eol, attrs, userdata);

	// DH sanity check
	if (strlen(dh_pubkey) != 181)
		return HEXCHAT_EAT_NONE;

	if (!irc_parse_message((const char **)word, &prefix, &command, &w))
		return HEXCHAT_EAT_NONE;
	sender_nick = irc_prefix_get_nick(prefix);

	// if a query is already open, display info text there
	query_ctx = hexchat_find_context(ph, NULL, sender_nick);
	if (query_ctx) hexchat_set_context(ph, query_ctx);

	// perform the actual key-exchange
	if (strncmp(dh_msg, ":DH1080_INIT", 12) == 0)
	{
		hexchat_printf(ph, "Received DH1080 public key from %s, sending mine...", sender_nick);
		hexchat_commandf(ph, "quote NOTICE %s :DH1080_FINISH %s", sender_nick, DH1080_getNewPublicKey(dh));
	}
	else if (strncmp(dh_msg, ":DH1080_FINISH", 14) != 0) {
		DH1080_flush(dh);
		return HEXCHAT_EAT_NONE;
	}

	keystore_store_key(sender_nick, DH1080_computeSymetricKey(dh, dh_pubkey));
	hexchat_printf(ph, "Key for %s successfully set!", sender_nick);
	return HEXCHAT_EAT_ALL;
}

/**
* Command handler for /keyx
*/
static int handle_keyx(char *word[], char *word_eol[], void *userdata) {
	char* target = word[2];
	char* getinfoPtr;
	hexchat_context *query_ctx;

	if (target == 0 || *target == 0)
	{
		// no paramter given - try current window
		target = (char *)hexchat_get_info(ph, "channel");
		getinfoPtr = hexchat_get_info(ph, "network");
		if (target == 0 || (getinfoPtr != 0 && stricmp(target, getinfoPtr) == 0))
		{
			hexchat_printf(ph, usage_keyx);
			return HEXCHAT_EAT_ALL;
		}
	}
	if (*target == '#' || *target == '&') {
		hexchat_printf(ph, "DH1080 Key exchange can't have a channel name as a target.\n");
		return HEXCHAT_EAT_ALL;
	}

	DH1080_flush(dh);
	query_ctx = hexchat_find_context(ph, NULL, target);
	if (query_ctx != 0) hexchat_set_context(ph, query_ctx);
	hexchat_commandf(ph, "quote NOTICE %s :DH1080_INIT %s", target, DH1080_getNewPublicKey(dh));
	hexchat_printf(ph, "Sent my DH1080 public key to %s, waiting for reply ...", target);

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

/**
 * Command handler for /topic+
 */
static int handle_crypt_topic(char *word[], char *word_eol[], void *userdata) {
	char* target;
	char* buf;
	char* topic = word_eol[2];


	if (topic == 0 || *topic == 0)
	{
		hexchat_printf(ph, usage_topic);
		return HEXCHAT_EAT_ALL;
	}

	target = (char *)hexchat_get_info(ph, "channel");
	if (target == 0 || (*target != '#' && *target != '&'))
	{
		hexchat_printf(ph, "Please change to the channel window where you want to set the topic!");
		return HEXCHAT_EAT_ALL;
	}

	buf = fish_encrypt_for_nick(target, topic);
	if (buf == NULL)
	{
		hexchat_printf(ph, "/topic+ error, no key found for %s", target);
		return HEXCHAT_EAT_ALL;
	}

	hexchat_commandf(ph, "TOPIC %s +OK %s\n", target, buf);

	return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /notice+
 */
int handle_crypt_notice(char *word[], char *word_eol[], void *userdata)
{
	char* buf;
	char *target = word[2];
	char *notice = word_eol[3];


	if (target == 0 || *target == 0 || notice == 0 || *notice == 0)
	{
		hexchat_printf(ph, usage_notice);
		return HEXCHAT_EAT_ALL;
	}

	buf = fish_encrypt_for_nick(target, notice);
	if (buf == NULL)
	{
		hexchat_printf(ph, "/notice+ error, no key found for %s.", target);
		return HEXCHAT_EAT_ALL;
	}

	hexchat_commandf(ph, "quote NOTICE %s :+OK %s", target, buf);

	hexchat_printf(ph, "Notice sent to %s", target);
	return HEXCHAT_EAT_ALL;
}

/**
 * Command handler for /msg+
 */
int handle_crypt_msg(char *word[], char *word_eol[], void *userdata)
{
	char* buf;
	char *target = word[2];
	char *message = word_eol[3];
	hexchat_context *query_ctx;

	if (target == 0 || *target == 0 || message == 0 || *message == 0)
	{
		hexchat_printf(ph, usage_msg);
		return HEXCHAT_EAT_ALL;
	}

	buf = fish_encrypt_for_nick(target, message);
	if (buf == NULL) {
		hexchat_printf(ph, "/msg+ error, no key found for %s", target);
		return HEXCHAT_EAT_ALL;
	}

	hexchat_commandf(ph, "PRIVMSG %s :+OK %s", target, buf);

	query_ctx = hexchat_find_context(ph, NULL, target);
	if (query_ctx)
	{	// open query/channel window found, display the event there
		hexchat_set_context(ph, query_ctx);

		hexchat_printf(ph, buf, hexchat_get_info(ph, "nick"), message);
	}
	else hexchat_printf(ph, "Message sent to %s", target);

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
    
    /* Add handlers */
    hexchat_hook_command(ph, "", HEXCHAT_PRI_NORM, handle_outgoing, NULL, NULL);
    hexchat_hook_server_attrs(ph, "NOTICE", HEXCHAT_PRI_NORM, handle_notice, NULL);
    hexchat_hook_server_attrs(ph, "PRIVMSG", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    /* hexchat_hook_server(ph, "RAW LINE", HEXCHAT_PRI_NORM, handle_debug, NULL); */
    hexchat_hook_server_attrs(ph, "TOPIC", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    hexchat_hook_server_attrs(ph, "332", HEXCHAT_PRI_NORM, handle_incoming, NULL);
    
    hexchat_printf(ph, "%s plugin loaded\n", plugin_name);

	dh = newDH1080();

    /* Return success */
    return 1;
}

int hexchat_plugin_deinit(void) {
    hexchat_printf(ph, "%s plugin unloaded\n", plugin_name);
    return 1;
}

