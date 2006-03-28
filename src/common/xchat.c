/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define WANTSOCKET
#include "inet.h"

#ifndef WIN32
#include <sys/wait.h>
#include <signal.h>
#endif

#include "xchat.h"
#include "fe.h"
#include "util.h"
#include "cfgfiles.h"
#include "ignore.h"
#include "xchat-plugin.h"
#include "plugin.h"
#include "plugin-timer.h"
#include "notify.h"
#include "server.h"
#include "servlist.h"
#include "outbound.h"
#include "text.h"
#include "url.h"
#include "xchatc.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>		  /* SSL_() */
#include "ssl.h"
#endif

GSList *popup_list = 0;
GSList *button_list = 0;
GSList *dlgbutton_list = 0;
GSList *command_list = 0;
GSList *ctcp_list = 0;
GSList *replace_list = 0;
GSList *sess_list = 0;
GSList *dcc_list = 0;
GSList *ignore_list = 0;
GSList *usermenu_list = 0;
GSList *urlhandler_list = 0;
GSList *tabmenu_list = 0;

static int in_xchat_exit = FALSE;
int xchat_is_quitting = FALSE;
/* command-line args */
int arg_dont_autoconnect = FALSE;
int arg_skip_plugins = FALSE;
char *arg_url = NULL;

struct session *current_tab;
struct session *current_sess = 0;
struct xchatprefs prefs;

#ifdef USE_OPENSSL
SSL_CTX *ctx = NULL;
#endif


int
is_session (session * sess)
{
	return g_slist_find (sess_list, sess) ? 1 : 0;
}

session *
find_dialog (server *serv, char *nick)
{
	GSList *list = sess_list;
	session *sess;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv && sess->type == SESS_DIALOG)
		{
			if (!serv->p_cmp (nick, sess->channel))
				return (sess);
		}
		list = list->next;
	}
	return 0;
}

session *
find_channel (server *serv, char *chan)
{
	session *sess;
	GSList *list = sess_list;
	while (list)
	{
		sess = list->data;
		if ((!serv || serv == sess->server) && sess->type != SESS_DIALOG)
		{
			if (!serv->p_cmp (chan, sess->channel))
				return sess;
		}
		list = list->next;
	}
	return 0;
}

static void
lagcheck_update (void)
{
	server *serv;
	GSList *list = serv_list;
	
	if (!prefs.lagometer)
		return;

	while (list)
	{
		serv = list->data;
		if (serv->lag_sent)
			fe_set_lag (serv, -1);

		list = list->next;
	}
}

void
lag_check (void)
{
	server *serv;
	GSList *list = serv_list;
	unsigned long tim;
	char tbuf[128];
	time_t now = time (0);
	int lag;

	tim = make_ping_time ();

	while (list)
	{
		serv = list->data;
		if (serv->connected && serv->end_of_motd)
		{
			lag = now - serv->ping_recv;
			if (prefs.pingtimeout && lag > prefs.pingtimeout && lag > 0)
			{
				sprintf (tbuf, "%d", lag);
				EMIT_SIGNAL (XP_TE_PINGTIMEOUT, serv->server_session, tbuf, NULL,
								 NULL, NULL, 0);
				serv->auto_reconnect (serv, FALSE, -1);
			} else
			{
				snprintf (tbuf, sizeof (tbuf), "LAG%lu", tim);
				serv->p_ping (serv, "", tbuf);
				serv->lag_sent = tim;
				fe_set_lag (serv, -1);
			}
		}
		list = list->next;
	}
}

static int
away_check (void)
{
	session *sess;
	GSList *list;
	int full, sent, loop = 0;

	if (!prefs.away_track || prefs.away_size_max < 1)
		return 1;

doover:
	/* request an update of AWAY status of 1 channel every 30 seconds */
	full = TRUE;
	sent = 0;	/* number of WHOs (users) requested */
	list = sess_list;
	while (list)
	{
		sess = list->data;

		if (sess->server->connected &&
			 sess->type == SESS_CHANNEL &&
			 sess->channel[0] &&
			 sess->total <= prefs.away_size_max)
		{
			if (!sess->done_away_check)
			{
				full = FALSE;

				/* if we're under 31 WHOs, send another channels worth */
				if (sent < 31 && !sess->doing_who)
				{
					sess->done_away_check = TRUE;
					sess->doing_who = TRUE;
					/* this'll send a WHO #channel */
					sess->server->p_away_status (sess->server, sess->channel);
					sent += sess->total;
				}
			}
		}

		list = list->next;
	}

	/* done them all, reset done_away_check to FALSE and start over */
	if (full)
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			sess->done_away_check = FALSE;
			list = list->next;
		}
		loop++;
		if (loop < 2)
			goto doover;
	}

	return 1;
}

static int
xchat_misc_checks (void)		/* this gets called every 1/2 second */
{
	static int count = 0;

	count++;

	lagcheck_update ();			/* every 500ms */

	if (count % 2)
		dcc_check_timeouts ();	/* every 1 second */

	if (count >= 60)				/* every 30 seconds */
	{
		if (prefs.lagometer)
			lag_check ();
		count = 0;
	}

	return 1;
}

/* executed when the first irc window opens */

static void
irc_init (session *sess)
{
	static int done_init = FALSE;

	if (done_init)
		return;

	done_init = TRUE;

#ifdef USE_PLUGIN
	if (!arg_skip_plugins)
		plugin_auto_load (sess);	/* autoload ~/.xchat *.so */
#endif
	plugin_add (sess, NULL, NULL, timer_plugin_init, NULL, NULL, FALSE);

	if (prefs.notify_timeout)
		notify_tag = fe_timeout_add (prefs.notify_timeout * 1000,
											  notify_checklist, 0);

	fe_timeout_add (prefs.away_timeout * 1000, away_check, 0);
	fe_timeout_add (500, xchat_misc_checks, 0);

	if (arg_url != NULL)
	{
		char buf[512];
		snprintf (buf, sizeof (buf), "server %s", arg_url);
		handle_command (sess, buf, FALSE);
		g_free (arg_url);	/* from GOption */
	}
}

static session *
session_new (server *serv, char *from, int type, int focus)
{
	session *sess;

	sess = malloc (sizeof (struct session));
	memset (sess, 0, sizeof (struct session));

	sess->server = serv;
	sess->logfd = -1;
	sess->type = type;
	sess->hide_join_part = prefs.confmode;

	if (from != NULL)
		safe_strcpy (sess->channel, from, CHANLEN);

	sess_list = g_slist_prepend (sess_list, sess);

	fe_new_window (sess, focus);

	return sess;
}

session *
new_ircwindow (server *serv, char *name, int type, int focus)
{
	session *sess;

	switch (type)
	{
	case SESS_SERVER:
		serv = server_new ();
		if (prefs.use_server_tab)
		{
			register unsigned int oldh = prefs.hideuserlist;
			prefs.hideuserlist = 1;
			sess = session_new (serv, name, SESS_SERVER, focus);
			prefs.hideuserlist = oldh;
		} else
		{
			sess = session_new (serv, name, SESS_CHANNEL, focus);
		}
		serv->server_session = sess;
		serv->front_session = sess;
		break;
	case SESS_DIALOG:
		sess = session_new (serv, name, type, focus);
		if (prefs.logging)
			log_open (sess);
		break;
	default:
/*	case SESS_CHANNEL:
	case SESS_NOTICES:
	case SESS_SNOTICES:*/
		sess = session_new (serv, name, type, focus);
		break;
	}

	irc_init (sess);
	plugin_emit_dummy_print (sess, "Open Context");

	return sess;
}

static void
exec_notify_kill (session * sess)
{
#ifndef WIN32
	struct nbexec *re;
	if (sess->running_exec != NULL)
	{
		re = sess->running_exec;
		sess->running_exec = NULL;
		kill (re->childpid, SIGKILL);
		waitpid (re->childpid, NULL, WNOHANG);
		fe_input_remove (re->iotag);
		close (re->myfd);
		if (re->linebuf)
			free(re->linebuf);
		free (re);
	}
#endif
}

static void
send_quit_or_part (session * killsess)
{
	int willquit = TRUE;
	GSList *list;
	session *sess;
	server *killserv = killsess->server;

	/* check if this is the last session using this server */
	list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == killserv && sess != killsess)
		{
			willquit = FALSE;
			list = 0;
		} else
			list = list->next;
	}

	if (xchat_is_quitting)
		willquit = TRUE;

	if (killserv->connected)
	{
		if (willquit)
		{
			if (!killserv->sent_quit)
			{
				killserv->flush_queue (killserv);
				server_sendquit (killsess);
				killserv->sent_quit = TRUE;
			}
		} else
		{
			if (killsess->type == SESS_CHANNEL && killsess->channel[0])
			{
				server_sendpart (killserv, killsess->channel, 0);
			}
		}
	}
}

void
session_free (session *killsess)
{
	server *killserv = killsess->server;
	session *sess;
	GSList *list;

	plugin_emit_dummy_print (killsess, "Close Context");

	if (current_tab == killsess)
		current_tab = NULL;

	if (killserv->server_session == killsess)
		killserv->server_session = NULL;

	if (killserv->front_session == killsess)
	{
		/* front_session is closed, find a valid replacement */
		killserv->front_session = NULL;
		list = sess_list;
		while (list)
		{
			sess = (session *) list->data;
			if (sess != killsess && sess->server == killserv)
			{
				killserv->front_session = sess;
				if (!killserv->server_session)
					killserv->server_session = sess;
				break;
			}
			list = list->next;
		}
	}

	if (!killserv->server_session)
		killserv->server_session = killserv->front_session;

	sess_list = g_slist_remove (sess_list, killsess);

	if (killsess->type == SESS_CHANNEL)
		userlist_free (killsess);

	exec_notify_kill (killsess);

	log_close (killsess);

	send_quit_or_part (killsess);

	history_free (&killsess->history);
	if (killsess->topic)
		free (killsess->topic);
	if (killsess->current_modes)
		free (killsess->current_modes);

	fe_session_callback (killsess);

	if (current_sess == killsess && sess_list)
		current_sess = sess_list->data;

	free (killsess);

	if (!sess_list && !in_xchat_exit)
		xchat_exit ();						/* sess_list is empty, quit! */

	list = sess_list;
	while (list)
	{
		sess = (session *) list->data;
		if (sess->server == killserv)
			return;					  /* this server is still being used! */
		list = list->next;
	}

	server_free (killserv);
}

static void
free_sessions (void)
{
	GSList *list = sess_list;

	while (list)
	{
		fe_close_window (list->data);
		list = sess_list;
	}
}



#define XTERM "gnome-terminal -x "

static char defaultconf_ctcp[] =
	"NAME TIME\n"				"CMD nctcp %s TIME %t\n\n"\
	"NAME PING\n"				"CMD nctcp %s PING %d\n\n";

static char defaultconf_replace[] =
	"NAME teh\n"				"CMD the\n\n";
/*	"NAME r\n"					"CMD are\n\n"\
	"NAME u\n"					"CMD you\n\n"*/

static char defaultconf_commands[] =
	"NAME ACTION\n"		"CMD me &2\n\n"\
	"NAME AME\n"			"CMD allchan me &2\n\n"\
	"NAME ANICK\n"			"CMD allserv nick &2\n\n"\
	"NAME AMSG\n"			"CMD allchan say &2\n\n"\
	"NAME BACK\n"			"CMD away\n\n"\
	"NAME BANLIST\n"		"CMD quote MODE %c +b\n\n"\
	"NAME CHAT\n"			"CMD dcc chat %2\n\n"\
	"NAME DIALOG\n"		"CMD query %2\n\n"\
	"NAME DMSG\n"			"CMD msg =%2 &3\n\n"\
	"NAME EXIT\n"			"CMD quit\n\n"\
	"NAME J\n"				"CMD join &2\n\n"\
	"NAME KILL\n"			"CMD quote KILL %2 :&3\n\n"\
	"NAME LEAVE\n"			"CMD part &2\n\n"\
	"NAME M\n"				"CMD msg &2\n\n"\
	"NAME ONOTICE\n"		"CMD notice @%c &2\n\n"\
	"NAME RAW\n"			"CMD quote &2\n\n"\
	"NAME SERVHELP\n"		"CMD quote HELP\n\n"\
	"NAME SPING\n"			"CMD ping\n\n"\
	"NAME SQUERY\n"		"CMD quote SQUERY %2 :&3\n\n"\
	"NAME SSLSERVER\n"	"CMD server -ssl &2\n\n"\
	"NAME SV\n"				"CMD echo xchat %v %m\n\n"\
	"NAME UMODE\n"			"CMD mode %n &2\n\n"\
	"NAME UPTIME\n"		"CMD quote STATS u\n\n"\
	"NAME VER\n"			"CMD ctcp %2 VERSION\n\n"\
	"NAME VERSION\n"		"CMD ctcp %2 VERSION\n\n"\
	"NAME WALLOPS\n"		"CMD quote WALLOPS :&2\n\n"\
	"NAME WII\n"			"CMD quote WHOIS %2 %2\n\n";

#if 1
static char defaultconf_urlhandlers[] =
		"NAME Open Link in Opera\n"		"CMD !opera -remote 'openURL(%s)'\n\n";
#else
#ifdef WIN32
static char defaultconf_urlhandlers[] =
	"NAME Connect as IRC server\n"		"CMD url %s\n\n";
#else
static char defaultconf_urlhandlers[] =
	"NAME SUB\n"								"CMD Epiphany...\n\n"\
		"NAME Open\n"							"CMD !epiphany '%s'\n\n"\
		"NAME Open in new tab\n"			"CMD !epiphany -n '%s'\n\n"\
		"NAME Open in new window\n"		"CMD !epiphany -w '%s'\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Netscape...\n\n"\
		"NAME Open in existing\n"			"CMD !netscape -remote 'openURL(%s)'\n\n"\
		"NAME Open in new window\n"		"CMD !netscape -remote 'openURL(%s,new-window)'\n\n"\
		"NAME Run new Netscape\n"			"CMD !netscape %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Mozilla...\n\n"\
		"NAME Open in existing\n"			"CMD !mozilla -remote 'openURL(%s)'\n\n"\
		"NAME Open in new window\n"		"CMD !mozilla -remote 'openURL(%s,new-window)'\n\n"\
		"NAME Open in new tab\n"			"CMD !mozilla -remote 'openURL(%s,new-tab)'\n\n"\
		"NAME Run new Mozilla\n"			"CMD !mozilla %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Mozilla FireFox...\n\n"\
		"NAME Open in existing\n"			"CMD !firefox -a firefox -remote 'openURL(%s)'\n\n"\
		"NAME Open in new window\n"		"CMD !firefox -a firefox -remote 'openURL(%s,new-window)'\n\n"\
		"NAME Open in new tab\n"			"CMD !firefox -a firefox -remote 'openURL(%s,new-tab)'\n\n"\
		"NAME Run new Mozilla FireFox\n"	"CMD !firefox %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Galeon...\n\n"\
		"NAME Open in existing\n"			"CMD !galeon -x '%s'\n\n"\
		"NAME Open in new window\n"		"CMD !galeon -w '%s'\n\n"\
		"NAME Open in new tab\n"			"CMD !galeon -n '%s'\n\n"\
		"NAME Run new Galeon\n"				"CMD !galeon '%s'\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Opera...\n\n"\
		"NAME Open in existing\n"			"CMD !opera -remote 'openURL(%s)'\n\n"\
		"NAME Open in new window\n"		"CMD !opera -remote 'openURL(%s,new-window)'\n\n"\
		"NAME Open in new tab\n"			"CMD !opera -remote 'openURL(%s,new-page)'\n\n"\
		"NAME Run new Opera\n"				"CMD !opera %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME SUB\n"								"CMD Send URL to...\n\n"\
		"NAME Gnome URL Handler\n"			"CMD !gnome-open %s\n\n"\
		"NAME Lynx\n"							"CMD !"XTERM"lynx %s\n\n"\
		"NAME Links\n"							"CMD !"XTERM"links %s\n\n"\
		"NAME w3m\n"							"CMD !"XTERM"w3m %s\n\n"\
		"NAME lFTP\n" 							"CMD !"XTERM"lftp %s\n\n"\
		"NAME gFTP\n"							"CMD !gftp %s\n\n"\
		"NAME Konqueror\n"					"CMD !konqueror %s\n\n"\
		"NAME Telnet\n"						"CMD !"XTERM"telnet %s\n\n"\
		"NAME Ping\n"							"CMD !"XTERM"ping -c 4 %s\n\n"\
	"NAME ENDSUB\n"							"CMD \n\n"\
	"NAME Connect as IRC server\n"		"CMD url %s\n\n";
#endif
#endif

#ifdef USE_SIGACTION
/* Close and open log files on SIGUSR1. Usefull for log rotating */

static void 
sigusr1_handler (int signal, siginfo_t *si, void *un)
{
	GSList *list = sess_list;
	session *sess;

	if (prefs.logging)
	{
		while (list)
		{
			sess = list->data;
			log_open (sess);
			list = list->next;
		}
	}
}

/* Execute /SIGUSR2 when SIGUSR2 received */

static void
sigusr2_handler (int signal, siginfo_t *si, void *un)
{
	session *sess = current_sess;

	if (sess)
		handle_command (sess, "SIGUSR2", FALSE);
}
#endif

static gint
xchat_auto_connect (gpointer userdata)
{
	servlist_auto_connect (NULL);
	return 0;
}

static void
xchat_init (void)
{
	char buf[3068];
	const char *cs = NULL;

#ifdef WIN32
	WSADATA wsadata;

#ifdef USE_IPV6
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		MessageBox (NULL, "Cannot find winsock 2.2+", "Error", MB_OK);
		exit (0);
	}
#else
	WSAStartup(0x0101, &wsadata);
#endif	/* !USE_IPV6 */
#endif	/* !WIN32 */

#ifdef USE_SIGACTION
	struct sigaction act;

	/* ignore SIGPIPE's */
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);
	sigaction (SIGPIPE, &act, NULL);

	/* Deal with SIGUSR1's & SIGUSR2's */
	act.sa_sigaction = sigusr1_handler;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);
	sigaction (SIGUSR1, &act, NULL);

	act.sa_sigaction = sigusr2_handler;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);
	sigaction (SIGUSR2, &act, NULL);
#else
#ifndef WIN32
	/* good enough for these old systems */
	signal (SIGPIPE, SIG_IGN);
#endif
#endif

	if (g_get_charset (&cs))
		prefs.utf8_locale = TRUE;

	load_text_events ();
	sound_load ();
	notify_load ();
	ignore_load ();

	snprintf (buf, sizeof (buf),
		"NAME %s\n"						"CMD query %%s\n\n"\
		"NAME %s\n"						"CMD send %%s\n\n"\
		"NAME %s\n"						"CMD whois %%s %%s\n\n"\

		"NAME SUB\n"					"CMD %s\n\n"\
			"NAME %s\n"					"CMD op %%a\n\n"\
			"NAME %s\n"					"CMD deop %%a\n\n"\
			"NAME SEP\n"				"CMD \n\n"\
			"NAME %s\n"					"CMD voice %%a\n\n"\
			"NAME %s\n"					"CMD devoice %%a\n"\
			"NAME SEP\n"				"CMD \n\n"\
			"NAME SUB\n"				"CMD %s\n\n"\
				"NAME %s\n"				"CMD kick %%s\n\n"\
				"NAME %s\n"				"CMD ban %%s\n\n"\
				"NAME SEP\n"			"CMD \n\n"\
				"NAME %s *!*@*.host\n""CMD ban %%s 0\n\n"\
				"NAME %s *!*@domain\n""CMD ban %%s 1\n\n"\
				"NAME %s *!*user@*.host\n""CMD ban %%s 2\n\n"\
				"NAME %s *!*user@domain\n""CMD ban %%s 3\n\n"\
				"NAME SEP\n"			"CMD \n\n"\
				"NAME %s *!*@*.host\n""CMD kickban %%s 0\n\n"\
				"NAME %s *!*@domain\n""CMD kickban %%s 1\n\n"\
				"NAME %s *!*user@*.host\n""CMD kickban %%s 2\n\n"\
				"NAME %s *!*user@domain\n""CMD kickban %%s 3\n\n"\
			"NAME ENDSUB\n"			"CMD \n\n"\
		"NAME ENDSUB\n"				"CMD \n\n",

		_("Open Dialog Window"),
		_("Send a File"),
		_("User Info (WHOIS)"),
		_("Operator Actions"),

		_("Give Ops"),
		_("Take Ops"),
		_("Give Voice"),
		_("Take Voice"),

		_("Kick/Ban"),
		_("Kick"),
		_("Ban"),
		_("Ban"),
		_("Ban"),
		_("Ban"),
		_("Ban"),
		_("KickBan"),
		_("KickBan"),
		_("KickBan"),
		_("KickBan"));

#if 0
	snprintf (buf, sizeof (buf),
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD dcc send %%s\n\n"\
		"NAME %s\n"				"CMD dcc chat %%s\n\n"\
		"NAME %s\n"				"CMD dcc close chat %%s\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD CTCP\n\n"\
		"NAME %s\n"				"CMD ctcp %%s VERSION\n\n"\
		"NAME %s\n"				"CMD ctcp %%s USERINFO\n\n"\
		"NAME %s\n"				"CMD ctcp %%s CLIENTINFO\n\n"\
		"NAME %s\n"				"CMD ping %%s\n\n"\
		"NAME %s\n"				"CMD ctcp %%s TIME\n\n"\
		"NAME %s\n"				"CMD ctcp %%s FINGER\n\n"\
		"NAME XDCC List\n"	"CMD ctcp %%s XDCC LIST\n\n"\
		"NAME CDCC List\n"	"CMD ctcp %%s CDCC LIST\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD quote KILL %%s :die!\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD op %%a\n\n"\
		"NAME %s\n"				"CMD deop %%a\n\n"\
		"NAME SEP\n"			"CMD \n\n"\
		"NAME %s\n"				"CMD hop %%a\n\n"\
		"NAME %s\n"				"CMD dehop %%a\n\n"\
		"NAME SEP\n"			"CMD \n\n"\
		"NAME %s\n"				"CMD voice %%a\n\n"\
		"NAME %s\n"				"CMD devoice %%a\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD ignore %%s!*@* ALL\n\n"\
		"NAME %s\n"				"CMD unignore %%s!*@*\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD kick %%s\n\n"\
		"NAME %s\n"				"CMD ban %%s\n\n"\
		"NAME SEP\n"			"CMD \n\n"\
		"NAME %s *!*@*.host\n""CMD ban %%s 0\n\n"\
		"NAME %s *!*@domain\n""CMD ban %%s 1\n\n"\
		"NAME %s *!*user@*.host\n""CMD ban %%s 2\n\n"\
		"NAME %s *!*user@domain\n""CMD ban %%s 3\n\n"\
		"NAME SEP\n"			"CMD \n\n"\
		"NAME %s *!*@*.host\n""CMD kickban %%s 0\n\n"\
		"NAME %s *!*@domain\n""CMD kickban %%s 1\n\n"\
		"NAME %s *!*user@*.host\n""CMD kickban %%s 2\n\n"\
		"NAME %s *!*user@domain\n""CMD kickban %%s 3\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD quote WHO %%s\n\n"\
		"NAME %s\n"				"CMD quote WHOIS %%s %%s\n\n"\
		"NAME %s\n"				"CMD dns %%s\n\n"\
		"NAME %s\n"				"CMD quote TRACE %%s\n\n"\
		"NAME %s\n"				"CMD quote USERHOST %%s\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME SUB\n"				"CMD %s\n\n"\
		"NAME %s\n"				"CMD !"XTERM"/usr/sbin/traceroute %%h\n\n"\
		"NAME %s\n"				"CMD !"XTERM"ping -c 4 %%h\n\n"\
		"NAME %s\n"				"CMD !"XTERM"telnet %%h\n\n"\
	"NAME ENDSUB\n"			"CMD \n\n"\
	"NAME %s\n"					"CMD query %%s\n\n",
		_("Direct client-to-client"),
		_("Send File"),
		_("Offer Chat"),
		_("Abort Chat"),
		_("Version"),
		_("Userinfo"),
		_("Clientinfo"),
		_("Ping"),
		_("Time"),
		_("Finger"),
		_("Oper"),
		_("Kill this user"),
		_("Mode"),
		_("Give Ops"),
		_("Take Ops"),
		_("Give Half-Ops"),
		_("Take Half-Ops"),
		_("Give Voice"),
		_("Take Voice"),
		_("Ignore"),
		_("Ignore User"),
		_("UnIgnore User"),
		_("Kick/Ban"),
		_("Kick"),
		_("Ban"),
		_("Ban"),
		_("Ban"),
		_("Ban"),
		_("Ban"),
		_("KickBan"),
		_("KickBan"),
		_("KickBan"),
		_("KickBan"),
		_("Info"),
		_("Who"),
		_("WhoIs"),
		_("DNS Lookup"),
		_("Trace"),
		_("UserHost"),
		_("External"),
		_("Traceroute"),
		_("Ping"),
		_("Telnet"),
		_("Open Dialog Window")
		);
#endif
	list_loadconf ("popup.conf", &popup_list, buf);

	snprintf (buf, sizeof (buf),
		"NAME %s\n"				"CMD part\n\n"
		"NAME %s\n"				"CMD getstr # join \"%s\"\n\n"
		"NAME %s\n"				"CMD quote LINKS\n\n"
		"NAME %s\n"				"CMD ping\n\n"
		"NAME TOGGLE %s\n"	"CMD irc_hide_version\n\n",
				_("Leave Channel"),
				_("Join Channel..."),
				_("Enter Channel to Join:"),
				_("Server Links"),
				_("Ping Server"),
				_("Hide Version"));
	list_loadconf ("usermenu.conf", &usermenu_list, buf);

	snprintf (buf, sizeof (buf),
		"NAME %s\n"		"CMD op %%a\n\n"
		"NAME %s\n"		"CMD deop %%a\n\n"
		"NAME %s\n"		"CMD ban %%s\n\n"
		"NAME %s\n"		"CMD getstr %s \"kick %%s\" \"%s\"\n\n"
		"NAME %s\n"		"CMD send %%s\n\n"
		"NAME %s\n"		"CMD query %%s\n\n",
				_("Op"),
				_("DeOp"),
				_("Ban"),
				_("Kick"),
				_("bye"),
				_("Enter reason to kick %s:"),
				_("Sendfile"),
				_("Dialog"));
	list_loadconf ("buttons.conf", &button_list, buf);

	snprintf (buf, sizeof (buf),
		"NAME %s\n"				"CMD whois %%s %%s\n\n"
		"NAME %s\n"				"CMD send %%s\n\n"
		"NAME %s\n"				"CMD dcc chat %%s\n\n"
		"NAME %s\n"				"CMD clear\n\n"
		"NAME %s\n"				"CMD ping %%s\n\n",
				_("WhoIs"),
				_("Send"),
				_("Chat"),
				_("Clear"),
				_("Ping"));
	list_loadconf ("dlgbuttons.conf", &dlgbutton_list, buf);

	list_loadconf ("tabmenu.conf", &tabmenu_list, NULL);
	list_loadconf ("ctcpreply.conf", &ctcp_list, defaultconf_ctcp);
	list_loadconf ("commands.conf", &command_list, defaultconf_commands);
	list_loadconf ("replace.conf", &replace_list, defaultconf_replace);
	list_loadconf ("urlhandlers.conf", &urlhandler_list,
						defaultconf_urlhandlers);

	servlist_init ();							/* load server list */

	/* if we got a URL, don't open the server list GUI */
	if (!prefs.slist_skip && !arg_url)
		fe_serverlist_open (NULL);

	/* turned OFF via -a arg */
	if (!arg_dont_autoconnect)
	{
		/* do any auto connects */
		if (!servlist_have_auto ())	/* if no new windows open .. */
		{
			/* and no serverlist gui ... */
			if (prefs.slist_skip || arg_url)
				/* we'll have to open one. */
				new_ircwindow (NULL, NULL, SESS_SERVER, 0);
		} else
		{
			fe_idle_add (xchat_auto_connect, NULL);
		}
	} else
	{
		if (prefs.slist_skip || arg_url)
			new_ircwindow (NULL, NULL, SESS_SERVER, 0);
	}
}

void
xchat_exit (void)
{
	xchat_is_quitting = TRUE;
	in_xchat_exit = TRUE;
	plugin_kill_all ();
	fe_cleanup ();
	if (prefs.autosave)
	{
		save_config ();
		if (prefs.save_pevents)
			pevent_save (NULL);
	}
	if (prefs.autosave_url)
		url_autosave ();
	sound_save ();
	notify_save ();
	ignore_save ();
	free_sessions ();
	fe_exit ();
}

#ifndef WIN32

static int
child_handler (gpointer userdata)
{
	int pid = GPOINTER_TO_INT (userdata);

	if (waitpid (pid, 0, WNOHANG) == pid)
		return 0;					  /* remove timeout handler */
	return 1;						  /* keep the timeout handler */
}

#endif

void
xchat_exec (char *cmd)
{
#ifdef WIN32
	util_exec (cmd);
#else
	int pid = util_exec (cmd);
	if (pid != -1)
	/* zombie avoiding system. Don't ask! it has to be like this to work
      with zvt (which overrides the default handler) */
		fe_timeout_add (1000, child_handler, GINT_TO_POINTER (pid));
#endif
}

int
main (int argc, char *argv[])
{
	int ret;
	
	srand (time (0));	/* CL: do this only once! */

#ifdef SOCKS
	SOCKSinit (argv[0]);
#endif

	ret = fe_args (argc, argv);
	if (ret != -1)
		return ret;

	load_config ();

	fe_init ();

	xchat_init ();

	fe_main ();

#ifdef USE_OPENSSL
	if (ctx)
		_SSL_context_free (ctx);
#endif

#ifdef USE_DEBUG
	xchat_mem_list ();
#endif

#ifdef WIN32
	WSACleanup ();
#endif

	return 0;
}
