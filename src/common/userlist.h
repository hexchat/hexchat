#include <time.h>

#ifndef XCHAT_USERLIST_H
#define XCHAT_USERLIST_H

struct User
{
	char nick[NICKLEN];
	char *hostname;
	char *realname;
	char *servername;
	time_t lasttalk;
	unsigned int access;	/* axs bit field */
	char prefix[2]; /* @ + % */
	unsigned int op:1;
	unsigned int hop:1;
	unsigned int voice:1;
	unsigned int me:1;
	unsigned int away:1;
	unsigned int selected:1;
};

#define USERACCESS_SIZE 12

int userlist_add_hostname (session *sess, char *nick,
									char *hostname, char *realname,
									char *servername, unsigned int away);
void userlist_set_away (session *sess, char *nick, unsigned int away);
struct User *userlist_find (session *sess, char *name);
struct User *userlist_find_global (server *serv, char *name);
void userlist_clear (session *sess);
void userlist_free (session *sess);
void userlist_add (session *sess, char *name, char *hostname);
int userlist_remove (session *sess, char *name);
int userlist_change (session *sess, char *oldname, char *newname);
void userlist_update_mode (session *sess, char *name, char mode, char sign);
GSList *userlist_flat_list (session *sess);
GList *userlist_double_list (session *sess);
void userlist_rehash (session *sess);

#endif
