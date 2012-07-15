#include "../../config.h"

#include <glib.h>
#include <time.h>			/* need time_t */

#ifndef XCHAT_H
#define XCHAT_H

#include "history.h"

#ifndef HAVE_SNPRINTF
#define snprintf g_snprintf
#endif

#ifndef HAVE_VSNPRINTF
#define vsnprintf _vsnprintf
#endif

#ifdef USE_DEBUG
#define malloc(n) xchat_malloc(n, __FILE__, __LINE__)
#define realloc(n, m) xchat_realloc(n, m, __FILE__, __LINE__)
#define free(n) xchat_dfree(n, __FILE__, __LINE__)
#define strdup(n) xchat_strdup(n, __FILE__, __LINE__)
void *xchat_malloc (int size, char *file, int line);
void *xchat_strdup (char *str, char *file, int line);
void xchat_dfree (void *buf, char *file, int line);
void *xchat_realloc (char *old, int len, char *file, int line);
#endif

#ifdef SOCKS
#ifdef __sgi
#include <sys/time.h>
#define INCLUDE_PROTOTYPES 1
#endif
#include <socks.h>
#endif

#ifdef USE_OPENSSL
#include <openssl/ssl.h>		  /* SSL_() */
#endif

#ifdef __EMX__						  /* for o/s 2 */
#define OFLAGS O_BINARY
#define g_ascii_strcasecmp stricmp
#define g_ascii_strncasecmp strnicmp
#define PATH_MAX MAXPATHLEN
#define FILEPATH_LEN_MAX MAXPATHLEN
#endif

/* force a 32bit CMP.L */
#define CMPL(a, c0, c1, c2, c3) (a == (guint32)(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24)))
#define WORDL(c0, c1, c2, c3) (guint32)(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))
#define WORDW(c0, c1) (guint16)(c0 | (c1 << 8))

#ifdef WIN32						/* for win32 */
#define OFLAGS O_BINARY
#define sleep(t) Sleep(t*1000)
#include <direct.h>
#define	F_OK	0
#define	X_OK	1
#define	W_OK	2
#define	R_OK	4
#ifndef S_ISDIR
#define	S_ISDIR(m)	((m) & _S_IFDIR)
#endif
#define NETWORK_PRIVATE
#else									/* for unix */
#define OFLAGS 0
#endif

#define FONTNAMELEN	127
#define PATHLEN		255
#define DOMAINLEN	100
#define NICKLEN		64				/* including the NULL, so 63 really */
#define CHANLEN		300
#define PDIWORDS		32
#define USERNAMELEN 10
#define HIDDEN_CHAR	8			/* invisible character for xtext */

#if defined(ENABLE_NLS) && !defined(_)
#  include <libintl.h>
#  define _(x) gettext(x)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#endif
#if !defined(_)
#  define N_(String) (String)
#  define _(x) (x)
#endif

struct nbexec
{
	int myfd;
	int childpid;
	int tochannel;					  /* making this int keeps the struct 4-byte aligned */
	int iotag;
	char *linebuf;
	int buffill;
	struct session *sess;
};

struct xchatprefs
{
	char nick1[NICKLEN];
	char nick2[NICKLEN];
	char nick3[NICKLEN];
	char realname[127];
	char username[127];
	char nick_suffix[4];			/* Only ever holds a one-character string. */
	char awayreason[256];
	char quitreason[256];
	char partreason[256];
#ifdef WIN32
	char font_normal[4 * FONTNAMELEN + 1];
	char font_main[FONTNAMELEN + 1];
	char font_alternative[3 * FONTNAMELEN + 1];
#else
	char font_normal[FONTNAMELEN + 1];
#endif
	char doubleclickuser[256];
	char spell_langs[64];
	char sounddir[PATHLEN + 1];
	char soundcmd[PATHLEN + 1];
	char background[PATHLEN + 1];
	char dccdir[PATHLEN + 1];
	char dcc_completed_dir[PATHLEN + 1];
	char irc_extra_hilight[300];
	char irc_no_hilight[300];
	char irc_nick_hilight[300];
	char dnsprogram[72];
	char hostname[127];
	char cmdchar[4];
	char logmask[256];
	char stamp_format[64];
	char timestamp_log_format[64];
	char irc_id_ytext[64];
	char irc_id_ntext[64];

	char proxy_host[64];
	int proxy_port;
	int proxy_type; /* 0=disabled, 1=wingate 2=socks4, 3=socks5, 4=http */
	int proxy_use; /* 0=all 1=IRC_ONLY 2=DCC_ONLY */
	unsigned int proxy_auth;
	char proxy_user[32];
	char proxy_pass[32];

	int first_dcc_send_port;
	int last_dcc_send_port;

	int tint_red;
	int tint_green;
	int tint_blue;

	int away_timeout;
	int away_size_max;

	int gui_pane_left_size;
	int gui_pane_right_size;
	int gui_pane_right_size_min;

	int gui_ulist_pos;
	int tab_pos;

	int _tabs_position;
	int tab_layout;
	int max_auto_indent;
	int dcc_blocksize;
	int max_lines;
	int notify_timeout;
	int dcctimeout;
	int dccstalltimeout;
	int dcc_global_max_get_cps;
	int dcc_global_max_send_cps;
	int dcc_max_get_cps;
	int dcc_max_send_cps;
	int mainwindow_left;
	int mainwindow_top;
	int mainwindow_width;
	int mainwindow_height;
	int completion_sort;
	int gui_win_state;
	int gui_url_mod;
	int gui_usermenu;
	int gui_join_dialog;
	int gui_quit_dialog;
#ifdef WIN32
	int gui_one_instance;
#endif
	int dialog_left;
	int dialog_top;
	int dialog_width;
	int dialog_height;
	int dccpermissions;
	int recon_delay;
	int bantype;
	int userlist_sort;
	guint32 local_ip;
	guint32 dcc_ip;
	char dcc_ip_str[DOMAINLEN + 1];

	unsigned int tab_small;
	unsigned int tab_sort;
	unsigned int mainwindow_save;
	unsigned int perc_color;
	unsigned int perc_ascii;
	unsigned int autosave;
	unsigned int autodialog;
	unsigned int autosave_url;
	unsigned int autoreconnect;
	unsigned int autoreconnectonfail;
	unsigned int invisible;
	unsigned int servernotice;
	unsigned int wallops;
	unsigned int skipmotd;
	unsigned int autorejoin;
	unsigned int autocopy_text;
	unsigned int autocopy_stamp;
	unsigned int autocopy_color;
	unsigned int colorednicks;
	unsigned int chanmodebuttons;
	unsigned int userlistbuttons;
	unsigned int showhostname_in_userlist;
	unsigned int nickcompletion;
	unsigned int completion_amount;
	unsigned int tabchannels;
	unsigned int paned_userlist;
	unsigned int autodccchat;
	unsigned int autodccsend;
	unsigned int autoresume;
	unsigned int autoopendccsendwindow;
	unsigned int autoopendccrecvwindow;
	unsigned int autoopendccchatwindow;
	unsigned int transparent;
	unsigned int stripcolor;
	unsigned int timestamp;
	unsigned int fastdccsend;
	unsigned int dcc_send_fillspaces;
	unsigned int dcc_remove;
	unsigned int slist_fav;
	unsigned int slist_skip;
	unsigned int slist_select;
	unsigned int filterbeep;

	unsigned int input_balloon_chans;
	unsigned int input_balloon_hilight;
	unsigned int input_balloon_priv;
	unsigned int input_balloon_time;

	unsigned int input_beep_chans;
	unsigned int input_beep_hilight;
	unsigned int input_beep_priv;

	unsigned int input_flash_chans;
	unsigned int input_flash_hilight;
	unsigned int input_flash_priv;

	unsigned int input_tray_chans;
	unsigned int input_tray_hilight;
	unsigned int input_tray_priv;

	unsigned int truncchans;
	unsigned int privmsgtab;
	unsigned int irc_join_delay;
	unsigned int logging;
	unsigned int timestamp_logs;
	unsigned int newtabstofront;
	unsigned int dccwithnick;
	unsigned int hidever;
	unsigned int ip_from_server;
	unsigned int raw_modes;
	unsigned int show_away_once;
	unsigned int show_away_message;
	unsigned int auto_unmark_away;
	unsigned int away_track;
	unsigned int userhost;
	unsigned int irc_whois_front;
	unsigned int use_server_tab;
	unsigned int notices_tabs;
	unsigned int style_namelistgad;
	unsigned int style_inputbox;
	unsigned int windows_as_tabs;
	unsigned int indent_nicks;
	unsigned int text_replay;
	unsigned int show_marker;
	unsigned int show_separator;
	unsigned int thin_separator;
	unsigned int auto_indent;
	unsigned int wordwrap;
	unsigned int gui_input_spell;
	unsigned int gui_tray;
	unsigned int gui_tray_flags;
	unsigned int gui_tweaks;
	unsigned int _gui_ulist_left;
	unsigned int throttle;
	unsigned int topicbar;
	unsigned int hideuserlist;
	unsigned int hidemenu;
	unsigned int perlwarnings;
	unsigned int lagometer;
	unsigned int throttlemeter;
	unsigned int pingtimeout;
	unsigned int whois_on_notifyonline;
	unsigned int wait_on_exit;
	unsigned int confmode;
	unsigned int utf8_locale;
#ifdef WIN32
	unsigned int identd;
	unsigned int emoticons;
	unsigned int tab_icons;
	unsigned int tab_xp;
#endif

	unsigned int ctcp_number_limit;	/*flood */
	unsigned int ctcp_time_limit;	/*seconds of floods */

	unsigned int msg_number_limit;	/*same deal */
	unsigned int msg_time_limit;

	unsigned int url_grabber;
	unsigned int url_grabber_limit;
	/* Tells us if we need to save, only when they've been edited.
		This is so that we continue using internal defaults (which can
		change in the next release) until the user edits them. */
	unsigned int save_pevents:1;
	unsigned int text_search_case_match;
	unsigned int text_search_backward;
	unsigned int text_search_highlight_all;
	unsigned int text_search_follow;
	unsigned int text_search_regexp;
};

/* Session types */
#define SESS_SERVER	1
#define SESS_CHANNEL	2
#define SESS_DIALOG	3
#define SESS_NOTICES	4
#define SESS_SNOTICES	5

/* Per-Channel Settings */
#define SET_OFF 0
#define SET_ON 1
#define SET_DEFAULT 2 /* use global setting */

typedef struct session
{
	/* Per-Channel Alerts */
	/* use a byte, because we need a pointer to each element */
	guint8 alert_beep;
	guint8 alert_taskbar;
	guint8 alert_tray;

	/* Per-Channel Settings */
	guint8 text_hidejoinpart;
	guint8 text_logging;
	guint8 text_scrollback;

	struct server *server;
	void *usertree_alpha;			/* pure alphabetical tree */
	void *usertree;					/* ordered with Ops first */
	struct User *me;					/* points to myself in the usertree */
	char channel[CHANLEN];
	char waitchannel[CHANLEN];		  /* waiting to join channel (/join sent) */
	char willjoinchannel[CHANLEN];	  /* will issue /join for this channel */
	char channelkey[64];			  /* XXX correct max length? */
	int limit;						  /* channel user limit */
	int logfd;
	int scrollfd;							/* scrollback filedes */
	int scrollwritten;					/* number of lines written */

	char lastnick[NICKLEN];			  /* last nick you /msg'ed */

	struct history history;

	int ops;								/* num. of ops in channel */
	int hops;						  /* num. of half-oped users */
	int voices;							/* num. of voiced people */
	int total;							/* num. of users in channel */

	char *quitreason;
	char *topic;
	char *current_modes;					/* free() me */

	int mode_timeout_tag;

	struct session *lastlog_sess;
	struct nbexec *running_exec;

	struct session_gui *gui;		/* initialized by fe_new_window */
	struct restore_gui *res;

	int type;					/* SESS_* */

	int new_data:1;			/* new data avail? (purple tab) */
	int nick_said:1;		/* your nick mentioned? (blue tab) */
	int msg_said:1;			/* new msg available? (red tab) */

	int ignore_date:1;
	int ignore_mode:1;
	int ignore_names:1;
	int end_of_names:1;
	int doing_who:1;		/* /who sent on this channel */
	int done_away_check:1;	/* done checking for away status changes */
	unsigned int lastlog_regexp:1;	/* this is a lastlog and using regexp */
} session;

struct msproxy_state_t
{
	gint32				clientid;
	gint32				serverid;
	unsigned char		seq_recv;		/* seq number of last packet recv.	*/
	unsigned char		seq_sent;		/* seq number of last packet sent.	*/
};

typedef struct server
{
	/*  server control operations (in server*.c) */
	void (*connect)(struct server *, char *hostname, int port, int no_login);
	void (*disconnect)(struct session *, int sendquit, int err);
	int  (*cleanup)(struct server *);
	void (*flush_queue)(struct server *);
	void (*auto_reconnect)(struct server *, int send_quit, int err);
	/* irc protocol functions (in proto*.c) */
	void (*p_inline)(struct server *, char *buf, int len);
	void (*p_invite)(struct server *, char *channel, char *nick);
	void (*p_cycle)(struct server *, char *channel, char *key);
	void (*p_ctcp)(struct server *, char *to, char *msg);
	void (*p_nctcp)(struct server *, char *to, char *msg);
	void (*p_quit)(struct server *, char *reason);
	void (*p_kick)(struct server *, char *channel, char *nick, char *reason);
	void (*p_part)(struct server *, char *channel, char *reason);
	void (*p_ns_identify)(struct server *, char *pass);
	void (*p_ns_ghost)(struct server *, char *usname, char *pass);
	void (*p_join)(struct server *, char *channel, char *key);
	void (*p_join_list)(struct server *, GSList *channels, GSList *keys);
	void (*p_login)(struct server *, char *user, char *realname);
	void (*p_join_info)(struct server *, char *channel);
	void (*p_mode)(struct server *, char *target, char *mode);
	void (*p_user_list)(struct server *, char *channel);
	void (*p_away_status)(struct server *, char *channel);
	void (*p_whois)(struct server *, char *nicks);
	void (*p_get_ip)(struct server *, char *nick);
	void (*p_get_ip_uh)(struct server *, char *nick);
	void (*p_set_back)(struct server *);
	void (*p_set_away)(struct server *, char *reason);
	void (*p_message)(struct server *, char *channel, char *text);
	void (*p_action)(struct server *, char *channel, char *act);
	void (*p_notice)(struct server *, char *channel, char *text);
	void (*p_topic)(struct server *, char *channel, char *topic);
	void (*p_list_channels)(struct server *, char *arg, int min_users);
	void (*p_change_nick)(struct server *, char *new_nick);
	void (*p_names)(struct server *, char *channel);
	void (*p_ping)(struct server *, char *to, char *timestring);
/*	void (*p_set_away)(struct server *);*/
	int (*p_raw)(struct server *, char *raw);
	int (*p_cmp)(const char *s1, const char *s2);

	int port;
	int sok;					/* is equal to sok4 or sok6 (the one we are using) */
	int sok4;					/* tcp4 socket */
	int sok6;					/* tcp6 socket */
	int proxy_type;
	int proxy_sok;				/* Additional information for MS Proxy beast */
	int proxy_sok4;
	int proxy_sok6;
	struct msproxy_state_t msp_state;
	int id;					/* unique ID number (for plugin API) */
#ifdef USE_OPENSSL
	SSL *ssl;
	int ssl_do_connect_tag;
#else
	void *ssl;
#endif
	int childread;
	int childwrite;
	int childpid;
	int iotag;
	int recondelay_tag;				/* reconnect delay timeout */
	int joindelay_tag;				/* waiting before we send JOIN */
	char hostname[128];				/* real ip number */
	char servername[128];			/* what the server says is its name */
	char password[86];
	char nick[NICKLEN];
	char linebuf[2048];				/* RFC says 512 chars including \r\n */
	char *last_away_reason;
	int pos;								/* current position in linebuf */
	int nickcount;
	int nickservtype;					/* 0=/MSG nickserv 1=/NICKSERV 2=/NS */

	char *chantypes;					/* for 005 numeric - free me */
	char *chanmodes;					/* for 005 numeric - free me */
	char *nick_prefixes;				/* e.g. "*@%+" */
	char *nick_modes;					/* e.g. "aohv" */
	char *bad_nick_prefixes;		/* for ircd that doesn't give the modes */
	int modes_per_line;				/* 6 on undernet, 4 on efnet etc... */

	void *network;						/* points to entry in servlist.c or NULL! */

	GSList *outbound_queue;
	time_t next_send;						/* cptr->since in ircu */
	time_t prev_now;					/* previous now-time */
	int sendq_len;						/* queue size */
	int lag;								/* milliseconds */

	struct session *front_session;	/* front-most window/tab */
	struct session *server_session;	/* server window/tab */

	struct server_gui *gui;		  /* initialized by fe_new_server */

	unsigned int ctcp_counter;	  /*flood */
	time_t ctcp_last_time;

	unsigned int msg_counter;	  /*counts the msg tab opened in a certain time */
	time_t msg_last_time;

	/*time_t connect_time;*/				/* when did it connect? */
	time_t lag_sent;
	time_t ping_recv;					/* when we last got a ping reply */
	time_t away_time;					/* when we were marked away */

	char *encoding;					/* NULL for system */
	char *autojoin;			/* list of channels & keys to join */

	unsigned int motd_skipped:1;
	unsigned int connected:1;
	unsigned int connecting:1;
	unsigned int no_login:1;
	unsigned int skip_next_userhost:1;/* used for "get my ip from server" */
	unsigned int skip_next_whois:1;	/* hide whois output */
	unsigned int inside_whois:1;
	unsigned int doing_dns:1;			/* /dns has been done */
	unsigned int end_of_motd:1;		/* end of motd reached (logged in) */
	unsigned int sent_quit:1;			/* sent a QUIT already? */
	unsigned int use_listargs:1;		/* undernet and dalnet need /list >0,<10000 */
	unsigned int is_away:1;
	unsigned int reconnect_away:1;	/* whether to reconnect in is_away state */
	unsigned int dont_use_proxy:1;	/* to proxy or not to proxy */
	unsigned int supports_watch:1;	/* supports the WATCH command */
	unsigned int bad_prefix:1;			/* gave us a bad PREFIX= 005 number */
	unsigned int have_namesx:1;		/* 005 tokens NAMESX and UHNAMES */
	unsigned int have_uhnames:1;
	unsigned int have_whox:1;		/* have undernet's WHOX features */
	unsigned int have_capab:1;		/* supports CAPAB (005 tells us) */
	unsigned int have_idmsg:1;		/* freenode's IDENTIFY-MSG */
	unsigned int have_except:1;	/* ban exemptions +e */
	unsigned int using_cp1255:1;	/* encoding is CP1255/WINDOWS-1255? */
	unsigned int using_irc:1;		/* encoding is "IRC" (CP1252/UTF-8 hybrid)? */
	unsigned int use_who:1;			/* whether to use WHO command to get dcc_ip */
#ifdef USE_OPENSSL
	unsigned int use_ssl:1;				  /* is server SSL capable? */
	unsigned int accept_invalid_cert:1;/* ignore result of server's cert. verify */
#endif
} server;

typedef int (*cmd_callback) (struct session * sess, char *tbuf, char *word[],
									  char *word_eol[]);

struct commands
{
	char *name;
	cmd_callback callback;
	char needserver;
	char needchannel;
	gint16 handle_quotes;
	char *help;
};

struct away_msg
{
	struct server *server;
	char nick[NICKLEN];
	char *message;
};

/* not just for popups, but used for usercommands, ctcp replies,
   userlist buttons etc */

struct popup
{
	char *cmd;
	char *name;
};

/* CL: get a random int in the range [0..n-1]. DON'T use rand() % n, it gives terrible results. */
#define RAND_INT(n) ((int)(rand() / (RAND_MAX + 1.0) * (n)))

#ifdef WIN32
#define xchat_filename_from_utf8 g_locale_from_utf8
#define xchat_filename_to_utf8 g_locale_to_utf8
#else
#define xchat_filename_from_utf8 g_filename_from_utf8
#define xchat_filename_to_utf8 g_filename_to_utf8
#endif

#endif
