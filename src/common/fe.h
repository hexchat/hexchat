/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

#include "userlist.h"
#include "dcc.h"

#ifndef HEXCHAT_FE_H
#define HEXCHAT_FE_H

/* for storage of /menu entries */
typedef struct
{
	gint32 pos;	/* position */
	gint16 modifier;	/* keybinding */
	gint16 root_offset;	/* bytes to offset ->path */

	char is_main;	/* is part of the Main menu? (not a popup) */
	char state;	/* state of toggle items */
	char markup;	/* use pango markup? */
	char enable;	/* enabled? sensitivity */

	int key;
	char *path;
	char *label;
	char *cmd;
	char *ucmd;	/* unselect command (toggles) */
	char *group;	/* for radio items or NULL */
	char *icon;	/* filename */
} menu_entry;

int fe_args (int argc, char *argv[]);
void fe_init (void);
void fe_main (void);
void fe_cleanup (void);
void fe_exit (void);
int fe_timeout_add (int interval, void *callback, void *userdata);
int fe_timeout_add_seconds (int interval, void *callback, void *userdata);
void fe_timeout_remove (int tag);
void fe_new_window (struct session *sess, int focus);
void fe_new_server (struct server *serv);
void fe_add_rawlog (struct server *serv, char *text, int len, int outbound);
#define FE_MSG_WAIT 1
#define FE_MSG_INFO 2
#define FE_MSG_WARN 4
#define FE_MSG_ERROR 8
#define FE_MSG_MARKUP 16
void fe_message (char *msg, int flags);
#define FIA_READ 1
#define FIA_WRITE 2
#define FIA_EX 4
#define FIA_FD 8
int fe_input_add (int sok, int flags, void *func, void *data);
void fe_input_remove (int tag);
void fe_idle_add (void *func, void *data);
void fe_set_topic (struct session *sess, char *topic, char *stripped_topic);
typedef enum
{
	FE_COLOR_NONE = 0,
	FE_COLOR_NEW_DATA = 1,
	FE_COLOR_NEW_MSG = 2,
	FE_COLOR_NEW_HILIGHT = 3,
	FE_COLOR_FLAG_NOOVERRIDE = 8,
} tabcolor;
#define FE_COLOR_ALLFLAGS (FE_COLOR_FLAG_NOOVERRIDE)
void fe_set_tab_color (struct session *sess, tabcolor col);
void fe_flash_window (struct session *sess);
void fe_update_mode_buttons (struct session *sess, char mode, char sign);
void fe_update_channel_key (struct session *sess);
void fe_update_channel_limit (struct session *sess);
int fe_is_chanwindow (struct server *serv);
void fe_add_chan_list (struct server *serv, char *chan, char *users,
							  char *topic);
void fe_chan_list_end (struct server *serv);
gboolean fe_add_ban_list (struct session *sess, char *mask, char *who, char *when, int rplcode);
gboolean fe_ban_list_end (struct session *sess, int rplcode);
void fe_notify_update (char *name);
void fe_notify_ask (char *name, char *networks);
void fe_text_clear (struct session *sess, int lines);
void fe_close_window (struct session *sess);
void fe_progressbar_start (struct session *sess);
void fe_progressbar_end (struct server *serv);
void fe_print_text (struct session *sess, char *text, time_t stamp,
					gboolean no_activity);
void fe_userlist_insert (struct session *sess, struct User *newuser, gboolean sel);
int fe_userlist_remove (struct session *sess, struct User *user);
void fe_userlist_rehash (struct session *sess, struct User *user);
void fe_userlist_update (struct session *sess, struct User *user);
void fe_userlist_numbers (struct session *sess);
void fe_userlist_clear (struct session *sess);
void fe_userlist_set_selected (struct session *sess);
void fe_uselect (session *sess, char *word[], int do_clear, int scroll_to);
void fe_dcc_add (struct DCC *dcc);
void fe_dcc_update (struct DCC *dcc);
void fe_dcc_remove (struct DCC *dcc);
int fe_dcc_open_recv_win (int passive);
int fe_dcc_open_send_win (int passive);
int fe_dcc_open_chat_win (int passive);
void fe_clear_channel (struct session *sess);
void fe_session_callback (struct session *sess);
void fe_server_callback (struct server *serv);
void fe_url_add (const char *text);
void fe_pluginlist_update (void);
void fe_buttons_update (struct session *sess);
void fe_dlgbuttons_update (struct session *sess);
void fe_dcc_send_filereq (struct session *sess, char *nick, int maxcps, int passive);
void fe_set_channel (struct session *sess);
void fe_set_title (struct session *sess);
void fe_set_nonchannel (struct session *sess, int state);
void fe_set_nick (struct server *serv, char *newnick);
void fe_ignore_update (int level);
void fe_beep (session *sess);
void fe_lastlog (session *sess, session *lastlog_sess, char *sstr, gtk_xtext_search_flags flags);
void fe_set_lag (server *serv, long lag);
void fe_set_throttle (server *serv);
void fe_set_away (server *serv);
void fe_serverlist_open (session *sess);
void fe_get_bool (char *title, char *prompt, void *callback, void *userdata);
void fe_get_str (char *prompt, char *def, void *callback, void *ud);
void fe_get_int (char *prompt, int def, void *callback, void *ud);
#define FRF_WRITE 1				/* save file */
#define FRF_MULTIPLE 2			/* multi-select */
#define FRF_RECENTLYUSED 4		/* let gtk decide start dir instead of our config */
#define FRF_CHOOSEFOLDER 8		/* choosing a folder only */
#define FRF_FILTERISINITIAL 16	/* filter is initial directory */
#define FRF_NOASKOVERWRITE 32	/* don't ask to overwrite existing files */
#define FRF_EXTENSIONS 64		/* specify file extensions to be displayed */
#define FRF_MIMETYPES 128		/* specify file mimetypes to be displayed */
#define FRF_MODAL 256           /* dialog should be modal to parent */
void fe_get_file (const char *title, char *initial,
				 void (*callback) (void *userdata, char *file), void *userdata,
				 int flags);
typedef enum {
	FE_GUI_HIDE,
	FE_GUI_SHOW,
	FE_GUI_FOCUS,
	FE_GUI_FLASH,
	FE_GUI_COLOR,
	FE_GUI_ICONIFY,
	FE_GUI_MENU,
	FE_GUI_ATTACH,
	FE_GUI_APPLY,
} fe_gui_action;
void fe_ctrl_gui (session *sess, fe_gui_action action, int arg);
int fe_gui_info (session *sess, int info_type);
void *fe_gui_info_ptr (session *sess, int info_type);
void fe_confirm (const char *message, void (*yesproc)(void *), void (*noproc)(void *), void *ud);
char *fe_get_inputbox_contents (struct session *sess);
int fe_get_inputbox_cursor (struct session *sess);
void fe_set_inputbox_contents (struct session *sess, char *text);
void fe_set_inputbox_cursor (struct session *sess, int delta, int pos);
void fe_open_url (const char *url);
void fe_menu_del (menu_entry *);
char *fe_menu_add (menu_entry *);
void fe_menu_update (menu_entry *);
#define FE_SE_CONNECT 0
#define FE_SE_LOGGEDIN 1
#define FE_SE_DISCONNECT 2
#define FE_SE_RECONDELAY 3
#define FE_SE_CONNECTING 4
void fe_server_event (server *serv, int type, int arg);
/* pass NULL filename2 for default HexChat icon */
void fe_tray_set_flash (const char *filename1, const char *filename2, int timeout);
/* pass NULL filename for default HexChat icon */
void fe_tray_set_file (const char *filename);
typedef enum
{
	FE_ICON_NORMAL = 0,
	FE_ICON_MESSAGE = 2,
	FE_ICON_HIGHLIGHT = 5,
	FE_ICON_PRIVMSG = 8,
	FE_ICON_FILEOFFER = 11
} feicon;
void fe_tray_set_icon (feicon icon);
void fe_tray_set_tooltip (const char *text);
void fe_open_chan_list (server *serv, char *filter, int do_refresh);
const char *fe_get_default_font (void);

#endif
