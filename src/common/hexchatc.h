#ifndef HEXCHAT_C_H
#define HEXCHAT_C_H

extern struct hexchatprefs prefs;

extern int hexchat_is_quitting;
extern gint arg_skip_plugins;	/* command-line args */
extern gint arg_dont_autoconnect;
extern char *arg_url;
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
void lastact_update (session * sess);
session * lastact_getfirst (int (*filter) (session *sess));
int is_session (session * sess);
void session_free (session *killsess);
void lag_check (void);
void hexchat_exit (void);
void hexchat_exec (const char *cmd);
void hexchat_execv (char * const argv[]);

#endif
