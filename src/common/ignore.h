#ifndef XCHAT_IGNORE_H
#define XCHAT_IGNORE_H

extern GSList *ignore_list;

extern int ignored_ctcp;
extern int ignored_priv;
extern int ignored_chan;
extern int ignored_noti;
extern int ignored_invi;

#define IG_PRIV	1
#define IG_NOTI	2
#define IG_CHAN	4
#define IG_CTCP	8
#define IG_INVI	16
#define IG_UNIG	32
#define IG_NOSAVE	64
#define IG_DCC		128

struct ignore
{
	char *mask;
	unsigned int type;	/* one of more of IG_* ORed together */
};

struct ignore *ignore_exists (char *mask);
int ignore_add (char *mask, int type);
void ignore_showlist (session *sess);
int ignore_del (char *mask, struct ignore *ig);
int ignore_check (char *mask, int type);
void ignore_load (void);
void ignore_save (void);
void ignore_gui_open (void);
void ignore_gui_update (int level);
int flood_check (char *nick, char *ip, server *serv, session *sess, int what);

#endif
