/* X-Chat
 * Copyright (C) 2002 Peter Zelezny.
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "xchat.h"
#include "fe.h"
#include "util.h"
#include "outbound.h"
#include "cfgfiles.h"
#include "ignore.h"
#include "text.h"
#define PLUGIN_C
typedef struct session xchat_context;
#include "xchat-plugin.h"
#include "plugin.h"


#include "xchatc.h"

/* the USE_PLUGIN define only removes libdl stuff */

#ifdef USE_PLUGIN
#ifdef USE_GMODULE
#include <gmodule.h>
#else
#include <dlfcn.h>
#endif
#endif

#define DEBUG(t) PrintText(0,t)

/* crafted to be an even 32 bytes */
struct _xchat_hook
{
	xchat_plugin *pl;	/* the plugin to which it belongs */
	char *name;			/* "xdcc" */
	void *callback;	/* pointer to xdcc_callback */
	char *help_text;	/* help_text for commands only */
	void *userdata;	/* passed to the callback */
	int tag;				/* for timers & FDs only */
	int type;			/* HOOK_* */
	int pri;	/* fd */	/* priority / fd for HOOK_FD only */
};

struct _xchat_list
{
	int type;			/* LIST_* */
	GSList *pos;		/* current pos */
	GSList *next;		/* next pos */
};

typedef int (xchat_cmd_cb) (char *word[], char *word_eol[], void *user_data);
typedef int (xchat_serv_cb) (char *word[], char *word_eol[], void *user_data);
typedef int (xchat_print_cb) (char *word[], void *user_data);
typedef int (xchat_fd_cb) (int fd, int flags, void *user_data);
typedef int (xchat_timer_cb) (void *user_data);
typedef int (xchat_init_func) (xchat_plugin *, char **, char **, char **, char *);
typedef int (xchat_deinit_func) (xchat_plugin *);

enum
{
	LIST_CHANNELS,
	LIST_DCC,
	LIST_IGNORE,
	LIST_USERS
};

enum
{
	HOOK_COMMAND,	/* /command */
	HOOK_SERVER,	/* PRIVMSG, NOTICE, numerics */
	HOOK_PRINT,		/* All print events */
	HOOK_TIMER,		/* timeouts */
	HOOK_FD,			/* sockets & fds */
	HOOK_DELETED	/* marked for deletion */
};

GSList *plugin_list = NULL;	/* export for plugingui.c */
static GSList *hook_list = NULL;

extern const struct prefs vars[];	/* cfgfiles.c */


/* 31 bit string hash function */

static guint32
str_hash (const char *key)
{
	const char *p = key;
	guint32 h = *p;

	if (h)
		for (p += 1; *p != '\0'; p++)
			h = (h << 5) - h + *p;

	return h;
}

/* unload a plugin and remove it from our linked list */

static int
plugin_free (xchat_plugin *pl, int do_deinit, int allow_refuse)
{
	GSList *list, *next;
	xchat_hook *hook;
	xchat_deinit_func *deinit_func;

	/* fake plugin added by xchat_plugingui_add() */
	if (pl->fake)
		goto xit;

	/* run the plugin's deinit routine, if any */
	if (do_deinit && pl->deinit_callback != NULL)
	{
		deinit_func = pl->deinit_callback;
		if (!deinit_func (pl) && allow_refuse)
			return FALSE;
	}

	/* remove all of this plugin's hooks */
	list = hook_list;
	while (list)
	{
		hook = list->data;
		next = list->next;
		if (hook->pl == pl)
			xchat_unhook (NULL, hook);
		list = next;
	}

#ifdef USE_PLUGIN
	if (pl->handle)
#ifdef USE_GMODULE
		g_module_close (pl->handle);
#else
		dlclose (pl->handle);
#endif
#endif

xit:
	if (pl->filename)
		free (pl->filename);
	free (pl);

	plugin_list = g_slist_remove (plugin_list, pl);

#ifdef USE_PLUGIN
	fe_pluginlist_update ();
#endif

	return TRUE;
}

static xchat_plugin *
plugin_list_add (xchat_context *ctx, char *filename, char *name, char *desc,
					  char *version, void *handle, void *deinit_func, int fake)
{
	xchat_plugin *pl;

	pl = malloc (sizeof (xchat_plugin));
	pl->handle = handle;
	pl->filename = filename;
	pl->context = ctx;
	pl->name = name;
	pl->desc = desc;
	pl->version = version;
	pl->deinit_callback = deinit_func;
	pl->fake = fake;

	plugin_list = g_slist_prepend (plugin_list, pl);

	return pl;
}

static void
xchat_dummy (xchat_plugin *ph)
{
}

/* Load a static plugin */

void
plugin_add (session *sess, char *filename, void *handle, void *init_func,
				void *deinit_func, char *arg, int fake)
{
	xchat_plugin *pl;
	char *file;

	file = NULL;
	if (filename)
		file = strdup (filename);

	pl = plugin_list_add (sess, file, file, NULL, NULL, handle, deinit_func,
								 fake);

	if (!fake)
	{
		pl->xchat_hook_command = xchat_hook_command;
		pl->xchat_hook_server = xchat_hook_server;
		pl->xchat_hook_print = xchat_hook_print;
		pl->xchat_hook_timer = xchat_hook_timer;
		pl->xchat_hook_fd = xchat_hook_fd;
		pl->xchat_unhook = xchat_unhook;
		pl->xchat_print = xchat_print;
		pl->xchat_printf = xchat_printf;
		pl->xchat_command = xchat_command;
		pl->xchat_commandf = xchat_commandf;
		pl->xchat_nickcmp = xchat_nickcmp;
		pl->xchat_set_context = xchat_set_context;
		pl->xchat_find_context = xchat_find_context;
		pl->xchat_get_context = xchat_get_context;
		pl->xchat_get_info = xchat_get_info;
		pl->xchat_get_prefs = xchat_get_prefs;
		pl->xchat_list_get = xchat_list_get;
		pl->xchat_list_free = xchat_list_free;
		pl->xchat_list_fields = xchat_list_fields;
		pl->xchat_list_str = xchat_list_str;
		pl->xchat_list_next = xchat_list_next;
		pl->xchat_list_int = xchat_list_int;
		pl->xchat_plugingui_add = xchat_plugingui_add;
		pl->xchat_plugingui_remove = xchat_plugingui_remove;
		pl->xchat_emit_print = xchat_emit_print;
		/* incase new plugins are loaded on older xchat */
		pl->xchat_dummy8 = xchat_dummy;
		pl->xchat_dummy7 = xchat_dummy;
		pl->xchat_dummy6 = xchat_dummy;
		pl->xchat_dummy5 = xchat_dummy;
		pl->xchat_dummy4 = xchat_dummy;
		pl->xchat_dummy3 = xchat_dummy;
		pl->xchat_dummy2 = xchat_dummy;
		pl->xchat_dummy1 = xchat_dummy;

		/* run xchat_plugin_init, if it returns 0, close the plugin */
		if (((xchat_init_func *)init_func) (pl, &pl->name, &pl->desc, &pl->version, arg) == 0)
		{
			plugin_free (pl, FALSE, FALSE);
			return;
		}
	}

#ifdef USE_PLUGIN
	fe_pluginlist_update ();
#endif
}

/* kill any plugin by the given (file) name (used by /unload) */

int
plugin_kill (char *name, int by_filename)
{
	GSList *list;
	xchat_plugin *pl;

	list = plugin_list;
	while (list)
	{
		pl = list->data;
		/* static-plugins (plugin-timer.c) have a NULL filename */
		if ((by_filename && pl->filename && strcasecmp (name, pl->filename) == 0) ||
			 (by_filename && pl->filename && strcasecmp (name, file_part (pl->filename)) == 0) ||
			(!by_filename && strcasecmp (name, pl->name) == 0))
		{
			/* statically linked plugins have a NULL filename */
			if (pl->filename != NULL && !pl->fake)
			{
				if (plugin_free (pl, TRUE, TRUE))
					return 1;
				return 2;
			}
		}
		list = list->next;
	}

	return 0;
}

/* kill all running plugins (at shutdown) */

void
plugin_kill_all (void)
{
	GSList *list, *next;
	xchat_plugin *pl;

	list = plugin_list;
	while (list)
	{
		pl = list->data;
		next = list->next;
		if (!pl->fake)
			plugin_free (list->data, TRUE, FALSE);
		list = next;
	}
}

#ifdef USE_PLUGIN

/* load a plugin from a filename. Returns: NULL-success or an error string */

char *
plugin_load (session *sess, char *filename, char *arg)
{
	void *handle;
	xchat_init_func *init_func;
	xchat_deinit_func *deinit_func;

#ifdef USE_GMODULE
	/* load the plugin */
	handle = g_module_open (filename, 0);
	if (handle == NULL)
		return (char *)g_module_error ();

	/* find the init routine xchat_plugin_init */
	if (!g_module_symbol (handle, "xchat_plugin_init", (gpointer *)&init_func))
	{
		g_module_close (handle);
		return _("No xchat_plugin_init symbol; is this really an xchat plugin?");
	}

	/* find the plugin's deinit routine, if any */
	if (!g_module_symbol (handle, "xchat_plugin_deinit", (gpointer *)&deinit_func))
		deinit_func = NULL;

#else
	char *error;

/* OpenBSD lacks this! */
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

	/* load the plugin */
	handle = dlopen (filename, RTLD_GLOBAL | RTLD_NOW);
	if (handle == NULL)
		return dlerror ();

	/* find the init routine xchat_plugin_init */
	init_func = dlsym (handle, "xchat_plugin_init");
	error = dlerror ();
	if (error != NULL)
	{
		dlclose (handle);
		return _("No xchat_plugin_init symbol; is this really an xchat plugin?");
	}

	/* find the plugin's deinit routine, if any */
	deinit_func = dlsym (handle, "xchat_plugin_deinit");
#endif

	/* add it to our linked list */
	plugin_add (sess, filename, handle, init_func, deinit_func, arg, FALSE);

	return NULL;
}

static session *ps;

static void
plugin_auto_load_cb (char *filename)
{
	plugin_load (ps, filename, NULL);
}

void
plugin_auto_load (session *sess)
{
	ps = sess;
#ifdef WIN32
	for_files ("./plugins", "*.dll", plugin_auto_load_cb);
	for_files (get_xdir (), "*.dll", plugin_auto_load_cb);
#else
	for_files (XCHATLIBDIR"/plugins", "*.so", plugin_auto_load_cb);
	for_files (get_xdir (), "*.so", plugin_auto_load_cb);
#endif
}

#endif

static GSList *
plugin_hook_find (GSList *list, int type, char *name)
{
	xchat_hook *hook;

	while (list)
	{
		hook = list->data;
		if (hook->type == type)
		{
			if (strcasecmp (hook->name, name) == 0)
				return list;

			if (type == HOOK_SERVER)
			{
				if (strcasecmp (hook->name, "RAW LINE") == 0)
					return list;
			}
		}
		list = list->next;
	}

	return NULL;
}

/* check for plugin hooks and run them */

static int
plugin_hook_run (session *sess, char *name, char *word[], char *word_eol[], int type)
{
	GSList *list, *next;
	xchat_hook *hook;
	int ret, eat = 0;

	list = hook_list;
	while (1)
	{
		list = plugin_hook_find (list, type, name);
		if (!list)
			goto xit;

		hook = list->data;
		next = list->next;
		hook->pl->context = sess;

		/* run the plugin's callback function */
		switch (type)
		{
		case HOOK_COMMAND:
			ret = ((xchat_cmd_cb *)hook->callback) (word, word_eol, hook->userdata);
			break;
		case HOOK_SERVER:
			ret = ((xchat_serv_cb *)hook->callback) (word, word_eol, hook->userdata);
			break;
		default: /*case HOOK_PRINT:*/
			ret = ((xchat_print_cb *)hook->callback) (word, hook->userdata);
			break;
		}

		if ((ret & XCHAT_EAT_XCHAT) && (ret & XCHAT_EAT_PLUGIN))
		{
			eat = 1;
			goto xit;
		}
		if (ret & XCHAT_EAT_PLUGIN)
			goto xit;	/* stop running plugins */
		if (ret & XCHAT_EAT_XCHAT)
			eat = 1;	/* eventually we'll return 1, but continue running plugins */

		list = next;
	}

xit:
	/* really remove deleted hooks now */
	list = hook_list;
	while (list)
	{
		hook = list->data;
		next = list->next;
		if (hook->type == HOOK_DELETED)
		{
			hook_list = g_slist_remove (hook_list, hook);
			free (hook);
		}
		list = next;
	}

	return eat;
}

/* execute a plugged in command. Called from outbound.c */

int
plugin_emit_command (session *sess, char *name, char *word[], char *word_eol[])
{
	return plugin_hook_run (sess, name, word, word_eol, HOOK_COMMAND);
}

/* got a server PRIVMSG, NOTICE, numeric etc... */

int
plugin_emit_server (session *sess, char *name, char *word[], char *word_eol[])
{
	return plugin_hook_run (sess, name, word, word_eol, HOOK_SERVER);
}

/* see if any plugins are interested in this print event */

int
plugin_emit_print (session *sess, char *word[])
{
	return plugin_hook_run (sess, word[0], word, NULL, HOOK_PRINT);
}

int
plugin_emit_dummy_print (session *sess, char *name)
{
	char *word[32];
	int i;

	word[0] = name;
	for (i = 1; i < 32; i++)
		word[i] = "\000";

	return plugin_hook_run (sess, name, word, NULL, HOOK_PRINT);
}

static int
plugin_timeout_cb (xchat_hook *hook)
{
	int ret;

	/* timer_cb's context starts as front-most-tab */
	hook->pl->context = current_sess;

	/* call the plugin's timeout function */
	ret = ((xchat_timer_cb *)hook->callback) (hook->userdata);

	/* the callback might have already unhooked it! */
	if (!g_slist_find (hook_list, hook))
		return 0;

	if (ret == 0)
	{
		hook->tag = 0;	/* avoid fe_timeout_remove, returning 0 is enough! */
		xchat_unhook (hook->pl, hook);
	}

	return ret;
}

/* insert a hook into hook_list according to its priority */

static void
plugin_insert_hook (xchat_hook *new_hook)
{
	GSList *list;
	xchat_hook *hook;

	list = hook_list;
	while (list)
	{
		hook = list->data;
		if (hook->type == new_hook->type && hook->pri <= new_hook->pri)
		{
			hook_list = g_slist_insert_before (hook_list, list, new_hook);
			return;
		}
		list = list->next;
	}

	hook_list = g_slist_append (hook_list, new_hook);
}

static gboolean
plugin_fd_cb (GIOChannel *source, GIOCondition condition, xchat_hook *hook)
{
	int flags = 0;

	if (condition & G_IO_IN)
		flags |= XCHAT_FD_READ;
	if (condition & G_IO_OUT)
		flags |= XCHAT_FD_WRITE;
	if (condition & G_IO_PRI)
		flags |= XCHAT_FD_EXCEPTION;

	return ((xchat_fd_cb *)hook->callback) (hook->pri, flags, hook->userdata);
}

/* allocate and add a hook to our list. Used for all 4 types */

static xchat_hook *
plugin_add_hook (xchat_plugin *pl, int type, int pri, char *name,
					  char *help_text, void *callb, int timeout, void *userdata)
{
	xchat_hook *hook;

	hook = malloc (sizeof (xchat_hook));
	memset (hook, 0, sizeof (xchat_hook));

	hook->type = type;
	hook->pri = pri;
	if (name)
		hook->name = strdup (name);
	if (help_text)
		hook->help_text = strdup (help_text);
	hook->callback = callb;
	hook->pl = pl;
	hook->userdata = userdata;

	/* insert it into the linked list */
	plugin_insert_hook (hook);

	if (type == HOOK_TIMER)
		hook->tag = fe_timeout_add (timeout, plugin_timeout_cb, hook);

	return hook;
}

int
plugin_show_help (session *sess, char *cmd)
{
	GSList *list;
	xchat_hook *hook;

	/* show all help commands */
	if (cmd == NULL)
	{
		list = hook_list;
		while (list)
		{
			hook = list->data;
			if (hook->type == HOOK_COMMAND)
				PrintText (sess, hook->name);
			list = list->next;
		}
		return 1;
	}

	list = plugin_hook_find (hook_list, HOOK_COMMAND, cmd);
	if (list)
	{
		hook = list->data;
		if (hook->help_text)
		{
			PrintText (sess, hook->help_text);
			return 1;
		}
	}

	return 0;
}

/* ========================================================= */
/* ===== these are the functions plugins actually call ===== */
/* ========================================================= */

void *
xchat_unhook (xchat_plugin *ph, xchat_hook *hook)
{
	/* perl.c trips this */
	if (hook->type == HOOK_DELETED || !g_slist_find (hook_list, hook))
		return NULL;

	if (hook->type == HOOK_TIMER && hook->tag != 0)
		fe_timeout_remove (hook->tag);

	if (hook->type == HOOK_FD)
		fe_input_remove (hook->tag);

	hook->type = HOOK_DELETED;	/* expunge later */

	if (hook->name)
		free (hook->name);	/* NULL for timers & fds */
	if (hook->help_text)
		free (hook->help_text);	/* NULL for non-commands */

	return hook->userdata;
}

xchat_hook *
xchat_hook_command (xchat_plugin *ph, char *name, int pri, xchat_cmd_cb *callb,
					 	  char *help_text, void *userdata)
{
	return plugin_add_hook (ph, HOOK_COMMAND, pri, name, help_text, callb, 0,
									userdata);
}

xchat_hook *
xchat_hook_server (xchat_plugin *ph, char *name, int pri, xchat_serv_cb *callb,
					 	 void *userdata)
{
	return plugin_add_hook (ph, HOOK_SERVER, pri, name, 0, callb, 0, userdata);
}

xchat_hook *
xchat_hook_print (xchat_plugin *ph, char *name, int pri, xchat_print_cb *callb,
					   void *userdata)
{
	return plugin_add_hook (ph, HOOK_PRINT, pri, name, 0, callb, 0, userdata);
}

xchat_hook *
xchat_hook_timer (xchat_plugin *ph, int timeout, xchat_timer_cb *callb,
					   void *userdata)
{
	return plugin_add_hook (ph, HOOK_TIMER, 0, 0, 0, callb, timeout, userdata);
}

xchat_hook *
xchat_hook_fd (xchat_plugin *ph, int fd, int flags,
					xchat_fd_cb *callb, void *userdata)
{
	xchat_hook *hook;

	hook = plugin_add_hook (ph, HOOK_FD, 0, 0, 0, callb, 0, userdata);
	hook->pri = fd;
	/* plugin hook_fd flags correspond exactly to FIA_* flags (fe.h) */
	hook->tag = fe_input_add (fd, flags, plugin_fd_cb, hook);

	return hook;
}

void
xchat_print (xchat_plugin *ph, char *text)
{
	if (!is_session (ph->context))
	{
		DEBUG("xchat_print called without a valid context.\n");
		return;
	}

	PrintText (ph->context, text);
}

void
xchat_printf (xchat_plugin *ph, char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	xchat_print (ph, buf);
	g_free (buf);
}

void
xchat_command (xchat_plugin *ph, char *command)
{
	if (!is_session (ph->context))
	{
		DEBUG("xchat_command called without a valid context.\n");
		return;
	}

	handle_command (ph->context, command, FALSE);
}

void
xchat_commandf (xchat_plugin *ph, char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	xchat_command (ph, buf);
	g_free (buf);
}

int
xchat_nickcmp (xchat_plugin *ph, char *s1, char *s2)
{
	return ((session *)ph->context)->server->p_cmp (s1, s2);
}

xchat_context *
xchat_get_context (xchat_plugin *ph)
{
	return ph->context;
}

int
xchat_set_context (xchat_plugin *ph, xchat_context *context)
{
	if (is_session (context))
	{
		ph->context = context;
		return 1;
	}
	return 0;
}

xchat_context *
xchat_find_context (xchat_plugin *ph, char *servname, char *channel)
{
	GSList *slist, *clist;
	server *serv;
	session *sess;

	if (servname == NULL && channel == NULL)
		return current_sess;

	slist = serv_list;
	while (slist)
	{
		serv = slist->data;
		if (servname == NULL ||
			 rfc_casecmp (servname, serv->servername) == 0 ||
			 strcasecmp (servname, serv->hostname) == 0 ||
			 (serv->networkname && strcasecmp (servname, serv->networkname) == 0))
		{
			if (channel == NULL)
				return serv->front_session;

			clist = sess_list;
			while (clist)
			{
				sess = clist->data;
				if (sess->server == serv)
				{
					if (rfc_casecmp (channel, sess->channel) == 0)
						return sess;
				}
				clist = clist->next;
			}
		}
		slist = slist->next;
	}

	return NULL;
}

const char *
xchat_get_info (xchat_plugin *ph, const char *id)
{
	session *sess;

	sess = ph->context;
	if (!is_session (sess))
	{
		DEBUG("xchat_get_info called without a valid context.\n");
		return NULL;
	}

	switch (str_hash (id))
	{
	case 0x2de2ee: /* away */
		if (sess->server->is_away)
			return sess->server->last_away_reason;
		return NULL;

  	case 0x2c0b7d03: /* channel */
		return sess->channel;

	case 0x30f5a8: /* host */
		return sess->server->hostname;

	case 0x6de15a2e:	/* network */
		return sess->server->networkname;

	case 0x339763: /* nick */
		return sess->server->nick;

	case 0xca022f43: /* server */
		if (!sess->server->connected)
			return NULL;
		return sess->server->servername;

	case 0x696cd2f: /* topic */
		return sess->topic;

	case 0x14f51cd8: /* version */
		return VERSION;

	case 0xdd9b1abd:	/* xchatdir */
		return get_xdir ();
	}

	return NULL;
}

int
xchat_get_prefs (xchat_plugin *ph, const char *name, const char **string, int *integer)
{
	int i = 0;

	do
	{
		if (!strcasecmp (name, vars[i].name))
		{
			switch (vars[i].type)
			{
			case TYPE_STR:
				*string = ((char *) &prefs + vars[i].offset);
				return 1;

			case TYPE_INT:
				*integer = *((int *) &prefs + vars[i].offset);
				return 2;

			default:
			/*case TYPE_BOOL:*/
				if (*((int *) &prefs + vars[i].offset))
					*integer = 1;
				else
					*integer = 0;
				return 3;
			}
		}
		i++;
	}
	while (vars[i].name);

	return 0;
}

xchat_list *
xchat_list_get (xchat_plugin *ph, const char *name)
{
	xchat_list *list;

	list = malloc (sizeof (xchat_list));
	list->pos = NULL;

	switch (str_hash (name))
	{
	case 0x556423d0: /* channels */
		list->type = LIST_CHANNELS;
		list->next = sess_list;
		break;

	case 0x183c4:	/* dcc */
		list->type = LIST_DCC;
		list->next = dcc_list;
		break;

	case 0xb90bfdd2:	/* ignore */
		list->type = LIST_IGNORE;
		list->next = ignore_list;
		break;

	case 0x6a68e08: /* users */
		if (is_session (ph->context))
		{
			list->type = LIST_USERS;
			list->next = ph->context->userlist;
			break;
		}	/* fall through */

	default:
		free (list);
		return NULL;
	}

	return list;
}

void
xchat_list_free (xchat_plugin *ph, xchat_list *xlist)
{
	free (xlist);
}

int
xchat_list_next (xchat_plugin *ph, xchat_list *xlist)
{
	if (xlist->next == NULL)
		return 0;

	xlist->pos = xlist->next;
	xlist->next = xlist->pos->next;
	return 1;
}

const char **
xchat_list_fields (xchat_plugin *ph, const char *name)
{
	static const char *dcc_fields[] =
	{
		"iaddress32","icps",		"sdestfile","sfile",		"snick",	"iport",
		"ipos",		"iresume",	"isize",		"istatus", 	"itype",		NULL
	};
	static const char *channels_fields[] =
	{
		"schannel",	"pcontext", "snetwork", "sserver",	"stype",	NULL
	};
	static const char *ignore_fields[] =
	{
		"iflags", "smask", NULL
	};
	static const char *users_fields[] =
	{
		"shost", "snick", "sprefix", NULL
	};
	static const char *list_of_lists[] =
	{
		"channels",	"dcc", "ignore", "users", NULL
	};

	switch (str_hash (name))
	{
	case 0x556423d0:	/* channels */
		return channels_fields;
	case 0x183c4:		/* dcc */
		return dcc_fields;
	case 0xb90bfdd2:	/* ignore */
		return ignore_fields;
	case 0x6a68e08:	/* users */
		return users_fields;
	case 0x6236395:	/* lists */
		return list_of_lists;
	}

	return NULL;
}

const char *
xchat_list_str (xchat_plugin *ph, xchat_list *xlist, const char *name)
{
	guint32 hash = str_hash (name);
	gpointer data = xlist->pos->data;

	switch (xlist->type)
	{
	case LIST_CHANNELS:
		switch (hash)
		{
		case 0x2c0b7d03: /* channel */
			return ((session *)data)->channel;
		case 0x38b735af: /* context */
			return data;	/* this is a session * */
		case 0xca022f43: /* server */
			return ((session *)data)->server->servername;
		case 0x6de15a2e:        /* network */
			return ((session *)data)->server->networkname;
		}
		break;

	case LIST_DCC:
		switch (hash)
		{
		case 0x3d9ad31e:	/* destfile */
			return ((struct DCC *)data)->destfile;
		case 0x2ff57c:	/* file */
			return ((struct DCC *)data)->file;
		case 0x339763: /* nick */
			return ((struct DCC *)data)->nick;
		}
		break;

	case LIST_IGNORE:
		switch (hash)
		{
		case 0x3306ec:	/* mask */
			return ((struct ignore *)data)->mask;
		}
		break;

	case LIST_USERS:
		switch (hash)
		{
		case 0x339763: /* nick */
			return ((struct User *)data)->nick;
		case 0x30f5a8: /* host */
			return ((struct User *)data)->hostname;
		case 0xc594b292: /* prefix */
			return ((struct User *)data)->prefix;
		}
		break;
	}

	return NULL;
}

int
xchat_list_int (xchat_plugin *ph, xchat_list *xlist, const char *name)
{
	guint32 hash = str_hash (name);
	gpointer data = xlist->pos->data;

	switch (xlist->type)
	{
	case LIST_DCC:
		switch (hash)
		{
		case 0x34207553: /* address32 */
			return ((struct DCC *)data)->addr;
		case 0x181a6: /* cps */
			return ((struct DCC *)data)->cps;
		case 0x349881: /* port */
			return ((struct DCC *)data)->port;
		case 0x1b254: /* pos */
			return ((struct DCC *)data)->pos;
		case 0xc84dc82d: /* resume */
			return ((struct DCC *)data)->resumable;
		case 0x35e001: /* size */
			return ((struct DCC *)data)->size;
		case 0xcacdcff2: /* status */
			return ((struct DCC *)data)->dccstat;
		case 0x368f3a: /* type */
			return ((struct DCC *)data)->type;
		}
		break;

	case LIST_IGNORE:
		switch (hash)
		{
		case 0x5cfee87:	/* flags */
			return ((struct ignore *)data)->type;
		}
		break;

	case LIST_CHANNELS:
		switch (hash)
		{
		case 0x368f3a:	/* type */
			return ((struct session *)data)->type;
		}
		break;

	}

	return -1;
}

void *
xchat_plugingui_add (xchat_plugin *ph, char *filename, char *name, char *desc,
							char *version, char *reserved)
{
#ifdef USE_PLUGIN
	ph = plugin_list_add (NULL, strdup (filename), name, desc, version, NULL,
								 NULL, TRUE);
	fe_pluginlist_update ();
#endif

	return ph;
}

void
xchat_plugingui_remove (xchat_plugin *ph, void *handle)
{
#ifdef USE_PLUGIN
	plugin_free (handle, FALSE, FALSE);
#endif
}

int
xchat_emit_print (xchat_plugin *ph, char *event_name, ...)
{
	va_list args;
	char *argv[4];
	int i = 0;

	memset (&argv, 0, sizeof (argv));
	va_start (args, event_name);
	while (1)
	{
		argv[i] = va_arg (args, char *);
		if (!argv[i])
			break;
		i++;
		if (i >= 4)
			break;
	}

	i = text_emit_by_name (event_name, ph->context, argv[0], argv[1],
								  argv[2], argv[3]);
	va_end (args);

	return i;
}
