#ifndef XCHAT_NOTIFY_H
#define XCHAT_NOTIFY_H

struct notify
{
	char *name;
	char *networks;	/* network names, comma sep */
	GSList *server_list;
};

struct notify_per_server
{
	struct server *server;
	struct notify *notify;
	time_t laston;
	time_t lastseen;
	time_t lastoff;
	unsigned int ison:1;
};

extern GSList *notify_list;
extern int notify_tag;

/* the WATCH stuff */
void notify_set_online (server * serv, char *nick);
void notify_set_offline (server * serv, char *nick, int quiet);
void notify_send_watches (server * serv);

/* the general stuff */
void notify_adduser (char *name, char *networks);
int notify_deluser (char *name);
void notify_cleanup (void);
void notify_load (void);
void notify_save (void);
void notify_showlist (session *sess);
int notify_isnotify (session *sess, char *name);
struct notify_per_server *notify_find_server_entry (struct notify *notify, struct server *serv);

/* the old ISON stuff - remove me? */
void notify_markonline (server *serv, char *word[]);
int notify_checklist (void);

#endif
