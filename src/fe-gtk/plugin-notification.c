/* HexChat
 * Copyright (C) 2015 Patrick Griffis.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <glib.h>

#include "../common/hexchat-plugin.h"
#include "../common/inbound.h" /* For alert_match_word() */
#include "notifications/notification-backend.h"

static hexchat_plugin *ph;

const int CHANNEL_FLAG_BALLOON       = 1 << 21;
const int CHANNEL_FLAG_BALLOON_UNSET = 1 << 22;

static gboolean
should_alert (void)
{
	int omit_away, omit_focused, omit_tray;

	if (hexchat_get_prefs (ph, "gui_focus_omitalerts", NULL, &omit_focused) == 3 && omit_focused)
	{
		const char *status = hexchat_get_info (ph, "win_status");

		if (status && !g_strcmp0 (status, "active"))
			return FALSE;
	}

	if (hexchat_get_prefs (ph, "away_omit_alerts", NULL, &omit_away) == 3 && omit_away)
	{
		if (hexchat_get_info (ph, "away"))
			return FALSE;
	}

	if (hexchat_get_prefs (ph, "gui_tray_quiet", NULL, &omit_tray) == 3 && omit_tray)
	{
		int tray_enabled;

		if (hexchat_get_prefs (ph, "gui_tray", NULL, &tray_enabled) == 3 && tray_enabled)
		{
			const char *status = hexchat_get_info (ph, "win_status");

			if (status && g_strcmp0 (status, "hidden") != 0)
				return FALSE;
		}
	}

	return TRUE;
}

static gboolean
is_ignored (char *nick)
{
	const char *no_hilight;

	if (hexchat_get_prefs (ph, "irc_no_hilight", &no_hilight, NULL) == 1 && no_hilight)
	{
		return alert_match_word (nick, (char*)no_hilight);
	}
	return FALSE;
}

static void
show_notification (const char *title, const char *text)
{
	char *stripped_title, *stripped_text;

	/* Strip all colors */
	stripped_title = hexchat_strip (ph, title, -1, 7);
	stripped_text = hexchat_strip (ph, text, -1, 7);
	
	notification_backend_show (stripped_title, stripped_text);

	hexchat_free (ph, stripped_title);
	hexchat_free (ph, stripped_text);
}

static void
show_notificationf (const char *text, const char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	show_notification (buf, text);
	g_free (buf);
}

static int
incoming_hilight_cb (char *word[], gpointer userdata)
{
	int hilight;

	if (hexchat_get_prefs (ph, "input_balloon_hilight", NULL, &hilight) == 3 && hilight && should_alert())
	{
		show_notificationf (word[2], _("Highlighted message from: %s (%s)"), word[1], hexchat_get_info (ph, "channel"));
	}
	return HEXCHAT_EAT_NONE;
}

static int
incoming_message_cb (char *word[], gpointer userdata)
{
	int message;
	int flags;
	int alert = 0;

	flags = hexchat_list_int(ph, NULL, "flags");

	/* Let sure that can alert */
	if (should_alert()) {
		/* Follow the channel rules if set */
		if (!(flags & CHANNEL_FLAG_BALLOON_UNSET)) {
			alert = (flags & CHANNEL_FLAG_BALLOON);
		} else {
			/* Else follow global environment */
			alert = (hexchat_get_prefs(ph, "input_balloon_chans", NULL, &message) == 3 && message);
		}
	}

	if (alert) {
		show_notificationf(word[2], _("Channel message from: %s (%s)"), word[1], hexchat_get_info(ph, "channel"));
	}
	return HEXCHAT_EAT_NONE;
}

static int
incoming_priv_cb (char *word[], gpointer userdata)
{
	int priv;
	int flags;
	int alert = 0;

	flags = hexchat_list_int(ph, NULL, "flags");

	/* Let sure that can alert */
	if (should_alert()) {
		/* Follow the private rules if set */
		if (!(flags & CHANNEL_FLAG_BALLOON_UNSET)) {
			alert = (flags & CHANNEL_FLAG_BALLOON);
		} else {
			/* Else follow global environment */
			alert = (hexchat_get_prefs(ph, "input_balloon_priv", NULL, &priv) == 3 && priv);
		}
	}

	if (alert)
	{
		const char *network = hexchat_get_info (ph, "network");
		if (!network)
			network = hexchat_get_info (ph, "server");

		if (userdata != NULL) /* Special event */
		{
			if (GPOINTER_TO_INT (userdata) == 3)
			{
				if (!is_ignored (word[2]))
					show_notificationf (word[1], _("File offer from: %s (%s)"), word[2], network);
			}
			else if (GPOINTER_TO_INT (userdata) == 2)
			{
				if (!is_ignored (word[2]))
					show_notificationf (word[1], _("Invited to channel by: %s (%s)"), word[2], network);
			}
			else
			{
				if (!is_ignored (word[1]))
					show_notificationf (word[2], _("Notice from: %s (%s)"), word[1], network);
			}
		}
		else
			show_notificationf (word[2], _("Private message from: %s (%s)"), word[1], network);
	}
	return HEXCHAT_EAT_NONE;
}

static int
tray_cmd_cb (char *word[], char *word_eol[], gpointer userdata)
{
	if (word[2] && !g_ascii_strcasecmp (word[2], "-b") && word[3] && word[4])
	{
		if (should_alert ())
			show_notification (word[3], word_eol[4]);
		return HEXCHAT_EAT_ALL;
	}

	return HEXCHAT_EAT_NONE;
}

int
notification_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	const char* error = NULL;

	ph = plugin_handle;
	*plugin_name = "";
	*plugin_desc = "";
	*plugin_version = "";

	if (!notification_backend_init (&error))
	{
		if (error)
			g_debug("Failed loading notification plugin: %s\n", error);
		return 0;
	}

	hexchat_hook_print (ph, "Channel Msg Hilight", HEXCHAT_PRI_LOWEST, incoming_hilight_cb, NULL);
	hexchat_hook_print (ph, "Channel Action Hilight", HEXCHAT_PRI_LOWEST, incoming_hilight_cb, NULL);

	hexchat_hook_print (ph, "Channel Message", HEXCHAT_PRI_LOWEST, incoming_message_cb, NULL);
	hexchat_hook_print (ph, "Channel Action", HEXCHAT_PRI_LOWEST, incoming_message_cb, NULL);
	hexchat_hook_print (ph, "Channel Notice", HEXCHAT_PRI_LOWEST, incoming_message_cb, NULL);

	hexchat_hook_print (ph, "Private Message", HEXCHAT_PRI_LOWEST, incoming_priv_cb, NULL);
	hexchat_hook_print (ph, "Private Message to Dialog", HEXCHAT_PRI_LOWEST, incoming_priv_cb, NULL);
	hexchat_hook_print (ph, "Private Action", HEXCHAT_PRI_LOWEST, incoming_priv_cb, NULL);
	hexchat_hook_print (ph, "Private Action to Dialog", HEXCHAT_PRI_LOWEST, incoming_priv_cb, NULL);

	/* Special events treated as priv */
	hexchat_hook_print (ph, "Notice", HEXCHAT_PRI_LOWEST, incoming_priv_cb, GINT_TO_POINTER (1));
	hexchat_hook_print (ph, "Invited", HEXCHAT_PRI_LOWEST, incoming_priv_cb, GINT_TO_POINTER (2));
	hexchat_hook_print (ph, "DCC Offer", HEXCHAT_PRI_LOWEST, incoming_priv_cb, GINT_TO_POINTER (3));

	hexchat_hook_command (ph, "TRAY", HEXCHAT_PRI_HIGH, tray_cmd_cb, NULL, NULL);
	
	return 1;
}


int
notification_plugin_deinit (void *unused_param)
{
	notification_backend_deinit ();
	return 1;
}
