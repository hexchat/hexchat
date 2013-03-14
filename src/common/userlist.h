#include <time.h>

#ifndef HEXCHAT_USERLIST_H
#define HEXCHAT_USERLIST_H

struct User
{
	char nick[NICKLEN];
	char *hostname;
	char *realname;
	char *servername;
	char *account;
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
									char *servername, char *account, unsigned int away);
void userlist_set_away (session *sess, char *nick, unsigned int away);
void userlist_set_account (session *sess, char *nick, char *account);
struct User *userlist_find (session *sess, const char *name);
struct User *userlist_find_global (server *serv, char *name);
void userlist_clear (session *sess);
void userlist_free (session *sess);
void userlist_add (session *sess, char *name, char *hostname, char *account, char *realname);
int userlist_remove (session *sess, char *name);
void userlist_remove_user (session *sess, struct User *user);
int userlist_change (session *sess, char *oldname, char *newname);
void userlist_update_mode (session *sess, char *name, char mode, char sign);
GSList *userlist_flat_list (session *sess);
GList *userlist_double_list (session *sess);
void userlist_rehash (session *sess);

#endif
