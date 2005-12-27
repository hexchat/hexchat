/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "fe.h"
#include "text.h"
#include "xchatc.h"

#ifdef WIN32
#define XCHAT_DIR "X-Chat 2"
#else
#define XCHAT_DIR ".xchat2"
#endif
#define DEF_FONT "Monospace 9"

void
list_addentry (GSList ** list, char *cmd, char *name)
{
	struct popup *pop;
	int cmd_len = 1, name_len;

	if (cmd)
		cmd_len = strlen (cmd) + 1;
	name_len = strlen (name) + 1;

	pop = malloc (sizeof (struct popup) + cmd_len + name_len);
	pop->name = (char *) pop + sizeof (struct popup);
	pop->cmd = pop->name + name_len;

	memcpy (pop->name, name, name_len);
	if (cmd)
		memcpy (pop->cmd, cmd, cmd_len);
	else
		pop->cmd[0] = 0;

	*list = g_slist_append (*list, pop);
}

/* read it in from a buffer to our linked list */

static void
list_load_from_data (GSList ** list, char *ibuf, int size)
{
	char cmd[384];
	char name[128];
	char *buf;
	int pnt = 0;

	cmd[0] = 0;
	name[0] = 0;

	while (buf_get_line (ibuf, &buf, &pnt, size))
	{
		if (*buf != '#')
		{
			if (!strncasecmp (buf, "NAME ", 5))
			{
				safe_strcpy (name, buf + 5, sizeof (name));
			}
			else if (!strncasecmp (buf, "CMD ", 4))
			{
				safe_strcpy (cmd, buf + 4, sizeof (cmd));
				if (*name)
				{
					list_addentry (list, cmd, name);
					cmd[0] = 0;
					name[0] = 0;
				}
			}
		}
	}
}

void
list_loadconf (char *file, GSList ** list, char *defaultconf)
{
	char filebuf[256];
	char *ibuf;
	int fh;
	struct stat st;

	snprintf (filebuf, sizeof (filebuf), "%s/%s", get_xdir_fs (), file);
	fh = open (filebuf, O_RDONLY | OFLAGS);
	if (fh == -1)
	{
		if (defaultconf)
			list_load_from_data (list, defaultconf, strlen (defaultconf));
		return;
	}
	if (fstat (fh, &st) != 0)
	{
		perror ("fstat");
		abort ();
	}

	ibuf = malloc (st.st_size);
	read (fh, ibuf, st.st_size);
	close (fh);

	list_load_from_data (list, ibuf, st.st_size);

	free (ibuf);
}

void
list_free (GSList ** list)
{
	void *data;
	while (*list)
	{
		data = (void *) (*list)->data;
		free (data);
		*list = g_slist_remove (*list, data);
	}
}

int
list_delentry (GSList ** list, char *name)
{
	struct popup *pop;
	GSList *alist = *list;

	while (alist)
	{
		pop = (struct popup *) alist->data;
		if (!strcasecmp (name, pop->name))
		{
			*list = g_slist_remove (*list, pop);
			free (pop);
			return 1;
		}
		alist = alist->next;
	}
	return 0;
}

char *
cfg_get_str (char *cfg, char *var, char *dest, int dest_len)
{
	while (1)
	{
		if (!strncasecmp (var, cfg, strlen (var)))
		{
			char *value, t;
			cfg += strlen (var);
			while (*cfg == ' ')
				cfg++;
			if (*cfg == '=')
				cfg++;
			while (*cfg == ' ')
				cfg++;
			/*while (*cfg == ' ' || *cfg == '=')
			   cfg++; */
			value = cfg;
			while (*cfg != 0 && *cfg != '\n')
				cfg++;
			t = *cfg;
			*cfg = 0;
			safe_strcpy (dest, value, dest_len);
			*cfg = t;
			return cfg;
		}
		while (*cfg != 0 && *cfg != '\n')
			cfg++;
		if (*cfg == 0)
			return 0;
		cfg++;
		if (*cfg == 0)
			return 0;
	}
}

static int
cfg_put_str (int fh, char *var, char *value)
{
	char buf[512];
	int len;

	snprintf (buf, sizeof buf, "%s = %s\n", var, value);
	len = strlen (buf);
	return (write (fh, buf, len) == len);
}

int
cfg_put_color (int fh, int r, int g, int b, char *var)
{
	char buf[400];
	int len;

	snprintf (buf, sizeof buf, "%s = %04x %04x %04x\n", var, r, g, b);
	len = strlen (buf);
	return (write (fh, buf, len) == len);
}

int
cfg_put_int (int fh, int value, char *var)
{
	char buf[400];
	int len;

	if (value == -1)
		value = 1;

	snprintf (buf, sizeof buf, "%s = %d\n", var, value);
	len = strlen (buf);
	return (write (fh, buf, len) == len);
}

int
cfg_get_color (char *cfg, char *var, int *r, int *g, int *b)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str, sizeof (str)))
		return 0;

	sscanf (str, "%04x %04x %04x", r, g, b);
	return 1;
}

int
cfg_get_int_with_result (char *cfg, char *var, int *result)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str, sizeof (str)))
	{
		*result = 0;
		return 0;
	}

	*result = 1;
	return atoi (str);
}

int
cfg_get_int (char *cfg, char *var)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str, sizeof (str)))
		return 0;

	return atoi (str);
}

char *xdir_fs = NULL;	/* file system encoding */
char *xdir_utf = NULL;	/* utf-8 encoding */

#ifdef WIN32

#include <windows.h>

static gboolean
get_reg_str (const char *sub, const char *name, char *out, DWORD len)
{
	HKEY hKey;
	DWORD t;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, sub, 0, KEY_READ, &hKey) ==
			ERROR_SUCCESS)
	{
		if (RegQueryValueEx (hKey, name, NULL, &t, out, &len) != ERROR_SUCCESS ||
			 t != REG_SZ)
		{
			RegCloseKey (hKey);
			return FALSE;
		}
		out[len-1] = 0;
		RegCloseKey (hKey);
		return TRUE;
	}

	return FALSE;
}

char *
get_xdir_fs (void)
{
	if (!xdir_fs)
	{
		char out[256];

		if (!get_reg_str ("Software\\Microsoft\\Windows\\CurrentVersion\\"
				"Explorer\\Shell Folders", "AppData", out, sizeof (out)))
			return "./config";
		xdir_fs = g_strdup_printf ("%s\\" XCHAT_DIR, out);
	}
	return xdir_fs;
}

#else

char *
get_xdir_fs (void)
{
	if (!xdir_fs)
		xdir_fs = g_strdup_printf ("%s/" XCHAT_DIR, g_get_home_dir ());

	return xdir_fs;
}

#endif	/* !WIN32 */

char *
get_xdir_utf8 (void)
{
	if (!xdir_utf)	/* never free this, keep it for program life time */
		xdir_utf = g_filename_to_utf8 (get_xdir_fs (), -1, 0, 0, 0);

	return xdir_utf;
}

static void
check_prefs_dir (void)
{
	char *dir = get_xdir_fs ();
	if (access (dir, F_OK) != 0)
	{
#ifdef WIN32
		if (mkdir (dir) != 0)
#else
		if (mkdir (dir, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
#endif
			fe_message (_("Cannot create ~/.xchat2"), FE_MSG_ERROR);
	}
}

static char *
default_file (void)
{
	static char *dfile = 0;

	if (!dfile)
	{
		dfile = malloc (strlen (get_xdir_fs ()) + 12);
		sprintf (dfile, "%s/xchat.conf", get_xdir_fs ());
	}
	return dfile;
}

/* Keep these sorted!! */

const struct prefs vars[] = {
	{"auto_save", P_OFFINT (autosave), TYPE_BOOL},
	{"auto_save_url", P_OFFINT (autosave_url), TYPE_BOOL},

	{"away_auto_unmark", P_OFFINT (auto_unmark_away), TYPE_BOOL},
	{"away_reason", P_OFFSET (awayreason), TYPE_STR},
	{"away_show_message", P_OFFINT (show_away_message), TYPE_BOOL},
	{"away_show_once", P_OFFINT (show_away_once), TYPE_BOOL},
	{"away_size_max", P_OFFINT (away_size_max), TYPE_INT},
	{"away_timeout", P_OFFINT (away_timeout), TYPE_INT},
	{"away_track", P_OFFINT (away_track), TYPE_BOOL},

	{"completion_amount", P_OFFINT (completion_amount), TYPE_INT},
	{"completion_auto", P_OFFINT (nickcompletion), TYPE_BOOL},
	{"completion_sort", P_OFFINT (completion_sort), TYPE_INT},
	{"completion_suffix", P_OFFSET (nick_suffix), TYPE_STR},

	{"dcc_auto_chat", P_OFFINT (autodccchat), TYPE_INT},
	{"dcc_auto_resume", P_OFFINT (autoresume), TYPE_BOOL},
	{"dcc_auto_send", P_OFFINT (autodccsend), TYPE_INT},
	{"dcc_blocksize", P_OFFINT (dcc_blocksize), TYPE_INT},
	{"dcc_completed_dir", P_OFFSET (dcc_completed_dir), TYPE_STR},
	{"dcc_dir", P_OFFSET (dccdir), TYPE_STR},
	{"dcc_fast_send", P_OFFINT (fastdccsend), TYPE_BOOL},
	{"dcc_global_max_get_cps", P_OFFINT (dcc_global_max_get_cps), TYPE_INT},
	{"dcc_global_max_send_cps", P_OFFINT (dcc_global_max_send_cps), TYPE_INT},
	{"dcc_ip", P_OFFSET (dcc_ip_str), TYPE_STR},
	{"dcc_ip_from_server", P_OFFINT (ip_from_server), TYPE_BOOL},
	{"dcc_max_get_cps", P_OFFINT (dcc_max_get_cps), TYPE_INT},
	{"dcc_max_send_cps", P_OFFINT (dcc_max_send_cps), TYPE_INT},
	{"dcc_permissions", P_OFFINT (dccpermissions), TYPE_INT},
	{"dcc_port_first", P_OFFINT (first_dcc_send_port), TYPE_INT},
	{"dcc_port_last", P_OFFINT (last_dcc_send_port), TYPE_INT},
	{"dcc_remove", P_OFFINT (dcc_remove), TYPE_BOOL},
	{"dcc_save_nick", P_OFFINT (dccwithnick), TYPE_BOOL},
	{"dcc_send_fillspaces", P_OFFINT (dcc_send_fillspaces), TYPE_BOOL},
	{"dcc_stall_timeout", P_OFFINT (dccstalltimeout), TYPE_INT},
	{"dcc_timeout", P_OFFINT (dcctimeout), TYPE_INT},

	{"dnsprogram", P_OFFSET (dnsprogram), TYPE_STR},

	{"flood_ctcp_num", P_OFFINT (ctcp_number_limit), TYPE_INT},
	{"flood_ctcp_time", P_OFFINT (ctcp_time_limit), TYPE_INT},
	{"flood_msg_num", P_OFFINT (msg_number_limit), TYPE_INT},
	{"flood_msg_time", P_OFFINT (msg_time_limit), TYPE_INT},

	{"gui_auto_open_chat", P_OFFINT (autoopendccchatwindow), TYPE_BOOL},
	{"gui_auto_open_dialog", P_OFFINT (autodialog), TYPE_BOOL},
	{"gui_auto_open_recv", P_OFFINT (autoopendccrecvwindow), TYPE_BOOL},
	{"gui_auto_open_send", P_OFFINT (autoopendccsendwindow), TYPE_BOOL},
	{"gui_dialog_height", P_OFFINT (dialog_height), TYPE_INT},
	{"gui_dialog_left", P_OFFINT (dialog_left), TYPE_INT},
	{"gui_dialog_top", P_OFFINT (dialog_top), TYPE_INT},
	{"gui_dialog_width", P_OFFINT (dialog_width), TYPE_INT},
	{"gui_hide_menu", P_OFFINT (hidemenu), TYPE_BOOL},
	{"gui_input_style", P_OFFINT (style_inputbox), TYPE_BOOL},
	{"gui_join_dialog", P_OFFINT (gui_join_dialog), TYPE_BOOL},
	{"gui_lagometer", P_OFFINT (lagometer), TYPE_INT},
	{"gui_mode_buttons", P_OFFINT (chanmodebuttons), TYPE_BOOL},
	{"gui_slist_select", P_OFFINT (slist_select), TYPE_INT},
	{"gui_slist_skip", P_OFFINT (slist_skip), TYPE_BOOL},
	{"gui_throttlemeter", P_OFFINT (throttlemeter), TYPE_INT},
	{"gui_topicbar", P_OFFINT (topicbar), TYPE_BOOL},
	{"gui_ulist_buttons", P_OFFINT (userlistbuttons), TYPE_BOOL},
	{"gui_ulist_doubleclick", P_OFFSET (doubleclickuser), TYPE_STR},
	{"gui_ulist_hide", P_OFFINT (hideuserlist), TYPE_BOOL},
	{"gui_ulist_resizable", P_OFFINT (paned_userlist), TYPE_BOOL},
	{"gui_ulist_show_hosts", P_OFFINT(showhostname_in_userlist), TYPE_BOOL},
	{"gui_ulist_size", P_OFFINT (paned_pos), TYPE_INT},
	{"gui_ulist_sort", P_OFFINT (userlist_sort), TYPE_INT},
	{"gui_ulist_style", P_OFFINT (style_namelistgad), TYPE_BOOL},
	{"gui_usermenu", P_OFFINT (gui_usermenu), TYPE_BOOL},
	{"gui_win_height", P_OFFINT (mainwindow_height), TYPE_INT},
	{"gui_win_left", P_OFFINT (mainwindow_left), TYPE_INT},
	{"gui_win_save", P_OFFINT (mainwindow_save), TYPE_BOOL},
	{"gui_win_state", P_OFFINT (gui_win_state), TYPE_INT},
	{"gui_win_top", P_OFFINT (mainwindow_top), TYPE_INT},
	{"gui_win_width", P_OFFINT (mainwindow_width), TYPE_INT},

#ifdef WIN32
	{"identd", P_OFFINT (identd), TYPE_BOOL},
#endif
	{"input_beep_chans", P_OFFINT (beepchans), TYPE_BOOL},
	{"input_beep_hilight", P_OFFINT (beephilight), TYPE_BOOL},
	{"input_beep_msg", P_OFFINT (beepmsg), TYPE_BOOL},
	{"input_command_char", P_OFFSET (cmdchar), TYPE_STR},
	{"input_filter_beep", P_OFFINT (filterbeep), TYPE_BOOL},
	{"input_flash_hilight", P_OFFINT (flash_hilight), TYPE_BOOL},
	{"input_fudge_snotice", P_OFFINT (fudgeservernotice), TYPE_BOOL},
	{"input_perc_ascii", P_OFFINT (perc_ascii), TYPE_BOOL},
	{"input_perc_color", P_OFFINT (perc_color), TYPE_BOOL},

	{"irc_auto_rejoin", P_OFFINT (autorejoin), TYPE_BOOL},
	{"irc_ban_type", P_OFFINT (bantype), TYPE_INT},
	{"irc_conf_mode", P_OFFINT (confmode), TYPE_BOOL},
	{"irc_extra_hilight", P_OFFSET (irc_extra_hilight), TYPE_STR},
	{"irc_hide_version", P_OFFINT (hidever), TYPE_BOOL},
	{"irc_id_ntext", P_OFFSET (irc_id_ntext), TYPE_STR},
	{"irc_id_ytext", P_OFFSET (irc_id_ytext), TYPE_STR},
	{"irc_invisible", P_OFFINT (invisible), TYPE_BOOL},
	{"irc_join_delay", P_OFFINT (irc_join_delay), TYPE_INT},
	{"irc_logging", P_OFFINT (logging), TYPE_BOOL},
	{"irc_logmask", P_OFFSET (logmask), TYPE_STR},
	{"irc_nick1", P_OFFSET (nick1), TYPE_STR},
	{"irc_nick2", P_OFFSET (nick2), TYPE_STR},
	{"irc_nick3", P_OFFSET (nick3), TYPE_STR},
	{"irc_nick_hilight", P_OFFSET (irc_nick_hilight), TYPE_STR},
	{"irc_no_hilight", P_OFFSET (irc_no_hilight), TYPE_STR},
	{"irc_part_reason", P_OFFSET (partreason), TYPE_STR},
	{"irc_quit_reason", P_OFFSET (quitreason), TYPE_STR},
	{"irc_raw_modes", P_OFFINT (raw_modes), TYPE_BOOL},
	{"irc_real_name", P_OFFSET (realname), TYPE_STR},
	{"irc_servernotice", P_OFFINT (servernotice), TYPE_BOOL},
	{"irc_skip_motd", P_OFFINT (skipmotd), TYPE_BOOL},
	{"irc_user_name", P_OFFSET (username), TYPE_STR},
	{"irc_wallops", P_OFFINT (wallops), TYPE_BOOL},
	{"irc_who_join", P_OFFINT (userhost), TYPE_BOOL},
	{"irc_whois_front", P_OFFINT (irc_whois_front), TYPE_BOOL},

	{"net_auto_reconnect", P_OFFINT (autoreconnect), TYPE_BOOL},
	{"net_auto_reconnectonfail", P_OFFINT (autoreconnectonfail), TYPE_BOOL},
	{"net_bind_host", P_OFFSET (hostname), TYPE_STR},
	{"net_ping_timeout", P_OFFINT (pingtimeout), TYPE_INT},
	{"net_proxy_auth", P_OFFINT (proxy_auth), TYPE_BOOL},
	{"net_proxy_host", P_OFFSET (proxy_host), TYPE_STR},
	{"net_proxy_pass", P_OFFSET (proxy_pass), TYPE_STR},
	{"net_proxy_port", P_OFFINT (proxy_port), TYPE_INT},
	{"net_proxy_type", P_OFFINT (proxy_type), TYPE_INT},
	{"net_proxy_user", P_OFFSET (proxy_user), TYPE_STR},

	{"net_reconnect_delay", P_OFFINT (recon_delay), TYPE_INT},
	{"net_throttle", P_OFFINT (throttle), TYPE_BOOL},

	{"notify_timeout", P_OFFINT (notify_timeout), TYPE_INT},
	{"notify_whois_online", P_OFFINT (whois_on_notifyonline), TYPE_BOOL},

	{"perl_warnings", P_OFFINT (perlwarnings), TYPE_BOOL},

	{"sound_command", P_OFFSET (soundcmd), TYPE_STR},
	{"sound_dir", P_OFFSET (sounddir), TYPE_STR},
	{"stamp_log", P_OFFINT (timestamp_logs), TYPE_BOOL},
	{"stamp_log_format", P_OFFSET (timestamp_log_format), TYPE_STR},
	{"stamp_text", P_OFFINT (timestamp), TYPE_BOOL},
	{"stamp_text_format", P_OFFSET (stamp_format), TYPE_STR},

	{"tab_chans", P_OFFINT (tabchannels), TYPE_BOOL},
	{"tab_dialogs", P_OFFINT (privmsgtab), TYPE_BOOL},
	{"tab_dnd",  P_OFFINT (tab_dnd), TYPE_BOOL},
	{"tab_icons", P_OFFINT (tab_icons), TYPE_BOOL},
	{"tab_layout", P_OFFINT (tab_layout), TYPE_INT},
	{"tab_new_to_front", P_OFFINT (newtabstofront), TYPE_INT},
	{"tab_notices", P_OFFINT (notices_tabs), TYPE_BOOL},
	{"tab_position", P_OFFINT (tabs_position), TYPE_INT},
	{"tab_server", P_OFFINT (use_server_tab), TYPE_BOOL},
	{"tab_small", P_OFFINT (tab_small), TYPE_BOOL},
	{"tab_sort", P_OFFINT (tab_sort), TYPE_BOOL},
	{"tab_trunc", P_OFFINT (truncchans), TYPE_INT},
	{"tab_utils", P_OFFINT (windows_as_tabs), TYPE_BOOL},

	{"text_background", P_OFFSET (background), TYPE_STR},
	{"text_color_nicks", P_OFFINT (colorednicks), TYPE_BOOL},
	{"text_font", P_OFFSET (font_normal), TYPE_STR},
	{"text_indent", P_OFFINT (indent_nicks), TYPE_BOOL},
	{"text_max_indent", P_OFFINT (max_auto_indent), TYPE_INT},
	{"text_max_lines", P_OFFINT (max_lines), TYPE_INT},
	{"text_show_marker", P_OFFINT (show_marker), TYPE_BOOL},
	{"text_show_sep", P_OFFINT (show_separator), TYPE_BOOL},
	{"text_stripcolor", P_OFFINT (stripcolor), TYPE_BOOL},
	{"text_thin_sep", P_OFFINT (thin_separator), TYPE_BOOL},
	{"text_tint_blue", P_OFFINT (tint_blue), TYPE_INT},
	{"text_tint_green", P_OFFINT (tint_green), TYPE_INT},
	{"text_tint_red", P_OFFINT (tint_red), TYPE_INT},
	{"text_transparent", P_OFFINT (transparent), TYPE_BOOL},
	{"text_wordwrap", P_OFFINT (wordwrap), TYPE_BOOL},

	{0, 0, 0},
};

static char *
convert_with_fallback (const char *str, const char *fallback)
{
	char *utf;

	utf = g_locale_to_utf8 (str, -1, 0, 0, 0);
	if (!utf)
	{
		/* this can happen if CHARSET envvar is set wrong */
		/* maybe it's already utf8 (breakage!) */
		if (!g_utf8_validate (str, -1, NULL))
			utf = g_strdup (fallback);
		else
			utf = g_strdup (str);
	}

	return utf;
}

void
load_config (void)
{
	struct stat st;
	char *cfg, *sp;
	const char *username, *realname;
	int res, val, i, fh;

	check_prefs_dir ();
	username = g_get_user_name ();
	if (!username)
		username = "root";

	realname = g_get_real_name ();
	if ((realname && realname[0] == 0) || !realname)
		realname = username;

	username = convert_with_fallback (username, "username");
	realname = convert_with_fallback (realname, "realname");

	memset (&prefs, 0, sizeof (struct xchatprefs));

	/* put in default values, anything left out is automatically zero */
	prefs.local_ip = 0xffffffff;
	prefs.irc_join_delay = 3;
	prefs.show_marker = 1;
	prefs.newtabstofront = 2;
	prefs.completion_amount = 5;
	prefs.away_timeout = 60;
	prefs.away_size_max = 300;
	prefs.away_track = 1;
	prefs.timestamp_logs = 1;
	prefs.truncchans = 20;
	prefs.autoresume = 1;
	prefs.show_away_once = 1;
	prefs.indent_nicks = 1;
	prefs.thin_separator = 1;
	/*prefs.tabs_position = 1;*/ /* 0 = bottom */
	prefs.fastdccsend = 1;
	prefs.wordwrap = 1;
	prefs.autosave = 1;
	prefs.autodialog = 1;
	prefs.autoreconnect = 1;
	prefs.recon_delay = 10;
	prefs.tabchannels = 1;
	prefs.tab_sort = 1;
	prefs.paned_userlist = 1;
	prefs.newtabstofront = 2;
	prefs.use_server_tab = 1;
	prefs.privmsgtab = 1;
	/*prefs.style_inputbox = 1;*/
	prefs.dccpermissions = 0600;
	prefs.max_lines = 300;
	prefs.mainwindow_width = 640;
	prefs.mainwindow_height = 400;
	prefs.dialog_width = 500;
	prefs.dialog_height = 256;
	prefs.gui_join_dialog = 1;
	prefs.dcctimeout = 180;
	prefs.dccstalltimeout = 60;
	prefs.notify_timeout = 15;
	prefs.tint_red =
		prefs.tint_green =
		prefs.tint_blue = 195;
	prefs.auto_indent = 1;
	prefs.max_auto_indent = 256;
	prefs.show_separator = 1;
	prefs.dcc_blocksize = 1024;
	prefs.throttle = 1;
	 /*FIXME*/ prefs.msg_time_limit = 30;
	prefs.msg_number_limit = 5;
	prefs.ctcp_time_limit = 30;
	prefs.ctcp_number_limit = 5;
	prefs.topicbar = 1;
	prefs.lagometer = 1;
	prefs.throttlemeter = 1;
	prefs.autoopendccrecvwindow = 1;
	prefs.autoopendccsendwindow = 1;
	prefs.autoopendccchatwindow = 1;
	prefs.userhost = 1;
	prefs.dcc_send_fillspaces = 1;
	prefs.mainwindow_save = 1;
	prefs.bantype = 2;
	prefs.flash_hilight = 1;
#ifdef WIN32
	prefs.identd = 1;
#endif
	strcpy (prefs.stamp_format, "[%H:%M] ");
	strcpy (prefs.timestamp_log_format, "%b %d %H:%M:%S ");
	strcpy (prefs.logmask, "%n-%c.log");
	strcpy (prefs.nick_suffix, ",");
	strcpy (prefs.cmdchar, "/");
	strcpy (prefs.nick1, username);
	strcpy (prefs.nick2, username);
	strcat (prefs.nick2, "_");
	strcpy (prefs.nick3, username);
	strcat (prefs.nick3, "__");
	strcpy (prefs.realname, realname);
	strcpy (prefs.username, username);
#ifdef WIN32
	strcpy (prefs.sounddir, "./sounds");
	{
		char out[256];

		if (get_reg_str ("Software\\Microsoft\\Windows\\CurrentVersion\\"
						 "Explorer\\Shell Folders", "Personal", out, sizeof (out)))
			snprintf (prefs.dccdir, sizeof (prefs.dccdir), "%s\\Downloads", out);
		else
			snprintf (prefs.dccdir, sizeof (prefs.dccdir), "%s\\Downloads", get_xdir_utf8 ());
	}
#else
	snprintf (prefs.sounddir, sizeof (prefs.sounddir), "%s/sounds", get_xdir_utf8 ());
	snprintf (prefs.dccdir, sizeof (prefs.dccdir), "%s/downloads", get_xdir_utf8 ());
#endif
	strcpy (prefs.doubleclickuser, "QUOTE WHOIS %s %s");
	strcpy (prefs.awayreason, _("I'm busy"));
	strcpy (prefs.quitreason, _("Leaving"));
	strcpy (prefs.partreason, prefs.quitreason);
	strcpy (prefs.font_normal, DEF_FONT);
	strcpy (prefs.dnsprogram, "host");

	g_free ((char *)username);
	g_free ((char *)realname);

	fh = open (default_file (), OFLAGS | O_RDONLY);
	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = malloc (st.st_size + 1);
		cfg[0] = '\0';
		i = read (fh, cfg, st.st_size);
		if (i >= 0)
			cfg[i] = '\0';					/* make sure cfg is NULL terminated */
		close (fh);
		i = 0;
		do
		{
			switch (vars[i].type)
			{
			case TYPE_STR:
				cfg_get_str (cfg, vars[i].name, (char *) &prefs + vars[i].offset,
								 vars[i].len);
				break;
			case TYPE_BOOL:
			case TYPE_INT:
				val = cfg_get_int_with_result (cfg, vars[i].name, &res);
				if (res)
					*((int *) &prefs + vars[i].offset) = val;
				break;
			}
			i++;
		}
		while (vars[i].name);

		free (cfg);

	} else
	{
#ifndef WIN32
#ifndef __EMX__
		/* OS/2 uses UID 0 all the time */
		if (getuid () == 0)
			fe_message (_("* Running IRC as root is stupid! You should\n"
							"  create a User Account and use that to login.\n"), FE_MSG_WARN|FE_MSG_WAIT);
#endif
#endif /* !WIN32 */

		mkdir_utf8 (prefs.dccdir);
		mkdir_utf8 (prefs.dcc_completed_dir);
	}
	if (prefs.mainwindow_height < 138)
		prefs.mainwindow_height = 138;
	if (prefs.mainwindow_width < 106)
		prefs.mainwindow_width = 106;

	sp = strchr (prefs.username, ' ');
	if (sp)
		sp[0] = 0;	/* spaces in username would break the login */
}

int
save_config (void)
{
	int fh, i;
	char *new_config, *config;

	check_prefs_dir ();

	config = default_file ();
	new_config = malloc (strlen (config) + 5);
	strcpy (new_config, config);
	strcat (new_config, ".new");
	
	fh = open (new_config, OFLAGS | O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh == -1)
	{
		free (new_config);
		return 0;
	}

	if (!cfg_put_str (fh, "version", VERSION))
	{
		free (new_config);
		return 0;
	}
		
	i = 0;
	do
	{
		switch (vars[i].type)
		{
		case TYPE_STR:
			if (!cfg_put_str (fh, vars[i].name, (char *) &prefs + vars[i].offset))
			{
				free (new_config);
				return 0;
			}
			break;
		case TYPE_INT:
		case TYPE_BOOL:
			if (!cfg_put_int (fh, *((int *) &prefs + vars[i].offset), vars[i].name))
			{
				free (new_config);
				return 0;
			}
		}
		i++;
	}
	while (vars[i].name);

	if (close (fh) == -1)
	{
		free (new_config);
		return 0;
	}

#ifdef WIN32
	unlink (config);	/* win32 can't rename to an existing file */
#endif
	if (rename (new_config, config) == -1)
	{
		free (new_config);
		return 0;
	}
	free (new_config);

	return 1;
}

static void
set_showval (session *sess, const struct prefs *var, char *tbuf)
{
	int len, dots, j;
	static const char *offon[] = { "OFF", "ON" };

	len = strlen (var->name);
	memcpy (tbuf, var->name, len);
	dots = 29 - len;
	if (dots < 0)
		dots = 0;
	tbuf[len++] = '\003';
	tbuf[len++] = '2';
	for (j=0;j<dots;j++)
		tbuf[j + len] = '.';
	len += j;
	switch (var->type)
	{
	case TYPE_STR:
		sprintf (tbuf + len, "\0033:\017 %s\n",
					(char *) &prefs + var->offset);
		break;
	case TYPE_INT:
		sprintf (tbuf + len, "\0033:\017 %d\n",
					*((int *) &prefs + var->offset));
		break;
	case TYPE_BOOL:
		sprintf (tbuf + len, "\0033:\017 %s\n", offon[
					*((int *) &prefs + var->offset)]);
		break;
	}
	PrintText (sess, tbuf);
}

static void
set_list (session * sess, char *tbuf)
{
	int i;

	i = 0;
	do
	{
		set_showval (sess, &vars[i], tbuf);
		i++;
	}
	while (vars[i].name);
}

int
cfg_get_bool (char *var)
{
	int i = 0;

	do
	{
		if (!strcasecmp (var, vars[i].name))
		{
			return *((int *) &prefs + vars[i].offset);
		}
		i++;
	}
	while (vars[i].name);

	return -1;
}

int
cmd_set (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int wild = FALSE;
	int quiet = FALSE;
	int erase = FALSE;
	int i = 0, finds = 0, found;
	int idx = 2;
	char *var, *val;

	if (strcasecmp (word[2], "-quiet") == 0)
	{
		idx++;
		quiet = TRUE;
	}

	if (strcasecmp (word[2], "-e") == 0)
	{
		idx++;
		erase = TRUE;
	}

	var = word[idx];
	val = word_eol[idx+1];

	if (!*var)
	{
		set_list (sess, tbuf);
		return TRUE;
	}

	if ((strchr (var, '*') || strchr (var, '?')) && !*val)
		wild = TRUE;

	if (*val == '=')
		val++;

	do
	{
		if (wild)
			found = !match (var, vars[i].name);
		else
			found = strcasecmp (var, vars[i].name);

		if (found == 0)
		{
			finds++;
			switch (vars[i].type)
			{
			case TYPE_STR:
				if (erase || *val)
				{
					strncpy ((char *) &prefs + vars[i].offset, val, vars[i].len);
					((char *) &prefs)[vars[i].offset + vars[i].len - 1] = 0;
					if (!quiet)
						PrintTextf (sess, "%s set to: %s\n", var, (char *) &prefs + vars[i].offset);
				} else
				{
					set_showval (sess, &vars[i], tbuf);
				}
				break;
			case TYPE_INT:
			case TYPE_BOOL:
				if (*val)
				{
					if (vars[i].type == TYPE_BOOL)
					{
						if (atoi (val))
							*((int *) &prefs + vars[i].offset) = 1;
						else
							*((int *) &prefs + vars[i].offset) = 0;
						if (!strcasecmp (val, "YES") || !strcasecmp (val, "ON"))
							*((int *) &prefs + vars[i].offset) = 1;
						if (!strcasecmp (val, "NO") || !strcasecmp (val, "OFF"))
							*((int *) &prefs + vars[i].offset) = 0;
					} else
					{
						*((int *) &prefs + vars[i].offset) = atoi (val);
					}
					if (!quiet)
						PrintTextf (sess, "%s set to: %d\n", var,
										*((int *) &prefs + vars[i].offset));
				} else
				{
					set_showval (sess, &vars[i], tbuf);
				}
				break;
			}
		}
		i++;
	}
	while (vars[i].name);

	if (!finds && !quiet)
		PrintText (sess, "No such variable.\n");

	return TRUE;
}
