#include <time.h>

struct User
{
	char nick[NICKLEN];
	char *hostname;
	char *realname;
	char *servername;
	time_t lasttalk;
	int weight;
	unsigned int access;	/* axs bit field */
	char prefix[2]; /* @ + % */
	unsigned int op:1;
	unsigned int hop:1;
	unsigned int voice:1;
};

#define USERACCESS_SIZE 16

int userlist_add_hostname (struct session *sess, char *nick,
											 char *hostname, char *realname,
											 char *servername);
struct User *find_name (struct session *sess, char *name);
struct User *find_name_global (struct server *serv, char *name);
void update_user_list (struct session *sess);
void clear_user_list (struct session *sess);
void free_userlist (struct session *sess);
void add_name (struct session *sess, char *name, char *hostname);
int sub_name (struct session *sess, char *name);
int change_nick (struct session *sess, char *oldname, char *newname);
void ul_update_entry (session *sess, char *name, char mode, char sign);
void update_all_of (char *name);
