#ifndef XCHAT_SERVER_H
#define XCHAT_SERVER_H

extern GSList *serv_list;

/* eventually need to keep the tcp_* functions isolated to server.c */
int tcp_send_len (server *serv, char *buf, int len);
int tcp_send (server *serv, char *buf);
void tcp_sendf (server *serv, char *fmt, ...);

server *server_new (void);
int is_server (server *serv);
void server_fill_her_up (server *serv);
void server_set_encoding (server *serv, char *new_encoding);
void server_set_defaults (server *serv);
char *server_get_network (server *serv, gboolean fallback);
void server_set_name (server *serv, char *name);
void server_free (server *serv);

void server_away_save_message (server *serv, char *nick, char *msg);
struct away_msg *server_away_find_message (server *serv, char *nick);

#endif
