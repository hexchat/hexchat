#ifndef XCHAT_SERVER_H
#define XCHAT_SERVER_H

/* eventually need to keep the tcp_* functions isolated to server.c */
int tcp_send_len (server *serv, char *buf, int len);
int tcp_send (server *serv, char *buf);
void tcp_sendf (server *serv, char *fmt, ...);

void server_fill_her_up (server *serv);

#endif
