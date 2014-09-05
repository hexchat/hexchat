/* HexChat
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

#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "hexchat-plugin.h"

static hexchat_plugin *ph;
static char name[] = "ZNC Extras";
static char desc[] = "Integrate with optional ZNC plugins";
static char version[] = "1.0";


/* Playback: http://wiki.znc.in/Playback */

/* This should behave like this:
 * On connect (end of MOTD):
 *    if new server, play all
 *    if old server, play all after latest timestamp
 * On new query:
 *    play all
 * On close query:
 *    clear all
 * On new message:
 *    update latest timestamp
 */

static GHashTable *playback_servers = NULL;

#define PLAYBACK_CAP "znc.in/playback"

static int
get_current_context_type (void)
{
    int type = 0;
    hexchat_list *list;
    hexchat_context *cur_ctx;

    list = hexchat_list_get (ph, "channels");
    if (!list)
        return 0;

    cur_ctx = hexchat_get_context (ph);

    while (hexchat_list_next (ph, list))
    {
        if ((hexchat_context*)hexchat_list_str (ph, list, "context") == cur_ctx)
        {
            type = hexchat_list_int (ph, list, "type");
            break;
        }
    }

    hexchat_list_free (ph, list);
    return type;
}

static int
get_current_context_id (void)
{
    gint id;

    if (hexchat_get_prefs (ph, "id", NULL, &id) != 2)
        return -1;

    return id;
}

static int
capls_cb (char *word[], void *userdata)
{
    if (strstr (word[2], PLAYBACK_CAP) != NULL)
        hexchat_command (ph, "quote CAP REQ :" PLAYBACK_CAP);

    return HEXCHAT_EAT_NONE;
}

static int
capack_cb (char *word[], void *userdata)
{
    if (strstr (word[2], PLAYBACK_CAP) != NULL)
    {
        int id;

        /* FIXME: emit this after request but before ack.. */
        hexchat_emit_print (ph, "Capability Request", PLAYBACK_CAP, NULL);

        id = get_current_context_id ();
        if (!g_hash_table_contains (playback_servers, GINT_TO_POINTER(id)))
            g_hash_table_insert (playback_servers, GINT_TO_POINTER(id), g_strdup ("0"));
    }

    return HEXCHAT_EAT_NONE;
}

static int
connected_cb (char *word[], char *word_eol[], void *userdata)
{
    int id = get_current_context_id ();
    if (g_hash_table_contains (playback_servers, GINT_TO_POINTER(id)))
    {
        g_print("connected\n");
        char *timestamp = g_hash_table_lookup (playback_servers, GINT_TO_POINTER(id));
        hexchat_commandf (ph, "quote PRIVMSG *playback :play * %s", timestamp);
    }

    return HEXCHAT_EAT_NONE;
}

static int
opencontext_cb (char *word[], void *userdata)
{
#if 0
    int id = get_current_context_id ();
    if (g_hash_table_contains (playback_servers, GINT_TO_POINTER(id))
        && get_current_context_type () == 3)
    {
        const char *channel = hexchat_get_info (ph, "channel");
        char *time_str = g_hash_table_lookup (playback_servers,  GINT_TO_POINTER(id));
        hexchat_commandf (ph, "quote PRIVMSG *playback :play %s %s", channel, time_str);
    }
#endif

    return HEXCHAT_EAT_NONE;
}

static int
closecontext_cb (char *word[], void *userdata)
{
    int id = get_current_context_id ();
    if (g_hash_table_contains (playback_servers, GINT_TO_POINTER(id)))
    {
        int type = get_current_context_type ();
        if (type == 3) /* Dialog */
        {
            const char *channel = hexchat_get_info (ph, "channel");
            hexchat_commandf (ph, "quote PRIVMSG *playback :clear %s", channel);
        }
        else if (type == 1) /* Server */
        {
            g_hash_table_remove (playback_servers, GINT_TO_POINTER(id));
        }
    }

    return HEXCHAT_EAT_NONE;
}

static int
chanactivity_cb (char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata)
{
    int id;

    if (!attrs->server_time_utc)
        return HEXCHAT_EAT_NONE;

    id = get_current_context_id ();
    if (g_hash_table_contains (playback_servers, GINT_TO_POINTER(id)))
    {
        char *time_str;

        time_str = g_strdup_printf ("%ld", attrs->server_time_utc);
        g_hash_table_replace (playback_servers, GINT_TO_POINTER(id), time_str);
    }

    return HEXCHAT_EAT_NONE;
}

/* End Playback */


/* Buffextras: http://wiki.znc.in/Buffextras */

static inline char *
strip_brackets (char *str)
{
    str++;
    str[strlen(str) - 1] = '\0';
    return str;
}

static int
buffextras_cb (char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata)
{
    char *channel, *user_nick, *user_host = NULL, *p;

    if (g_ascii_strncasecmp (word[1] + 1, "*buffextras", 11) != 0)
        return HEXCHAT_EAT_NONE;

    channel = word[3];
    user_nick = word[4] + 1;
    p = strchr (user_nick, '!');
    if (p != NULL)
    {
        *p = '\0';
        user_host = p + 1;
    }

    /* This might be gross, but there are a lot of repeated calls.. */
    #define EMIT(x,...) hexchat_emit_print_attrs(ph,attrs,x,__VA_ARGS__,NULL)
    #define IS_EVENT(x) g_str_has_prefix(word_eol[5],x)

    if (IS_EVENT ("joined"))
    {
        EMIT ("Join", user_nick, channel, user_host);
    }
    else if (IS_EVENT ("quit with message"))
    {
        EMIT ("Quit", user_nick, strip_brackets(word_eol[7]), user_host);
    }
    else if (IS_EVENT ("parted with message"))
    {
        char *reason = strip_brackets(word_eol[7]);

        if (*reason)
            EMIT ("Part with Reason", user_nick, user_host, channel, reason);
        else
            EMIT ("Part", user_nick, user_host, channel);
    }
    else if (IS_EVENT ("is now known as"))
    {
        EMIT ("Change Nick", user_nick, word[9]);
    }
    else if (IS_EVENT ("set mode"))
    {
        char *prefix = (word[7][0] == '+' ? "+" : "-");

        EMIT ("Channel Mode Generic", user_nick, prefix, word[7] + 1, channel);
    }
    else if (IS_EVENT ("changed the topic to"))
    {
        EMIT ("Topic Change", user_nick, word_eol[9], channel);
    }
    else if (IS_EVENT ("kicked")) /* X Reason: [Y] */
    {
        EMIT ("Kick", user_nick, word[6], channel, strip_brackets(word_eol[8]));
    }
    else
    {
        return HEXCHAT_EAT_NONE; /* We don't know, let it print */
    }

    return HEXCHAT_EAT_ALL;
}

/* End Buffextras */


/* Privmsg: http://wiki.znc.in/Privmsg */

static gboolean
prefix_is_channel (char prefix)
{
    gboolean is_channel = TRUE; /* Defaults to TRUE as being wrong would be annoying */
    int id;
    hexchat_list *list;

    if (hexchat_get_prefs (ph, "id", NULL, &id) != 2)
        return TRUE;

    list = hexchat_list_get (ph, "channels");
    if (!list)
        return TRUE;


    while (hexchat_list_next (ph, list))
    {
        if (hexchat_list_int (ph, list, "id") == id)
        {
            const char *chantypes = hexchat_list_str (ph, list, "chantypes");
            if (strchr (chantypes, prefix) == NULL)
                is_channel = FALSE;
            break;
        }
    }

    hexchat_list_free (ph, list);
    return is_channel;
}

static int
privmsg_cb (char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata)
{
    char *sender, *recipient, *msg, *p;
    const char *network, *mynick;

    if (prefix_is_channel (word[3][0]))
        return HEXCHAT_EAT_NONE;

    mynick = hexchat_get_info (ph, "nick");
    network = hexchat_get_info (ph, "network");
    recipient = word[3];
    msg = word_eol[4] + 1;
    sender = word[1] + 1;

    p = strchr (sender, '!');
    if (p != NULL)
        *p = '\0';

    if (hexchat_nickcmp (ph, sender, mynick) == 0
        && hexchat_nickcmp (ph, recipient, mynick) != 0)
    {
        hexchat_context *new_ctx;
        char *event;

        hexchat_commandf (ph, "query -nofocus %s", recipient);
        new_ctx = hexchat_find_context (ph, network, recipient);
        hexchat_set_context (ph, new_ctx);

        if (g_ascii_strncasecmp (msg, "\001ACTION", 7) == 0)
        {
            msg += 7;
            msg[strlen (msg) - 1] = '\0';
            msg = g_strstrip (msg);
            event = "Your Action";
        }
        else
        {
            event = "Your Message";
        }

        hexchat_emit_print_attrs (ph, attrs, event, mynick, msg, NULL);
        return HEXCHAT_EAT_ALL;
    }

    return HEXCHAT_EAT_NONE;
}

/* End Privmsg */


int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc,
                     char **plugin_version, char *arg)
{
    ph = plugin_handle;
    *plugin_name = name;
    *plugin_desc = desc;
    *plugin_version = version;

    /* Privmsg */
    hexchat_hook_server_attrs (ph, "PRIVMSG", HEXCHAT_PRI_HIGH, privmsg_cb, NULL);

    /* Buffextras */
    hexchat_hook_server_attrs (ph, "PRIVMSG", HEXCHAT_PRI_HIGHEST, buffextras_cb, NULL);

    /* Playback */
    playback_servers = g_hash_table_new_full (NULL, NULL, NULL, g_free);

    hexchat_hook_print (ph, "Capability List", HEXCHAT_PRI_NORM, capls_cb, NULL);
    hexchat_hook_print (ph, "Capability Acknowledgement", HEXCHAT_PRI_NORM, capack_cb, NULL);
    hexchat_hook_print (ph, "Open Context", HEXCHAT_PRI_NORM, opencontext_cb, NULL);
    hexchat_hook_print (ph, "Close Context", HEXCHAT_PRI_NORM, closecontext_cb, NULL);
    hexchat_hook_server (ph, "376", HEXCHAT_PRI_NORM, connected_cb, NULL);

    /* FIXME: Events buffextras uses */
    hexchat_hook_server_attrs (ph, "PRIVMSG", HEXCHAT_PRI_LOW, chanactivity_cb, NULL);

    hexchat_printf (ph, "%s plugin loaded\n", name);
    return 1;
}

int
hexchat_plugin_deinit (void)
{
    g_hash_table_unref (playback_servers);
    hexchat_printf (ph, "%s plugin unloaded\n", name);
    return 1;
}
