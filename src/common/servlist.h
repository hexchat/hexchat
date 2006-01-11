#ifndef XCHAT_SERVLIST_H
#define XCHAT_SERVLIST_H

typedef struct ircserver
{
	char *hostname;
} ircserver;

typedef struct ircnet
{
	char *name;
	char *nick;
	char *nick2;
	char *user;
	char *real;
	char *pass;
	char *autojoin;
	char *command;
	char *nickserv;
	char *comment;
	char *encoding;
	GSList *servlist;
	int selected;
	guint32 flags;
} ircnet;

extern GSList *network_list;

#define FLAG_CYCLE				1
#define FLAG_USE_GLOBAL			2
#define FLAG_USE_SSL				4
#define FLAG_AUTO_CONNECT		8
#define FLAG_USE_PROXY			16
#define FLAG_ALLOW_INVALID		32
#define FLAG_COUNT				6

void servlist_init (void);
int servlist_save (void);
int servlist_cycle (server *serv);
void servlist_connect (session *sess, ircnet *net, gboolean join);
int servlist_connect_by_netname (session *sess, char *network, gboolean join);
int servlist_auto_connect (session *sess);
int servlist_have_auto (void);
int servlist_check_encoding (char *charset);

ircnet *servlist_net_add (char *name, char *comment, int prepend);
void servlist_net_remove (ircnet *net);
ircnet *servlist_net_find (char *name, int *pos, int (*cmpfunc) (const char *, const char *));
ircnet *servlist_net_find_from_server (char *server_name);

void servlist_server_remove (ircnet *net, ircserver *serv);
ircserver *servlist_server_add (ircnet *net, char *name);
ircserver *servlist_server_find (ircnet *net, char *name, int *pos);

#endif
