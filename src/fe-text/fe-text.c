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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef WIN32
#include <io.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#else
#include <unistd.h>
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <ctype.h>
#include <glib-object.h>
#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/cfgfiles.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include "../common/fe.h"
#include "fe-text.h"


static int done = FALSE;		  /* finished ? */


static void
send_command (char *cmd)
{
	handle_multiline (current_tab, cmd, TRUE, FALSE);
}

static gboolean
handle_line (GIOChannel *channel, GIOCondition cond, gpointer data)
{

	gchar *str_return;
	gsize length, terminator_pos;
	GError *error = NULL;
	GIOStatus result;

	result = g_io_channel_read_line(channel, &str_return, &length, &terminator_pos, &error);
	if (result == G_IO_STATUS_ERROR || result == G_IO_STATUS_EOF) {
		return FALSE;
	}
	else {
		send_command(str_return);
		g_free(str_return);
		return TRUE;
	}
}

static int done_intro = 0;

void
fe_new_window (struct session *sess, int focus)
{
	char buf[512];

	sess->gui = malloc (4);
	current_sess = sess;

	if (!sess->server->front_session)
		sess->server->front_session = sess;
	if (!sess->server->server_session)
		sess->server->server_session = sess;
	if (!current_tab || focus)
		current_tab = sess;

	if (done_intro)
		return;
	done_intro = 1;

	snprintf (buf, sizeof (buf),
				"\n"
				" \017HexChat-Text \00310"PACKAGE_VERSION"\n"
				" \017Running on \00310%s \017glib \00310%d.%d.%d\n"
				" \017This binary compiled \00310"__DATE__"\017\n",
				get_sys_str (1),
				glib_major_version, glib_minor_version, glib_micro_version);
	fe_print_text (sess, buf, 0, FALSE);

	fe_print_text (sess, "\n\nCompiled in Features\0032:\017 "
#ifdef USE_PLUGIN
	"Plugin "
#endif
#ifdef ENABLE_NLS
	"NLS "
#endif
#ifdef USE_OPENSSL
	"OpenSSL "
#endif
#ifdef USE_IPV6
	"IPv6"
#endif
	"\n\n", 0, FALSE);
	fflush (stdout);
}

static int
get_stamp_str (time_t tim, char *dest, int size)
{
	return strftime_validated (dest, size, prefs.hex_stamp_text_format, localtime (&tim));
}

static int
timecat (char *buf, time_t stamp)
{
	char stampbuf[64];

	/* set the stamp to the current time if not provided */
	if (!stamp)
		stamp = time (0);

	get_stamp_str (stamp, stampbuf, sizeof (stampbuf));
	strcat (buf, stampbuf);
	return strlen (stampbuf);
}

/* Windows doesn't handle ANSI codes in cmd.exe, need to not display them */
#ifndef WIN32
/*                       0  1  2  3  4  5  6  7   8   9   10 11  12  13  14 15 */
static const short colconv[] = { 0, 7, 4, 2, 1, 3, 5, 11, 13, 12, 6, 16, 14, 15, 10, 7 };

void
fe_print_text (struct session *sess, char *text, time_t stamp,
			   gboolean no_activity)
{
	int dotime = FALSE;
	char num[8];
	int reverse = 0, under = 0, bold = 0,
		comma, k, i = 0, j = 0, len = strlen (text);
	unsigned char *newtext = malloc (len + 1024);

	if (prefs.hex_stamp_text)
	{
		newtext[0] = 0;
		j += timecat (newtext, stamp);
	}
	while (i < len)
	{
		if (dotime && text[i] != 0)
		{
			dotime = FALSE;
			newtext[j] = 0;
			j += timecat (newtext, stamp);
		}
		switch (text[i])
		{
		case 3:
			i++;
			if (!isdigit (text[i]))
			{
				newtext[j] = 27;
				j++;
				newtext[j] = '[';
				j++;
				newtext[j] = 'm';
				j++;
				goto endloop;
			}
			k = 0;
			comma = FALSE;
			while (i < len)
			{
				if (text[i] >= '0' && text[i] <= '9' && k < 2)
				{
					num[k] = text[i];
					k++;
				} else
				{
					int col, mirc;
					num[k] = 0;
					newtext[j] = 27;
					j++;
					newtext[j] = '[';
					j++;
					if (k == 0)
					{
						newtext[j] = 'm';
						j++;
					} else
					{
						if (comma)
							col = 40;
						else
							col = 30;
						mirc = atoi (num);
						mirc = colconv[mirc];
						if (mirc > 9)
						{
							mirc += 50;
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						} else
						{
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						}
						j = strlen (newtext);
					}
					switch (text[i])
					{
					case ',':
						comma = TRUE;
						break;
					default:
						goto endloop;
					}
					k = 0;
				}
				i++;
			}
			break;
		/* don't actually want hidden text */
		case '\010':				  /* hidden */
			break;
		case '\026':				  /* REVERSE */
			if (reverse)
			{
				reverse = FALSE;
				strcpy (&newtext[j], "\033[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\033[7m");
			}
			j = strlen (newtext);
			break;
		case '\037':				  /* underline */
			if (under)
			{
				under = FALSE;
				strcpy (&newtext[j], "\033[24m");
			} else
			{
				under = TRUE;
				strcpy (&newtext[j], "\033[4m");
			}
			j = strlen (newtext);
			break;
		case '\002':				  /* bold */
			if (bold)
			{
				bold = FALSE;
				strcpy (&newtext[j], "\033[22m");
			} else
			{
				bold = TRUE;
				strcpy (&newtext[j], "\033[1m");
			}
			j = strlen (newtext);
			break;
		case '\007':
			if (!prefs.hex_input_filter_beep)
			{
				newtext[j] = text[i];
				j++;
			}
			break;
		case '\017':				  /* reset all */
			strcpy (&newtext[j], "\033[m");
			j += 3;
			reverse = FALSE;
			bold = FALSE;
			under = FALSE;
			break;
		case '\t':
			newtext[j] = ' ';
			j++;
			break;
		case '\n':
			newtext[j] = '\r';
			j++;
			if (prefs.hex_stamp_text)
				dotime = TRUE;
		default:
			newtext[j] = text[i];
			j++;
		}
		i++;
		endloop:
			;
	}

	/* make sure last character is a new line */
	if (text[i-1] != '\n')
		newtext[j++] = '\n';

	newtext[j] = 0;
	write (STDOUT_FILENO, newtext, j);
	free (newtext);
}
#else
/* The win32 version for cmd.exe */
void
fe_print_text (struct session *sess, char *text, time_t stamp,
			   gboolean no_activity)
{
	int dotime = FALSE;
	int comma, k, i = 0, j = 0, len = strlen (text);

	unsigned char *newtext = malloc (len + 1024);

	if (prefs.hex_stamp_text)
	{
		newtext[0] = 0;
		j += timecat (newtext, stamp);
	}
	while (i < len)
	{
		if (dotime && text[i] != 0)
		{
			dotime = FALSE;
			newtext[j] = 0;
			j += timecat (newtext, stamp);
		}
		switch (text[i])
		{
		case 3:
			i++;
			if (!isdigit (text[i]))
			{
				goto endloop;
			}
			k = 0;
			comma = FALSE;
			while (i < len)
			{
				if (text[i] >= '0' && text[i] <= '9' && k < 2)
				{
					k++;
				} else
				{
					switch (text[i])
					{
					case ',':
						comma = TRUE;
						break;
					default:
						goto endloop;
					}
					k = 0;

				}
				i++;
			}
			break;
		/* don't actually want hidden text */
		case '\010':				  /* hidden */
		case '\026':				  /* REVERSE */
		case '\037':				  /* underline */
		case '\002':				  /* bold */
		case '\017':				  /* reset all */
			break;
		case '\007':
			if (!prefs.hex_input_filter_beep)
			{
				newtext[j] = text[i];
				j++;
			}
			break;
		case '\t':
			newtext[j] = ' ';
			j++;
			break;
		case '\n':
			newtext[j] = '\r';
			j++;
			if (prefs.hex_stamp_text)
				dotime = TRUE;
		default:
			newtext[j] = text[i];
			j++;
		}
		i++;
		endloop:
			;
	}

	/* make sure last character is a new line */
	if (text[i-1] != '\n')
		newtext[j++] = '\n';

	newtext[j] = 0;
	write (STDOUT_FILENO, newtext, j);
	free (newtext);
}
#endif

void
fe_timeout_remove (int tag)
{
	g_source_remove (tag);
}

int
fe_timeout_add (int interval, void *callback, void *userdata)
{
	return g_timeout_add (interval, (GSourceFunc) callback, userdata);
}

void
fe_input_remove (int tag)
{
	g_source_remove (tag);
}

int
fe_input_add (int sok, int flags, void *func, void *data)
{
	int tag, type = 0;
	GIOChannel *channel;

#ifdef G_OS_WIN32
	if (flags & FIA_FD)
		channel = g_io_channel_win32_new_fd (sok);
	else
		channel = g_io_channel_win32_new_socket (sok);
#else
	channel = g_io_channel_unix_new (sok);
#endif

	if (flags & FIA_READ)
		type |= G_IO_IN | G_IO_HUP | G_IO_ERR;
	if (flags & FIA_WRITE)
		type |= G_IO_OUT | G_IO_ERR;
	if (flags & FIA_EX)
		type |= G_IO_PRI;

	tag = g_io_add_watch (channel, type, (GIOFunc) func, data);
	g_io_channel_unref (channel);

	return tag;
}

/* === command-line parameter parsing : requires glib 2.6 === */

static char *arg_cfgdir = NULL;
static gint arg_show_autoload = 0;
static gint arg_show_config = 0;
static gint arg_show_version = 0;

static const GOptionEntry gopt_entries[] = 
{
 {"no-auto",	'a', 0, G_OPTION_ARG_NONE,	&arg_dont_autoconnect, N_("Don't auto connect to servers"), NULL},
 {"cfgdir",	'd', 0, G_OPTION_ARG_STRING,	&arg_cfgdir, N_("Use a different config directory"), "PATH"},
 {"no-plugins",	'n', 0, G_OPTION_ARG_NONE,	&arg_skip_plugins, N_("Don't auto load any plugins"), NULL},
 {"plugindir",	'p', 0, G_OPTION_ARG_NONE,	&arg_show_autoload, N_("Show plugin/script auto-load directory"), NULL},
 {"configdir",	'u', 0, G_OPTION_ARG_NONE,	&arg_show_config, N_("Show user config directory"), NULL},
 {"url",	 0,  G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,	&arg_url, N_("Open an irc://server:port/channel URL"), "URL"},
 {"version",	'v', 0, G_OPTION_ARG_NONE,	&arg_show_version, N_("Show version information"), NULL},
 {G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &arg_urls, N_("Open an irc://server:port/channel?key URL"), "URL"},
 {NULL}
};

int
fe_args (int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, gopt_entries, GETTEXT_PACKAGE);
	g_option_context_parse (context, &argc, &argv, &error);

	if (error)
	{
		if (error->message)
			printf ("%s\n", error->message);
		return 1;
	}

	g_option_context_free (context);

	if (arg_show_version)
	{
		printf (PACKAGE_TARNAME" "PACKAGE_VERSION"\n");
		return 0;
	}

	if (arg_show_autoload)
	{
#ifdef WIN32
		/* see the chdir() below */
		char *sl, *exe = strdup (argv[0]);
		sl = strrchr (exe, '\\');
		if (sl)
		{
			*sl = 0;
			printf ("%s\\plugins\n", exe);
		}
		free (exe);
#else
		printf ("%s\n", HEXCHATLIBDIR);
#endif
		return 0;
	}

	if (arg_show_config)
	{
		printf ("%s\n", get_xdir ());
		return 0;
	}

	if (arg_cfgdir)	/* we want filesystem encoding */
	{
		if (xdir)
			g_free (xdir);
		xdir = strdup (arg_cfgdir);
		if (xdir[strlen (xdir) - 1] == '/')
			xdir[strlen (xdir) - 1] = 0;
		g_free (arg_cfgdir);
	}

	return -1;
}

void
fe_init (void)
{
	/* the following should be default generated, not enfoced in binary */
	prefs.hex_gui_tab_server = 0;
	prefs.hex_gui_autoopen_dialog = 0;
	/* except for these, there is no lag meter, there is no server list */
	prefs.hex_gui_lagometer = 0;
	prefs.hex_gui_slist_skip = 1;
}

void
fe_main (void)
{
	GIOChannel *keyboard_input;

	main_loop = g_main_loop_new(NULL, FALSE);

	/* Keyboard Entry Setup */
#ifdef G_OS_WIN32
	keyboard_input = g_io_channel_win32_new_fd(STDIN_FILENO);
#else
	keyboard_input = g_io_channel_unix_new(STDIN_FILENO);
#endif

	g_io_add_watch(keyboard_input, G_IO_IN, handle_line, NULL);

	g_main_loop_run(main_loop);

	return;
}

void
fe_exit (void)
{
	done = TRUE;
	g_main_loop_quit(main_loop);
}

void
fe_new_server (struct server *serv)
{
	serv->gui = malloc (4);
}

void
fe_message (char *msg, int flags)
{
	puts (msg);
}

void
fe_close_window (struct session *sess)
{
	session_free (sess);
	done = TRUE;
}

void
fe_beep (session *sess)
{
	putchar (7);
}

void
fe_add_rawlog (struct server *serv, char *text, int len, int outbound)
{
}
void
fe_set_topic (struct session *sess, char *topic, char *stripped_topic)
{
}
void
fe_cleanup (void)
{
}
void
fe_set_hilight (struct session *sess)
{
}
void
fe_set_tab_color (struct session *sess, int col)
{
}
void
fe_update_mode_buttons (struct session *sess, char mode, char sign)
{
}
void
fe_update_channel_key (struct session *sess)
{
}
void
fe_update_channel_limit (struct session *sess)
{
}
int
fe_is_chanwindow (struct server *serv)
{
	return 0;
}

void
fe_add_chan_list (struct server *serv, char *chan, char *users, char *topic)
{
}
void
fe_chan_list_end (struct server *serv)
{
}
gboolean
fe_add_ban_list (struct session *sess, char *mask, char *who, char *when, int rplcode)
{
	return 0;
}
gboolean
fe_ban_list_end (struct session *sess, int rplcode)
{
	return 0;
}
void
fe_notify_update (char *name)
{
}
void
fe_notify_ask (char *name, char *networks)
{
}
void
fe_text_clear (struct session *sess, int lines)
{
}
void
fe_progressbar_start (struct session *sess)
{
}
void
fe_progressbar_end (struct server *serv)
{
}
void
fe_userlist_insert (struct session *sess, struct User *newuser, int row, int sel)
{
}
int
fe_userlist_remove (struct session *sess, struct User *user)
{
	return 0;
}
void
fe_userlist_rehash (struct session *sess, struct User *user)
{
}
void
fe_userlist_move (struct session *sess, struct User *user, int new_row)
{
}
void
fe_userlist_numbers (struct session *sess)
{
}
void
fe_userlist_clear (struct session *sess)
{
}
void
fe_userlist_set_selected (struct session *sess)
{
}
void
fe_dcc_add (struct DCC *dcc)
{
}
void
fe_dcc_update (struct DCC *dcc)
{
}
void
fe_dcc_remove (struct DCC *dcc)
{
}
void
fe_clear_channel (struct session *sess)
{
}
void
fe_session_callback (struct session *sess)
{
}
void
fe_server_callback (struct server *serv)
{
}
void
fe_url_add (const char *text)
{
}
void
fe_pluginlist_update (void)
{
}
void
fe_buttons_update (struct session *sess)
{
}
void
fe_dlgbuttons_update (struct session *sess)
{
}
void
fe_dcc_send_filereq (struct session *sess, char *nick, int maxcps, int passive)
{
}
void
fe_set_channel (struct session *sess)
{
}
void
fe_set_title (struct session *sess)
{
}
void
fe_set_nonchannel (struct session *sess, int state)
{
}
void
fe_set_nick (struct server *serv, char *newnick)
{
}
void
fe_change_nick (struct server *serv, char *nick, char *newnick)
{
}
void
fe_ignore_update (int level)
{
}
int
fe_dcc_open_recv_win (int passive)
{
	return FALSE;
}
int
fe_dcc_open_send_win (int passive)
{
	return FALSE;
}
int
fe_dcc_open_chat_win (int passive)
{
	return FALSE;
}
void
fe_userlist_hide (session * sess)
{
}
void
fe_lastlog (session *sess, session *lastlog_sess, char *sstr, gtk_xtext_search_flags flags)
{
}
void
fe_set_lag (server * serv, long lag)
{
}
void
fe_set_throttle (server * serv)
{
}
void
fe_set_away (server *serv)
{
}
void
fe_serverlist_open (session *sess)
{
}
void
fe_get_bool (char *title, char *prompt, void *callback, void *userdata)
{
}
void
fe_get_str (char *prompt, char *def, void *callback, void *ud)
{
}
void
fe_get_int (char *prompt, int def, void *callback, void *ud)
{
}
void
fe_idle_add (void *func, void *data)
{
	g_idle_add (func, data);
}
void
fe_ctrl_gui (session *sess, fe_gui_action action, int arg)
{
	/* only one action type handled for now, but could add more */
	switch (action)
	{
	/* gui focus is really the only case hexchat-text needs to wory about */
	case FE_GUI_FOCUS:
		current_sess = sess;
		current_tab = sess;
		sess->server->front_session = sess;
		break;
	default:
		break;
	}
}
int
fe_gui_info (session *sess, int info_type)
{
	return -1;
}
void *
fe_gui_info_ptr (session *sess, int info_type)
{
	return NULL;
}
void fe_confirm (const char *message, void (*yesproc)(void *), void (*noproc)(void *), void *ud)
{
}
char *fe_get_inputbox_contents (struct session *sess)
{
	return NULL;
}
void fe_set_inputbox_contents (struct session *sess, char *text)
{
}
int fe_get_inputbox_cursor (struct session *sess)
{
	return 0;
}
void fe_set_inputbox_cursor (struct session *sess, int delta, int pos)
{
}
void fe_open_url (const char *url)
{
}
void fe_menu_del (menu_entry *me)
{
}
char *fe_menu_add (menu_entry *me)
{
	return NULL;
}
void fe_menu_update (menu_entry *me)
{
}
void fe_uselect (struct session *sess, char *word[], int do_clear, int scroll_to)
{
}
void
fe_server_event (server *serv, int type, int arg)
{
}
void
fe_flash_window (struct session *sess)
{
}
void fe_get_file (const char *title, char *initial,
				 void (*callback) (void *userdata, char *file), void *userdata,
				 int flags)
{
}
void fe_tray_set_flash (const char *filename1, const char *filename2, int timeout){}
void fe_tray_set_file (const char *filename){}
void fe_tray_set_icon (feicon icon){}
void fe_tray_set_tooltip (const char *text){}
void fe_tray_set_balloon (const char *title, const char *text){}
void fe_userlist_update (session *sess, struct User *user){}
void
fe_open_chan_list (server *serv, char *filter, int do_refresh)
{
	serv->p_list_channels (serv, filter, 1);
}
const char *
fe_get_default_font (void)
{
	return NULL;
}
void
fe_sslalert_open (struct server *serv, void (*callback)(int, void *), void *callback_data)
{
	callback (SSLALERT_RESPONSE_ACCEPT, callback_data);
}
