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

#ifndef HEXCHAT_C_H
#define HEXCHAT_C_H

extern struct hexchatprefs prefs;

extern int hexchat_is_quitting;
extern gint arg_skip_plugins;	/* command-line args */
extern gint arg_dont_autoconnect;
extern char *arg_url;
extern char **arg_urls;
extern char *arg_command;
extern gint arg_existing;

extern session *current_sess;
extern session *current_tab;

extern GSList *popup_list;
extern GSList *button_list;
extern GSList *dlgbutton_list;
extern GSList *command_list;
extern GSList *ctcp_list;
extern GSList *replace_list;
extern GSList *sess_list;
extern GSList *dcc_list;
extern GSList *ignore_list;
extern GSList *usermenu_list;
extern GSList *urlhandler_list;
extern GSList *tabmenu_list;
extern GList *sess_list_by_lastact[];

session * find_channel (server *serv, char *chan);
session * find_dialog (server *serv, char *nick);
session * new_ircwindow (server *serv, char *name, int type, int focus);
void hexchat_reinit_timers (void);
void lastact_update (session * sess);
session * lastact_getfirst (int (*filter) (session *sess));
int is_session (session * sess);
void session_free (session *killsess);
void lag_check (void);
void hexchat_exit (void);
void hexchat_exec (const char *cmd);

#endif
