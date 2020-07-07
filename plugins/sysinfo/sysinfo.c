/*
 * SysInfo - sysinfo plugin for HexChat
 * Copyright (c) 2012 Berke Viktor.
 *
 * xsys.c - main functions for X-Sys 2
 * by mikeshoup
 * Copyright (C) 2003, 2004, 2005 Michael Shoup
 * Copyright (C) 2005, 2006, 2007 Tony Vroon
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "hexchat-plugin.h"
#include "sysinfo-backend.h"
#include "sysinfo.h"

#define _(x) hexchat_gettext(ph,x)
#define DEFAULT_ANNOUNCE TRUE

static hexchat_plugin *ph;

static char name[] = "Sysinfo";
static char desc[] = "Display info about your hardware and OS";
static char version[] = "1.0";
static char sysinfo_help[] = "SysInfo Usage:\n  /SYSINFO [-e|-o] [CLIENT|OS|CPU|RAM|DISK|VGA|SOUND|ETHERNET|UPTIME], print various details about your system or print a summary without arguments\n  /SYSINFO SET <variable>\n";

typedef struct
{
	const char *name; /* Lower case name used for prefs */
	const char *title; /* Used for the end formatting */
	char *(*callback) (void);
	gboolean def; /* Hide by default? */
} hwinfo;

static char *
get_client (void)
{
	return g_strdup_printf ("HexChat %s", hexchat_get_info(ph, "version"));
}

static hwinfo hwinfos[] = {
	{"client", "Client", get_client},
	{"os", "OS", sysinfo_backend_get_os},
	{"cpu", "CPU", sysinfo_backend_get_cpu},
	{"memory", "Memory", sysinfo_backend_get_memory},
	{"storage", "Storage", sysinfo_backend_get_disk},
	{"vga", "VGA", sysinfo_backend_get_gpu},
	{"sound", "Sound", sysinfo_backend_get_sound, TRUE},
	{"ethernet", "Ethernet", sysinfo_backend_get_network, TRUE},
	{"uptime", "Uptime", sysinfo_backend_get_uptime},
	{NULL, NULL},
};

static gboolean sysinfo_get_bool_pref (const char *pref, gboolean def);

static gboolean
should_show_info (hwinfo info)
{
	char hide_pref[32];

	g_snprintf (hide_pref, sizeof(hide_pref), "hide_%s", info.name);
	return !sysinfo_get_bool_pref (hide_pref, info.def);
}

static void
print_summary (gboolean announce)
{
	char **strings = g_new0 (char*, G_N_ELEMENTS(hwinfos));
	int i, x;
	char *output;

	for (i = 0, x = 0; hwinfos[i].name != NULL; i++)
	{
		if (should_show_info (hwinfos[i]))
		{
			char *str = hwinfos[i].callback();
			if (str)
			{
				strings[x++] = g_strdup_printf ("\002%s\002: %s", hwinfos[i].title, str);
				g_free (str);
			}
		}
	}

	output = g_strjoinv (" \002\342\200\242\002 ", strings);
	hexchat_commandf (ph, "%s %s", announce ? "SAY" : "ECHO", output);

	g_strfreev (strings);
	g_free (output);
}

static void
print_info (char *info, gboolean announce)
{
	int i;

	for (i = 0; hwinfos[i].name != NULL; i++)
	{
		if (!g_ascii_strcasecmp (info, hwinfos[i].name))
		{
			char *str = hwinfos[i].callback();
			if (str)
			{
				hexchat_commandf (ph, "%s \002%s\002: %s", announce ? "SAY" : "ECHO",
									hwinfos[i].title, str);
				g_free (str);
			}
			else
				hexchat_print (ph, _("Sysinfo: Failed to get info. Either not supported or error."));
			return;
		}
	}

	hexchat_print (ph, _("Sysinfo: No info by that name\n"));
}

static gboolean
sysinfo_get_bool_pref (const char *pref, gboolean def)
{
	int value = hexchat_pluginpref_get_int (ph, pref);

	if (value != -1)
		return value;

	return def;
}

static void
sysinfo_set_pref_real (const char *pref, char *value, gboolean def)
{
	if (value && value[0])
	{
		guint64 i = g_ascii_strtoull (value, NULL, 0);
		hexchat_pluginpref_set_int (ph, pref, i != 0);
		hexchat_printf (ph, _("Sysinfo: %s is set to: %d\n"), pref, i != 0);
	}
	else
	{
		hexchat_printf (ph, _("Sysinfo: %s is set to: %d\n"), pref,
						sysinfo_get_bool_pref(pref, def));
	}
}

static void
sysinfo_set_pref (char *key, char *value)
{
	if (!key || !key[0])
	{
		hexchat_print (ph, _("Sysinfo: Valid settings are: announce and hide_* for each piece of information. e.g. hide_os. Without a value it will show current (or default) setting.\n"));
		return;
	}

	if (!strcmp (key, "announce"))
	{
		sysinfo_set_pref_real (key, value, DEFAULT_ANNOUNCE);
		return;
	}
	else if (g_str_has_prefix (key, "hide_"))
	{
		int i;
		for (i = 0; hwinfos[i].name != NULL; i++)
		{
			if (!strcmp (key + 5, hwinfos[i].name))
			{
				sysinfo_set_pref_real (key, value, hwinfos[i].def);
				return;
			}
		}
	}

	hexchat_print (ph, _("Sysinfo: Invalid variable name\n"));
}

static int
sysinfo_cb (char *word[], char *word_eol[], void *userdata)
{
	gboolean announce = sysinfo_get_bool_pref("announce", DEFAULT_ANNOUNCE);
	int offset = 0, channel_type;
	char *cmd;

	/* Allow overriding global announce setting */
	if (!strcmp ("-e", word[2]))
	{
		announce = FALSE;
		offset++;
	}
	else if (!strcmp ("-o", word[2]))
	{
		announce = TRUE;
		offset++;
	}

	/* Cannot send to server tab */
	channel_type = hexchat_list_int (ph, NULL, "type");
	if (channel_type != 2 /* SESS_CHANNEL */ && channel_type != 3 /* SESS_DIALOG */)
		announce = FALSE;

	cmd = word[2+offset];
	if (!g_ascii_strcasecmp ("SET", cmd))
		sysinfo_set_pref (word[3+offset], word_eol[4+offset]);
	else if (!cmd || !cmd[0])
		print_summary (announce);
	else
		print_info (cmd, announce);

	return HEXCHAT_EAT_ALL;
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;
	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	hexchat_hook_command (ph, "SYSINFO", HEXCHAT_PRI_NORM, sysinfo_cb, sysinfo_help, NULL);

	hexchat_command (ph, "MENU ADD \"Window/Send System Info\" \"SYSINFO\"");
	hexchat_printf (ph, _("%s plugin loaded\n"), name);
	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_command (ph, "MENU DEL \"Window/Display System Info\"");
	hexchat_printf (ph, _("%s plugin unloaded\n"), name);
	return 1;
}
