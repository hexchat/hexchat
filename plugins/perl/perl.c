/* X-Chat 2.0 PERL Plugin
 * Copyright (C) 1998-2002 Peter Zelezny.
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

/*
incompatibilities from 1.8.x:

- user_list and user_list_short:
  If a user has both op and voice, only the op flag will be 1.

- dcc_list
  The address field is always 0.

- add_user_list/sub_user_list/clear_user_list
  These functions do nothing.

- notify_list
  Not implemented. Always returns an empty list.

- server_list
  Lists servers that are not connected aswell.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef ENABLE_NLS
#include <locale.h>
#endif

#undef PACKAGE
#include "../../config.h"	/* for #define OLD_PERL */
#include "xchat-plugin.h"


struct perlscript
{
	char *name;
	char *version;
	char *desc;
	char *shutdowncallback;
	void *gui_entry;
};

static xchat_plugin *ph; /* plugin handle */


static int perl_load_file (char *script_name);



/* leave this before XSUB.h, to avoid readdir() being redefined */
static void
perl_auto_load (void)
{
	DIR *dir;
	struct dirent *ent;
	const char *xdir;

	xdir = xchat_get_info (ph, "xchatdir");
	dir = opendir (xdir);
	if (dir)
	{
		while ((ent = readdir (dir)))
		{
			int len = strlen (ent->d_name);
			if (len > 3 && strcasecmp (".pl", ent->d_name + len - 3) == 0)
			{
				char *file = malloc (len + strlen (xdir) + 2);
				sprintf (file, "%s/%s", xdir, ent->d_name);
				perl_load_file (file);
				free (file);
			}
		}
		closedir (dir);
	}
}

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include <glib.h>

static PerlInterpreter *my_perl = NULL;
static GSList *perl_list = 0;
static GSList *hook_list = 0;
#ifdef OLD_PERL
extern void boot_DynaLoader _((CV * cv));
#else
extern void boot_DynaLoader (pTHX_ CV* cv);
#endif

/* 
   execute_perl is modified in order to avoid crashing of xchat when a
   perl error occours. The embedded interpreter will instead print the
   error message using IRC::print and return 1 to stop futher
   processing of the event.

   patch by TheHobbit <thehobbit@altern.org>

*/

/*
  2001/06/14: execute_perl replaced by Martin Persson <mep@passagen.se>
	      previous use of perl_eval leaked memory, replaced with
	      a version that uses perl_call instead
*/
static int
execute_perl (char *function, char *args)
{
	char *perl_args[2] = { args, NULL };
	int count, ret_value = 1;
	SV *sv;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(sp);
	count = perl_call_argv(function, G_EVAL | G_SCALAR, perl_args);
	SPAGAIN;

	sv = GvSV(gv_fetchpv("@", TRUE, SVt_PV));
	if (SvTRUE(sv)) {
		xchat_printf(ph, "Perl error: %s\n", SvPV(sv, count));
		POPs;
	} else if (count != 1) {
		xchat_printf(ph, "Perl error: expected 1 value from %s, "
						 "got: %d\n", function, count);
	} else {
		ret_value = POPi;
	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return ret_value;
}

static int
perl_timer_cb (void *perl_callback)
{
	execute_perl (perl_callback, "");
	hook_list = g_slist_remove (hook_list, perl_callback);
	free (perl_callback);
	return 0;	/* returning zero removes the timeout handler */
}

static int
perl_server_cb (char *word[], char *word_eol[], void *perl_callback)
{
	return execute_perl (perl_callback, word_eol[1]);
}

static int
perl_command_cb (char *word[], char *word_eol[], void *perl_callback)
{
	return execute_perl (perl_callback, word_eol[2]);
}

static int
perl_print_cb (char *word[], void *perl_callback)
{
	char *arg;
	int ret;

	arg = g_strdup_printf ("%s %s %s %s", word[1], word[2], word[3], word[4]);
	ret = execute_perl (perl_callback, arg);
	g_free (arg);

	return ret;
}

/* custom IRC perl functions for scripting */

/* IRC::register (scriptname, version, shutdowncallback, unused)

 *  all scripts should call this at startup
 *
 */

static XS (XS_IRC_register)
{
	char *name, *ver, *callback, *desc;
	int junk;
	struct perlscript *scp;
	dXSARGS;

	name = SvPV (ST (0), junk);
	ver = SvPV (ST (1), junk);
	callback = SvPV (ST (2), junk);
	desc = SvPV (ST (3), junk);

	scp = malloc (sizeof (struct perlscript));
	scp->name = strdup (name);
	scp->version = strdup (ver);
	scp->desc = strdup (desc);
	scp->shutdowncallback = strdup (callback);
	/* FIXME: no filename */
	scp->gui_entry = xchat_plugingui_add (ph, scp->name, scp->name, scp->desc,
													  scp->version, NULL);

	perl_list = g_slist_prepend (perl_list, scp);

	XST_mPV (0, VERSION);
	XSRETURN (1);
}


/* print to main window */
/* IRC::main_print(output) */
static XS (XS_IRC_print)
{
	int junk;
	int i;
	char *output;
	dXSARGS;

	for (i = 0; i < items; ++i)
	{
		output = SvPV (ST (i), junk);
		xchat_print (ph, output);
	}

	XSRETURN_EMPTY;
}

/*
 * IRC::print_with_channel( text, channelname, servername )
 *    
 *   The servername is optional, channelname is required.
 *   Returns 1 on success, 0 on fail.
 */

static XS (XS_IRC_print_with_channel)
{
	int junk;
	dXSARGS;
	void *ctx, *old_ctx;

	old_ctx = xchat_get_context (ph);
	ctx = xchat_find_context (ph, SvPV (ST (1), junk), SvPV (ST (2), junk));
	if (ctx)
	{
		xchat_set_context (ph, ctx);
		xchat_print (ph, SvPV (ST (0), junk));
		xchat_set_context (ph, old_ctx);
		XSRETURN_YES;
	}

	XSRETURN_NO;
}

static XS (XS_IRC_get_info)
{
	dXSARGS;
	char *ret;
	const char *ids[] = {"version", "nick", "channel", "server", "xchatdir"};
	int i = SvIV (ST (0));

	if (i < 5 && i >= 0)
		XST_mPV (0, xchat_get_info (ph, ids[i]));
	else
	{
		switch (i)
		{
		case 5:
			if (xchat_get_info (ph, "away"))
				XST_mIV (0, 1);
			else
				XST_mIV (0, 0);
			break;

		default:
			XST_mPV (0, "Error2");
		}
	}

	XSRETURN (1);
}

/* Added by TheHobbit <thehobbit@altern.org>*/
/* IRC::get_prefs(var) */
static XS (XS_IRC_get_prefs)
{
	int junk;
	dXSARGS;
	const char *str;
	int integer;

	switch (xchat_get_prefs (ph, SvPV (ST (0), junk), &str, &integer))
	{
	case 0:
		XST_mPV (0, "Unknown variable");
		break;
	case 1:
		XST_mPV (0, str);
		break;
	case 2:
		XST_mIV (0, integer);
		break;
	case 3:
		if (integer)
			XST_mYES (0);
		else
			XST_mNO (0);
	}

	XSRETURN (1);
}

/* add handler for messages with message_type(ie PRIVMSG, 400, etc) */
/* IRC::add_message_handler(message_type, handler_name) */
static XS (XS_IRC_add_message_handler)
{
	int junk;
	dXSARGS;
	char *tmp;
	char *name;
	void *hook;

	tmp = strdup (SvPV (ST (1), junk));
	name = SvPV (ST (0), junk);
	if (strcasecmp (name, "inbound") == 0)	/* xchat 1.x.x compat */
		name = "RAW LINE";

	hook = xchat_hook_server (ph, name, PRI_NORM, perl_server_cb, tmp);

	hook_list = g_slist_prepend (hook_list, hook);

	XSRETURN_EMPTY;
}

/* add handler for commands with command_name */
/* IRC::add_command_handler(command_name, handler_name) */
static XS (XS_IRC_add_command_handler)
{
	int junk;
	dXSARGS;
	char *tmp;
	void *hook;

	tmp = strdup (SvPV (ST (1), junk));

	hook = xchat_hook_command (ph, SvPV (ST (0), junk), PRI_NORM,
										perl_command_cb, NULL, tmp);

	hook_list = g_slist_prepend (hook_list, hook);

	XSRETURN_EMPTY;
}

/* add handler for commands with print_name */
/* IRC::add_print_handler(print_name, handler_name) */
static XS (XS_IRC_add_print_handler)
{
	int junk;
	dXSARGS;
	char *tmp;
	void *hook;

	tmp = strdup (SvPV (ST (1), junk));

	hook = xchat_hook_print (ph, SvPV (ST (0), junk), PRI_NORM, perl_print_cb,
									 tmp);

	hook_list = g_slist_prepend (hook_list, hook);

	XSRETURN_EMPTY;
}

static XS (XS_IRC_add_timeout_handler)
{
	int junk;
	dXSARGS;
	void *hook;

	hook = xchat_hook_timer (ph, SvIV (ST (0)), perl_timer_cb,
									 strdup (SvPV (ST (1), junk)));
	hook_list = g_slist_prepend (hook_list, hook);

	XSRETURN_EMPTY;
}

/* send raw data to server */
/* IRC::send_raw(data) */
static XS (XS_IRC_send_raw)
{
	int junk;
	dXSARGS;

	xchat_commandf (ph, "quote %s", SvPV (ST (0), junk));

	XSRETURN_EMPTY;
}

static XS (XS_IRC_channel_list)
{
	int i = 0;
	dXSARGS;
	xchat_list *list;
	xchat_context *old = xchat_get_context (ph);

	list = xchat_list_get (ph, "channels");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
			XST_mPV (i, xchat_list_str (ph, list, "channel"));
			i++;
			XST_mPV (i, xchat_list_str (ph, list, "server"));
			i++;
			xchat_set_context (ph, (xchat_context *)
									 xchat_list_str (ph, list, "context"));
			XST_mPV (i, xchat_get_info (ph, "nick"));
			i++;
		}

		xchat_list_free (ph, list);
	}

	xchat_set_context (ph, old);

	XSRETURN (i);
}

static XS (XS_IRC_server_list)
{
	int i = 0;
	dXSARGS;
	xchat_list *list;

	list = xchat_list_get (ph, "channels");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
		/*	if (serv->connected && serv->end_of_motd)*/
			XST_mPV (i, xchat_list_str (ph, list, "server"));
			i++;
		}

		xchat_list_free (ph, list);
	}

	XSRETURN (i);
}

static XS (XS_IRC_ignore_list)
{
	int i = 0, junk, flags;
	dXSARGS;
	xchat_list *list;

	list = xchat_list_get (ph, "ignore");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
			/* Make sure there is room on the stack */
			EXTEND(SP, i + 10);

			XST_mPV (i, xchat_list_str (ph, list, "mask"));
			i++;

			flags = xchat_list_int (ph, list, "flags");

			XST_mIV (i, flags & 1);
			i++;
			XST_mIV (i, flags & 2);
			i++;
			XST_mIV (i, flags & 4);
			i++;
			XST_mIV (i, flags & 8);
			i++;
			XST_mIV (i, flags & 16);
			i++;
			XST_mIV (i, flags & 32);
			i++;
			XST_mPV (i, ":");
			i++;
		}

		xchat_list_free (ph, list);
	}

	XSRETURN (i);
}

static XS (XS_IRC_notify_list)
{
	int i = 0;
	dXSARGS;
#if 0
	struct notify *not;
	struct notify_per_server *notserv;
	GSList *list = notify_list;
	GSList *notslist;

	while (list)
	{
		not = (struct notify *) list->data;
		notslist = not->server_list;

		XST_mPV (i, not->name);
		i++;
		while (notslist)
		{
			notserv = (struct notify_per_server *)notslist->data;

			XST_mPV (i, notserv->server->servername);
			i++;
			XST_mIV (i, notserv->laston);
			i++;
			XST_mIV (i, notserv->lastseen);
			i++;
			XST_mIV (i, notserv->lastoff);
			i++;
			if (notserv->ison)
				XST_mYES (i);
			else
				XST_mNO (i);
			i++;
			XST_mPV (i, "::");
			i++;
			
			notslist = notslist->next;
		}
		XST_mPV (i, ":");
		i++;

		list = list->next;
	}
#endif

	XSRETURN (i);
}


/*

   IRC::user_info( nickname )

 */

static XS (XS_IRC_user_info)
{
	dXSARGS;
	xchat_list *list;
	int junk, i = 0;
	const char *nick;
	const char *find_nick;
	const char *host, *prefix;

	find_nick = SvPV (ST (0), junk);
	if (find_nick[0] == 0)
		find_nick = xchat_get_info (ph, "nick");

	list = xchat_list_get (ph, "users");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
			nick = xchat_list_str (ph, list, "nick");

			if (xchat_nickcmp (ph, (char *)nick, (char *)find_nick))
			{
				XST_mPV (i, nick);
				i++;
				host = xchat_list_str (ph, list, "host");
				if (!host)
					host = "FETCHING";
				XST_mPV (i, host);
				i++;

				prefix = xchat_list_str (ph, list, "prefix");

				XST_mIV (i, (prefix[0] == '@') ? 1 : 0);
				i++;
				XST_mIV (i, (prefix[0] == '+') ? 1 : 0);
				i++;
				xchat_list_free (ph, list);
				XSRETURN (4);
			}
		}

		xchat_list_free (ph, list);
	}

	XSRETURN (0);
}

/*
 * IRC::add_user_list(ul_channel, ul_server, nick, user_host,
 * 		      realname, server)
 */
static XS (XS_IRC_add_user_list)
{
	dXSARGS;

	XSRETURN_NO;
}

/*
 * IRC::sub_user_list(ul_channel, ul_server, nick)
 */
static XS (XS_IRC_sub_user_list)
{
	dXSARGS;

	XSRETURN_NO;
}

/*
 * IRC::clear_user_list(channel, server)
 */
static XS (XS_IRC_clear_user_list)
{
	dXSARGS;

	XSRETURN_NO;
};

/*

   IRC::user_list( channel, server )

 */

static XS (XS_IRC_user_list)
{
	const char *host;
	const char *prefix;
	int i = 0, junk;
	dXSARGS;
	xchat_list *list;
	xchat_context *ctx, *old = xchat_get_context (ph);

	ctx = xchat_find_context (ph, SvPV (ST (1), junk), SvPV (ST (0), junk));
	if (!ctx)
		XSRETURN (0);
	xchat_set_context (ph, ctx);

	list = xchat_list_get (ph, "users");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
			/* Make sure there is room on the stack */
			EXTEND(SP, i + 9);

			XST_mPV (i, xchat_list_str (ph, list, "nick"));
			i++;
			host = xchat_list_str (ph, list, "host");
			if (!host)
				host = "FETCHING";
			XST_mPV (i, host);
			i++;

			prefix = xchat_list_str (ph, list, "prefix");

			XST_mIV (i, (prefix[0] == '@') ? 1 : 0);
			i++;
			XST_mIV (i, (prefix[0] == '+') ? 1 : 0);
			i++;
			XST_mPV (i, ":");
			i++;
		}

		xchat_list_free (ph, list);
	}

	xchat_set_context (ph, old);

	XSRETURN (i);
}

static XS (XS_IRC_dcc_list)
{
	int i = 0;
	dXSARGS;
	xchat_list *list;
	const char *file;
	int type;

	list = xchat_list_get (ph, "dcc");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
			/* Make sure there is room on the stack */
			EXTEND (SP, i + 10);

			file = xchat_list_str (ph, list, "file");
			if (!file)
				file = "";

			XST_mPV (i, file);
			i++;
			XST_mIV (i, xchat_list_int (ph, list, "type"));
			i++;
			XST_mIV (i, xchat_list_int (ph, list, "status"));
			i++;
			XST_mIV (i, xchat_list_int (ph, list, "cps"));
			i++;
			XST_mIV (i, xchat_list_int (ph, list, "size"));
			i++;
			type = xchat_list_int (ph, list, "type");
			if (type == 0)
				XST_mIV (i, xchat_list_int (ph, list, "pos"));
			else
				XST_mIV (i, xchat_list_int (ph, list, "resume"));
			i++;
			XST_mIV (i, 0);	/* FIXME: addr */
			i++;

			file = xchat_list_str (ph, list, "destfile");
			if (!file)
				file = "";

			XST_mPV (i, file);
			i++;
		}

		xchat_list_free (ph, list);
	}

	XSRETURN (i);
}

/* run internal xchat command */
/* IRC::command(command) */
static XS (XS_IRC_command)
{
	int junk;
	dXSARGS;

	/* skip the forward slash */
	xchat_command (ph, SvPV (ST (0), junk) + 1);

	XSRETURN_EMPTY;
}

/* command_with_server(command, server) */
static XS (XS_IRC_command_with_server)
{
	int junk;
	dXSARGS;
	void *ctx, *old_ctx;

	old_ctx = xchat_get_context (ph);
	ctx = xchat_find_context (ph, SvPV (ST (1), junk), NULL);
	if (ctx)
	{
		xchat_set_context (ph, ctx);
		xchat_command (ph, SvPV (ST (0), junk) + 1);
		xchat_set_context (ph, old_ctx);
		XSRETURN_YES;
	}

	XSRETURN_NO;
}

/* MAG030600: BEGIN IRC::user_list_short */
/*

   IRC::user_list_short( channel, server )
   returns a shorter user list consisting of pairs of nick & user@host
   suitable for assigning to a hash.  modified IRC::user_list()
   
 */
static XS (XS_IRC_user_list_short)
{
	const char *host;
	const char *prefix;
	int i = 0, junk;
	dXSARGS;
	xchat_list *list;
	xchat_context *ctx, *old = xchat_get_context (ph);

	ctx = xchat_find_context (ph, SvPV (ST (1), junk), SvPV (ST (0), junk));
	if (!ctx)
		XSRETURN (0);
	xchat_set_context (ph, ctx);

	list = xchat_list_get (ph, "users");
	if (list)
	{
		while (xchat_list_next (ph, list))
		{
			/* Make sure there is room on the stack */
			EXTEND(SP, i + 5);

			XST_mPV (i, xchat_list_str (ph, list, "nick"));
			i++;
			host = xchat_list_str (ph, list, "host");
			if (!host)
				host = "FETCHING";
			XST_mPV (i, host);
			i++;
		}

		xchat_list_free (ph, list);
	}

	xchat_set_context (ph, old);

	XSRETURN (i);
}

/* MAG030600 BEGIN IRC::perl_script_list() */
/* return a list of currently loaded perl script names and versions */
static XS (XS_IRC_perl_script_list)
{
	int i = 0;
	GSList *handler;
	dXSARGS;

	handler = perl_list;
	while (handler)
	{
		struct perlscript *scp = handler->data;

		/* Make sure there is room on the stack */
		EXTEND(SP, i + 5);

		XST_mPV (i, scp->name);
		i++;
		XST_mPV (i, scp->version);
		i++;
		handler = handler->next;
	}
	XSRETURN(i);
}

/* xs_init is the second argument perl_pars. As the name hints, it
   initializes XS subroutines (see the perlembed manpage) */
static void
#ifdef OLD_PERL
xs_init ()
#else
xs_init (pTHX)
#endif
{
	char *file = __FILE__;

	/* This one allows dynamic loading of perl modules in perl
	   scripts by the 'use perlmod;' construction*/
	newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
	/* load up all the custom IRC perl functions */
	/* deplaced here from perl_init function (TheHobbit)*/
	newXS ("IRC::register", XS_IRC_register, "IRC");
	newXS ("IRC::add_message_handler", XS_IRC_add_message_handler, "IRC");
	newXS ("IRC::add_command_handler", XS_IRC_add_command_handler, "IRC");
	newXS ("IRC::add_print_handler", XS_IRC_add_print_handler, "IRC");
	newXS ("IRC::add_timeout_handler", XS_IRC_add_timeout_handler, "IRC");
	newXS ("IRC::print", XS_IRC_print, "IRC");
	newXS ("IRC::print_with_channel", XS_IRC_print_with_channel, "IRC");
	newXS ("IRC::send_raw", XS_IRC_send_raw, "IRC");
	newXS ("IRC::command", XS_IRC_command, "IRC");
	newXS ("IRC::command_with_server", XS_IRC_command_with_server, "IRC");
	newXS ("IRC::channel_list", XS_IRC_channel_list, "IRC");
	newXS ("IRC::server_list", XS_IRC_server_list, "IRC");
	newXS ("IRC::add_user_list", XS_IRC_add_user_list, "IRC");
	newXS ("IRC::sub_user_list", XS_IRC_sub_user_list, "IRC");
	newXS ("IRC::clear_user_list", XS_IRC_clear_user_list, "IRC");
	newXS ("IRC::user_list", XS_IRC_user_list, "IRC");
	newXS ("IRC::user_info", XS_IRC_user_info, "IRC");
	newXS ("IRC::ignore_list", XS_IRC_ignore_list, "IRC");
	newXS ("IRC::notify_list", XS_IRC_notify_list, "IRC");
	newXS ("IRC::dcc_list", XS_IRC_dcc_list, "IRC");
	newXS ("IRC::get_info", XS_IRC_get_info, "IRC");
	newXS ("IRC::get_prefs", XS_IRC_get_prefs, "IRC");
	newXS ("IRC::user_list_short", XS_IRC_user_list_short, "IRC");
	newXS ("IRC::perl_script_list", XS_IRC_perl_script_list, "IRC");
}

static void
perl_init (void)
{
	/*changed the name of the variable from load_file to
	  perl_definitions since now it does much more than defining
	  the load_file sub. Moreover, deplaced the initialisation to
	  the xs_init function. (TheHobbit)*/
	int warn;
	char *perl_args[] = { "", "-e", "0", "-w" };
	const char perl_definitions[] =
	{
	  /* We use to function one to load a file the other to
	     execute the string obtained from the first and holding
	     the file conents. This allows to have a realy local $/
	     without introducing temp variables to hold the old
	     value. Just a question of style:) 
	     We also redefine the $SIG{__WARN__} handler to have XChat
	     printing warnings in the main window. (TheHobbit)*/
	  "sub load_file{"
	    "my $f_name=shift;"
	    "local $/=undef;"
	    "open FH,$f_name or return \"__FAILED__\";"
	    "$_=<FH>;"
	    "close FH;"
	    "return $_;"
	  "}"
	  "sub load_n_eval{"
	    "my $f_name=shift;"
	    "my $strin=load_file($f_name);"
	    "return 2 if($strin eq \"__FAILED__\");"
	    "eval $strin;"
	    "if($@){"
	    /*"  #something went wrong\n"*/
	      "IRC::print\"Errors loading file $f_name:\\n\";"
	      "IRC::print\"$@\\n\";"
	      "return 1;"
	    "}"
	    "return 0;"
	  "}"
	  "$SIG{__WARN__}=sub{IRC::print\"$_[0]\n\";};"
	};
#ifdef ENABLE_NLS

	/* Problem is, dynamicaly loaded modules check out the $]
	   var. It appears that in the embedded interpreter we get
	   5,00503 as soon as the LC_NUMERIC locale calls for a comma
	   instead of a point in separating integer and decimal
	   parts. I realy can't understant why... The following
	   appears to be an awful workaround... But it'll do until I
	   (or someone else :)) found the "right way" to solve this
	   nasty problem. (TheHobbit <thehobbit@altern.org>)*/
	
 	setlocale(LC_NUMERIC,"C"); 
	
#endif

	my_perl = perl_alloc ();
	PL_perl_destruct_level = 1;
	perl_construct (my_perl);

	warn = 0;
	xchat_get_prefs (ph, "perl_warnings", NULL, &warn);
	if (warn)
		perl_parse (my_perl, xs_init, 4, perl_args, NULL);
	else
		perl_parse (my_perl, xs_init, 3, perl_args, NULL);
	/*
	  Now initialising the perl interpreter by loading the
	  perl_definition array.
	*/
#ifdef HAVE_EVAL_PV
	eval_pv (perl_definitions, TRUE);
#else
	perl_eval_pv (perl_definitions, TRUE);	/* deprecated */
#endif
}

/*
  To avoid problems, the load_n_eval sub must be executed directly
  without going into a supplementary eval.

  TheHobbit <thehobbit@altern.org>
*/
static int
perl_load_file (char *script_name)
{
	if (my_perl == NULL)
		perl_init ();

	return execute_perl ("load_n_eval", script_name);
}

/* checks for "~" in a file and expands */

static char *
expand_homedir (char *file)
{
#ifndef WIN32
	char *ret;

	if (*file == '~')
	{
		ret = malloc (strlen (file) + strlen (g_get_home_dir ()) + 1);
		sprintf (ret, "%s%s", g_get_home_dir (), file + 1);
		return ret;
	}
#endif
	return strdup (file);
}

static void
perl_end (void)
{
	struct perlscript *scp;
	char *tmp;

	while (perl_list)
	{
		scp = perl_list->data;
		perl_list = g_slist_remove (perl_list, scp);
		if (scp->shutdowncallback[0])
			execute_perl (scp->shutdowncallback, "");
		xchat_plugingui_remove (ph, scp->gui_entry);
		free (scp->name);
		free (scp->version);
		free (scp->shutdowncallback);
		free (scp);
	}

	if (my_perl != NULL)
	{
		perl_destruct (my_perl);
		perl_free (my_perl);
		my_perl = NULL;
	}

	while (hook_list)
	{
		tmp = xchat_unhook (ph, hook_list->data);
		if (tmp)
			free (tmp);
		hook_list = g_slist_remove (hook_list, hook_list->data);
	}
}

static int
perl_command_unloadall (char *word[], char *word_eol[], void *userdata)
{
	perl_end ();

	return EAT_XCHAT;
}

static int
perl_command_unload (char *word[], char *word_eol[], void *userdata)
{
#if 0
	int len;

	len = strlen (word[2]);
	if (len > 3 && strcasecmp (".pl", word[2] + len - 3) == 0)
	{

		/* if only unloading was possible with this shitty interface :( */

		return EAT_XCHAT;
	}
#endif

	return EAT_NONE;
}

static int
perl_command_load (char *word[], char *word_eol[], void *userdata)
{
	char *file;
	int len;

	len = strlen (word[2]);
	if (len > 3 && strcasecmp (".pl", word[2] + len - 3) == 0)
	{
		file = expand_homedir (word[2]);
		switch (perl_load_file (file))
		{
		case 1:
			xchat_print (ph, "Error compiling script\n");
			break;
		case 2:
			xchat_print (ph, "Error Loading file\n");
		}
		free (file);
		return EAT_XCHAT;
	}

	return EAT_NONE;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle,
				char **plugin_name, char **plugin_desc, char **plugin_version,
				char *arg)
{
	ph = plugin_handle;

	*plugin_name = "Perl";
	*plugin_version = VERSION;
	*plugin_desc = "Perl scripting interface";

	xchat_hook_command (ph, "load", PRI_NORM, perl_command_load, 0, 0);
	xchat_hook_command (ph, "unload", PRI_NORM, perl_command_unload, 0, 0);
	xchat_hook_command (ph, "unloadall", PRI_NORM, perl_command_unloadall, 0, 0);

	perl_auto_load ();

	return 1;
}

int
xchat_plugin_deinit (xchat_plugin *plugin_handle)
{
	perl_end ();

	return 1;
}
