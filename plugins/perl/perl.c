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
#ifdef WIN32
#include <windows.h>
#endif

#undef PACKAGE
#include "../../config.h"	/* for #define OLD_PERL */
#include "xchat-plugin.h"

static xchat_plugin *ph; /* plugin handle */


static int perl_load_file (char *script_name);


#ifdef WIN32

static DWORD
child (char *str)
{
	MessageBoxA (0, str, "Perl DLL Error",
		     MB_OK|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
	return 0;
}

static void
thread_mbox (char *str)
{
	DWORD tid;

	CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) child,
					 str, 0, &tid));
}

#endif

/* leave this before XSUB.h, to avoid readdir() being redefined */
static void
perl_auto_load (void)
{
	DIR *dir;
	struct dirent *ent;
	const char *xdir;

	/* get the dir in local filesystem encoding (what opendir() expects!) */
	xdir = xchat_get_info (ph, "xchatdirfs");
	if (!xdir)	/* xchatdirfs is new for 2.0.9, will fail on older */
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
#define WIN32IOP_H
#include <perl.h>
#include <XSUB.h>
#include <glib.h>

typedef enum { PRINT_HOOK, SERVER_HOOK, COMMAND_HOOK, TIMER_HOOK } HookType;

typedef struct
{
  char *name;
  char *version;
  char *desc;
  SV *shutdowncallback;
  void *gui_entry;
} PerlScript;

typedef struct
{
  SV *name;
  SV *callback;
  SV *userdata;
  HookType type;
  xchat_hook *hook;
  
} HookData;

static PerlInterpreter *my_perl = NULL;
static GSList *perl_list = NULL;
static GSList *hook_list = NULL;

extern void boot_DynaLoader (pTHX_ CV* cv);


/*
   this is used for autoload and shutdown callbacks
*/
static int
execute_perl( SV *function, char *args)
{

	int count, ret_value = 1;
	SV *sv;

	dSP;
	ENTER;
	SAVETMPS;

	PUSHMARK (SP);
	XPUSHs ( sv_2mortal (newSVpvn (args, strlen (args))));
	PUTBACK;

	count = call_sv(function, G_EVAL | G_SCALAR);
	SPAGAIN;

	sv = GvSV(gv_fetchpv("@", TRUE, SVt_PV));
	if (SvTRUE(sv)) {
/* 		xchat_printf(ph, "Perl error: %s\n", SvPV(sv, count)); */
	  POPs; /* remove undef from the top of the stack */
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

/* static int */
/* fd_cb (int fd, int flags, void *userdata) */
/* { */
/* 	HookData *data = (HookData *)userdata; */
	
/* 	dSP; */
/* 	ENTER; */
/* 	SAVETMPS; */

/* 	PUSHMARK (SP); */
/* 	XPUSHs (data->userdata); */
/* 	PUTBACK; */

/* 	call_sv (data->callback, G_EVAL); */
/* 	SPAGAIN; */
/* 	if (SvTRUE (ERRSV)) */
/* 	{ */
/* 		xchat_printf (ph, "Error in fd callback %s", */
/* 				SvPV_nolen (ERRSV)); */
/* 		POPs; /\* remove undef from the top of the stack *\/ */
/* 	} */
	
/* 	PUTBACK; */
/* 	FREETMPS; */
/* 	LEAVE; */
	
/* 	return XCHAT_EAT_ALL; */
/* } */

static int
timer_cb (void *userdata)
{
	HookData *data = (HookData *)userdata;
	int retVal = 0;
	int count = 0;

	dSP;
	ENTER;
	SAVETMPS;

	PUSHMARK (SP);
	XPUSHs (data->userdata);
	PUTBACK;

	count =	call_sv (data->callback, G_EVAL);
	SPAGAIN;
	if (SvTRUE (ERRSV))
	{
		xchat_printf (ph, "Error in timer callback %s",
				SvPV_nolen (ERRSV));
		POPs; /* remove undef from the top of the stack */
		retVal = XCHAT_EAT_ALL;
	}
	else
	{
		if (count != 1)
		{
			xchat_print (ph, "Timer handler should only return 1 value." );
			retVal = XCHAT_EAT_NONE;
		}
		else
		{
			retVal = POPi;
			if (retVal == 0)
			{
				/* if 0 is return the timer is going to get
				   unhooked
				*/
				hook_list = g_slist_remove (hook_list,
								data->hook);
				SvREFCNT_dec (data->callback);

				if (data->userdata) {
				  SvREFCNT_dec (data->userdata);
				}
				free (data);
			}
		}

	}
	
	PUTBACK;
	FREETMPS;
	LEAVE;
	
	return retVal;
}

static int
server_cb (char *word[], char *word_eol[], void *userdata)
{
	HookData *data = (HookData *)userdata;
	int retVal = 0;
	int count = 0;

	/* these must be initialized after SAVETMPS */
	AV* wd = NULL;
	AV* wd_eol = NULL;

	dSP;
	ENTER;
	SAVETMPS;
	
	wd = newAV ();
	sv_2mortal ((SV*)wd);
	wd_eol = newAV ();
	sv_2mortal ((SV*)wd_eol);

	for (count = 1;
	(count < 32) && (word[count] != NULL) && (word[count][0] != 0);
	count++) {
	  av_push (wd, newSVpv (word[count], 0));
	}

	for (count = 1;
	(count < 32) && (word_eol[count] != NULL) && (word_eol[count][0] != 0);
	count++)
	{
		av_push (wd_eol, newSVpv (word_eol[count], 0));
	}
	
/* 	xchat_printf (ph, */
/* 			"Recieved %d words in server callback", av_len (wd)); */
	PUSHMARK (SP);
	XPUSHs (newRV_noinc ((SV*) wd));
	XPUSHs (newRV_noinc ((SV*) wd_eol));
	XPUSHs (data->userdata);
	PUTBACK;

	count =	call_sv (data->callback, G_EVAL);
	SPAGAIN;
	if (SvTRUE (ERRSV))
	{
		xchat_printf (ph, "Error in server callback %s",
				SvPV_nolen (ERRSV));
		POPs; /* remove undef from the top of the stack */
		retVal = XCHAT_EAT_NONE;
	}
	else
	{
		if (count != 1)
		{
			xchat_print (ph, "Server handler should only return 1 value.");
			retVal = XCHAT_EAT_NONE;
		}
		else
		{
			retVal = POPi;
		}

	}
	
	PUTBACK;
	FREETMPS;
	LEAVE;
	
	return retVal;
}

static int
command_cb (char* word[], char* word_eol[], void *userdata)
{
	HookData *data = (HookData *)userdata;
	int retVal = 0;
	int count = 0;

	/* these must be initialized after SAVETMPS */
	AV* wd = NULL;
	AV* wd_eol = NULL;

	dSP;
	ENTER;
	SAVETMPS;
	
	wd = newAV ();
	sv_2mortal ((SV*)wd);
	wd_eol = newAV ();
	sv_2mortal ((SV*)wd_eol);

	for (count = 1;
		  (count < 32) && (word[count] != NULL) && (word[count][0] != 0);
		  count++) {
	  av_push (wd, newSVpv (word[count], 0));
	}

	for (count = 1;
	(count < 32) && (word_eol[count] != NULL) && 
		(word_eol[count][0] != 0);
	count++)
	{
		av_push (wd_eol, newSVpv (word_eol[count], 0));
	}
	
/* 	xchat_printf (ph, "Recieved %d words in command callback", */
/* 			av_len (wd)); */
	PUSHMARK (SP);
	XPUSHs (newRV_noinc ((SV*)wd));
	XPUSHs (newRV_noinc ((SV*)wd_eol));
	XPUSHs (data->userdata);
	PUTBACK;

	count =	call_sv (data->callback, G_EVAL);
	SPAGAIN;
	if (SvTRUE (ERRSV))
	{
		xchat_printf (ph, "Error in command callback %s",
				SvPV_nolen (ERRSV));
		POPs; /* remove undef from the top of the stack */
		retVal = XCHAT_EAT_NONE;
	}
	else
	{
		if (count != 1)
		{
			xchat_print (ph, "Command handler should only return 1 value.");
			retVal = XCHAT_EAT_NONE;
		}
		else
		{
			retVal = POPi;
		}

	}
	
	PUTBACK;
	FREETMPS;
	LEAVE;
	
	return retVal;
}

static int
print_cb (char *word[], void *userdata)
{

	HookData *data = (HookData *)userdata;
	int retVal = 0;
	int count = 0;

	/* must be initialized after SAVETMPS */
	AV* wd = NULL;

	dSP;
	ENTER;
	SAVETMPS;

	wd = newAV ();
	sv_2mortal ((SV*)wd);

	for (count = 1;
	(count < 32) && (word[count] != NULL) && (word[count][0] != 0);
	count++)
	{
		av_push (wd, newSVpv (word[count], 0));
	}
	
/* 	xchat_printf (ph, "Recieved %d words in print callback", av_len (wd)); */
	PUSHMARK (SP);
	XPUSHs (newRV_noinc ((SV*)wd));
	XPUSHs (data->userdata);
	PUTBACK;

	count =	call_sv (data->callback, G_EVAL);
	SPAGAIN;
	if (SvTRUE (ERRSV))
	{
		xchat_printf (ph, "Error in print callback %s",
			       SvPV_nolen(ERRSV));
		POPs; /* remove undef from the top of the stack */
		retVal = XCHAT_EAT_NONE;
	}
	else
	{
		if (count != 1)
		{
			xchat_print (ph, "Print handler should only return 1 value.");
			retVal = XCHAT_EAT_NONE;
		}
		else
		{
			retVal = POPi;
		}

	}
	
	PUTBACK;
	FREETMPS;
	LEAVE;
	
	return retVal;
}

/* custom IRC perl functions for scripting */

/* Xchat::_register (scriptname, version, desc, shutdowncallback, filename)
 *
 */

static XS (XS_Xchat_register)
{
	char *name, *ver, *desc, *filename;
	SV *callback;
	PerlScript *scp;
	dXSARGS;
	if (items != 5) {
		xchat_printf (ph,
		"Usage: Xchat::_register(scriptname, version, desc,shutdowncallback, filename)");
	} else {
		name = SvPV_nolen (ST (0));
		ver = SvPV_nolen (ST (1));
		desc = SvPV_nolen (ST (2));
		callback = ST (3);
		filename = SvPV_nolen(ST (4));

		scp = malloc (sizeof (PerlScript));
		scp->name = strdup (name);
		scp->version = strdup (ver);
		scp->desc = strdup (desc);
	  
		scp->shutdowncallback = sv_mortalcopy (callback);
		SvREFCNT_inc (scp->shutdowncallback);

		scp->gui_entry = xchat_plugingui_add (ph, filename, scp->name,
						      scp->desc, scp->version, NULL);
	  
		perl_list = g_slist_prepend (perl_list, scp);
	  
		XSRETURN_EMPTY;
	}
}


/* Xchat::print(output) */
static XS (XS_Xchat_print)
{
  
	char *text = NULL;

	dXSARGS;
	if (items != 1 ) {
		xchat_print (ph, "Usage: Xchat::_print(text)");
	} else {
		text = SvPV_nolen (ST (0));
		xchat_print (ph, text);
	}
	XSRETURN_EMPTY;
}

static XS (XS_Xchat_emit_print)
{
	char *event_name;
	int RETVAL;
	int count;

	dXSARGS;
	if (items < 1) {
		xchat_print (ph, "Usage: Xchat::emit_print(event_name, ...)");
	} else {
		event_name = (char *)SvPV_nolen (ST (0));
		RETVAL = 0;
		
		/* we need to figure out the number of defined values passed in */
		for( count = 0; count < items; count++ ) {
		  if (!SvOK (ST (count) )) {
			 break;
		  }
		}

		/*		switch  (items) {*/
		switch (count) {
		case 1:
			RETVAL = xchat_emit_print (ph, event_name, NULL);
			break;
		case 2:
			RETVAL = xchat_emit_print (ph, event_name,
							SvPV_nolen (ST (1)),
							NULL);
			break;
		case 3:
			RETVAL = xchat_emit_print (ph, event_name,
							SvPV_nolen (ST (1)), 
							SvPV_nolen (ST (2)),
							NULL);
			break;
		case 4:
	       		RETVAL = xchat_emit_print (ph, event_name,
						  SvPV_nolen (ST (1)), 
						  SvPV_nolen (ST (2)),
						  SvPV_nolen (ST (3)),
						  NULL);
		       break;
		case 5:
			RETVAL = xchat_emit_print (ph, event_name,
						  SvPV_nolen (ST (1)), 
						  SvPV_nolen (ST (2)),
						  SvPV_nolen (ST (3)),
						  SvPV_nolen (ST (4)),
						  NULL);
			break;

		}

		XSRETURN_IV(RETVAL);
    }
}
static XS (XS_Xchat_get_info)
{
	dXSARGS;
	if (items != 1) {
	  xchat_print (ph, "Usage: Xchat::get_info(id)");
	} else {
		SV *	id = ST (0);
		const char *	RETVAL;
	  
		RETVAL = xchat_get_info (ph, SvPV_nolen (id));
		if (RETVAL == NULL) {
		      XSRETURN_UNDEF;
		}
		XSRETURN_PV (RETVAL);
	}
}

static XS (XS_Xchat_get_prefs)
{
	const char *str;
	int integer;
	dXSARGS;
	if(items != 1) {
		xchat_print (ph, "Usage: Xchat::get_prefs(name)");
	} else {
	  
	  
		switch (xchat_get_prefs
				(ph, SvPV_nolen (ST (0)), &str, &integer)) {
		case 0:
			XSRETURN_UNDEF;
			break;
		case 1:
			XSRETURN_PV(str);
			break;
		case 2:
			XSRETURN_IV(integer);
			break;
		case 3:
			if (integer) {
				XSRETURN_YES;
			} else {
				XSRETURN_NO;
			}
		}
	}
}

/* Xchat::_hook_server(name, priority, callback, [userdata]) */
static XS (XS_Xchat_hook_server)
{

	SV *	name;
	int	pri;
	SV *	callback;
	SV *	userdata;
	xchat_hook *	RETVAL;
	HookData *data;

	dXSARGS;

	if (items != 4) {
		xchat_print (ph,
				"Usage: Xchat::_hook_server(name, priority, callback, userdata)");
	} else {
		name = ST (0);
		pri = (int)SvIV(ST (1));
		callback = ST (2);
		userdata = ST (3);
		data = NULL;
		data = malloc (sizeof (HookData));
		if (data == NULL) {
			XSRETURN_UNDEF;
		}
		
		data->name = sv_mortalcopy (name);
		SvREFCNT_inc (data->name);
		data->callback = sv_mortalcopy (callback);
		SvREFCNT_inc (data->callback);
		data->userdata = sv_mortalcopy (userdata);
		SvREFCNT_inc (data->userdata);
		RETVAL = xchat_hook_server (ph, SvPV_nolen (name), pri,
						server_cb, data);
		hook_list = g_slist_append (hook_list, RETVAL);

		XSRETURN_IV(PTR2UV(RETVAL));
	}
}

/* Xchat::_hook_command(name, priority, callback, help_text, userdata) */
static XS(XS_Xchat_hook_command)
{
	SV *name;
	int pri;
	SV *callback;
	char *help_text;
	SV *userdata;
	xchat_hook *RETVAL;
	HookData *data;
	
	dXSARGS;
	
	if (items != 5) {
		xchat_print (ph, "Usage: Xchat::_hook_command(name, priority, callback, help_text, userdata)");
	} else {
		name = ST (0);
		pri = (int)SvIV(ST (1));
		callback = ST (2);
		help_text = SvPV_nolen(ST (3));
		userdata = ST(4);
		data = NULL;

		data = malloc (sizeof (HookData));
		if (data == NULL) {
			XSRETURN_UNDEF;
		}

		data->name = sv_mortalcopy (name);
		SvREFCNT_inc (data->name);
		data->callback = sv_mortalcopy (callback);
		SvREFCNT_inc (data->callback);
		data->userdata = sv_mortalcopy (userdata);
		SvREFCNT_inc (data->userdata);
		RETVAL = xchat_hook_command (ph, SvPV_nolen (name), pri,
						command_cb, help_text, data);
		hook_list = g_slist_append (hook_list, RETVAL);

		XSRETURN_IV (PTR2UV(RETVAL));
	}

}

/* Xchat::_hook_print(name, priority, callback, [userdata]) */
static XS (XS_Xchat_hook_print)
{

	SV *	name;
	int	pri;
	SV *	callback;
	SV *	userdata;
	xchat_hook *	RETVAL;
	HookData *data;
	dXSARGS;
	if (items != 4) {
		xchat_print (ph,
				"Usage: Xchat::_hook_print(name, priority, callback, userdata)");
	} else {
		name = ST (0);
		pri = (int)SvIV(ST (1));
		callback = ST (2);
		data = NULL;
		userdata = ST (3);

		data = malloc (sizeof (HookData));
		if (data == NULL)
		{
			XSRETURN_UNDEF;
		}

		data->name = sv_mortalcopy (name);
		SvREFCNT_inc (data->name);
		data->callback = sv_mortalcopy (callback);
		SvREFCNT_inc (data->callback);
		data->userdata = sv_mortalcopy (userdata);
		SvREFCNT_inc (data->userdata);
		RETVAL = xchat_hook_print (ph, SvPV_nolen (name), pri,
						print_cb, data);
		hook_list = g_slist_append (hook_list, RETVAL);

		XSRETURN_IV(PTR2UV(RETVAL));
	}
}

/* Xchat::_hook_timer(timeout, callback, userdata) */
static XS (XS_Xchat_hook_timer)
{
	int	timeout;
	SV *	callback;
	SV *	userdata;
	xchat_hook * RETVAL;
	HookData *data;

	dXSARGS;

	if (items != 3) {
		xchat_print (ph,
		"Usage: Xchat::_hook_timer(timeout, callback, userdata)");
	} else {
		timeout = (int)SvIV(ST (0));
		callback = ST (1);
		data = NULL;
		userdata = ST (2);
		data = malloc (sizeof (HookData));

		if (data == NULL) {
			XSRETURN_UNDEF;
		}

		data->name = NULL;
		data->callback = sv_mortalcopy (callback);
		SvREFCNT_inc (data->callback);
		data->userdata = sv_mortalcopy (userdata);
		SvREFCNT_inc (data->userdata);
		RETVAL = xchat_hook_timer (ph, timeout, timer_cb, data);
		data->hook = RETVAL;
		hook_list = g_slist_append (hook_list, RETVAL);

		XSRETURN_IV (PTR2UV(RETVAL));
	}
}

/* Xchat::_hook_fd(fd, callback, flags, userdata) */
/* static XS (XS_Xchat_hook_fd) */
/* { */
/* 	int	fd; */
/* 	SV *	callback; */
/* 	int	flags; */
/* 	SV *	userdata; */
/* 	xchat_hook * RETVAL; */
/* 	HookData *data; */

/* 	dXSARGS; */

/* 	if (items != 4) { */
/* 		xchat_print (ph, */
/* 		"Usage: Xchat::_hook_fd(fd, callback, flags, userdata)"); */
/* 	} else { */
/* 		fd = (int)SvIV (ST (0)); */
/* 		callback = ST (1); */
/* 		flags = (int)SvIV (ST (2)); */
/* 		userdata = ST (3); */
/* 		data = NULL; */
/* 		data = malloc (sizeof (HookData)); */

/* 		if (data == NULL) { */
/* 			XSRETURN_UNDEF; */
/* 		} */

/* 		data->name = NULL; */
/* 		data->callback = sv_mortalcopy (callback); */
/* 		SvREFCNT_inc (data->callback); */
/* 		data->userdata = sv_mortalcopy (userdata); */
/* 		SvREFCNT_inc (data->userdata); */
/* 		RETVAL = xchat_hook_fd (ph, fd, flags, fd_cb, data); */
/* 		data->hook = RETVAL; */
/* 		hook_list = g_slist_append (hook_list, RETVAL); */

/* 		XSRETURN_IV (PTR2UV(RETVAL)); */
/* 	} */
/* } */

static XS(XS_Xchat_unhook)
{
	xchat_hook *	hook;
	HookData *userdata;
	int retCount = 0;
	dXSARGS;
	if (items != 1) {
		xchat_print (ph, "Usage: Xchat::unhook(hook)");
	} else {
	  if(looks_like_number(ST (0))) {
		 hook = INT2PTR(xchat_hook *,SvIV(ST (0)));
		 
		 if(g_slist_find( hook_list, hook ) != NULL ) {
			userdata = (HookData*)xchat_unhook (ph, hook);
			hook_list = g_slist_remove (hook_list, hook);
			if (userdata->name) {
			  SvREFCNT_dec (userdata->name);
			}
			if (userdata->callback) {
			  SvREFCNT_dec (userdata->callback);
			}
			if (userdata->userdata) {
			  XPUSHs(sv_mortalcopy (userdata->userdata));
			  SvREFCNT_dec (userdata->userdata);
			  retCount = 1;
			}
			free (userdata);
			XSRETURN(retCount);
		 }
	  }
	  XSRETURN_EMPTY;
	}
}

/* Xchat::_command(command) */
static XS (XS_Xchat_command)
{
	char *cmd = NULL;

	dXSARGS;
	if (items != 1 ) {
		xchat_print (ph, "Usage: Xchat::_command(command)");
	} else {
		cmd = SvPV_nolen (ST (0));
		xchat_command (ph, cmd);

	}
	XSRETURN_EMPTY;
}

static XS (XS_Xchat_find_context)
{
	char * server = NULL;
	char * chan = NULL;
	xchat_context * RETVAL;

	dXSARGS;
	if (items > 2)
		xchat_print (ph, "Usage: Xchat::find_context ([channel, [server]])");
	{

	switch (items)
	{
	case 0: /* no server name and no channel name */
		/* nothing to do, server and chan are already NULL */
		break;
	case 1: /* channel name only */
		/* change channel value only if it is true or 0*/
		/* otherwise leave it as null */
		if (SvTRUE (ST (0)) || SvNIOK (ST (0))) {
			chan = SvPV_nolen (ST (0));
/* 			xchat_printf( ph, "XSUB - find_context( %s, NULL )", chan ); */
		} /* else { xchat_print( ph, "XSUB - find_context( NULL, NULL )" ); } */
		/* chan is already NULL */
		break;
	case 2: /* server and channel */
		/* change channel value only if it is true or 0 */
		/* otherwise leave it as NULL */
		if (SvTRUE (ST (0)) || SvNIOK (ST (0)) ) {
			chan = SvPV_nolen (ST (0));
/* 			xchat_printf( ph, "XSUB - find_context( %s, NULL )", SvPV_nolen(ST(0) )); */
		} /* else { xchat_print( ph, "XSUB - 2 arg NULL chan" ); } */
		/* change server value only if it is true or 0 */
		/* otherwise leave it as NULL */
		if (SvTRUE (ST (1)) || SvNIOK (ST (1)) ){
			server = SvPV_nolen (ST (1));
/* 			xchat_printf( ph, "XSUB - find_context( NULL, %s )", SvPV_nolen(ST(1) )); */
		}/*  else { xchat_print( ph, "XSUB - 2 arg NULL server" ); } */

		break;
	}

	RETVAL = xchat_find_context (ph, server, chan);
	if (RETVAL != NULL)
	{
/*  		xchat_print (ph, "XSUB - context found"); */
		XSRETURN_IV(PTR2UV(RETVAL));
	}
	else
	{
 /* 		xchat_print (ph, "XSUB - context not found"); */
		XSRETURN_UNDEF;
	}
	}
}

static XS (XS_Xchat_get_context)
{
	dXSARGS;
	if (items != 0) {
		xchat_print (ph, "Usage: Xchat::get_context()");
	} else {
		XSRETURN_IV(PTR2UV(xchat_get_context (ph)));
	}
}

static XS (XS_Xchat_set_context)
{
	xchat_context *ctx;
	dXSARGS;
	if (items != 1) {
		xchat_print (ph, "Usage: Xchat::set_context(ctx)");
	} else {
		ctx = INT2PTR(xchat_context *,SvIV(ST (0)));
		XSRETURN_IV((IV)xchat_set_context (ph, ctx));
	}
}

static XS (XS_Xchat_nickcmp)
{
	dXSARGS;
	if (items != 2) {
		xchat_print (ph, "Usage: Xchat::nickcmp(s1, s2)");
	} else {
		XSRETURN_IV((IV)xchat_nickcmp (ph, SvPV_nolen (ST (0)),
					 SvPV_nolen (ST (1))));
	}
}

static XS (XS_Xchat_get_list)
{
	SV *	name;
	HV * hash;
	xchat_list *list;
	const char ** fields;
	const char *field;
	int i = 0; /* field index */
	int count = 0; /* return value for scalar context */
	U32 context;
	dXSARGS;

	if (items != 1) {
		xchat_print (ph, "Usage: Xchat::get_list(name)");
	} else {
		SP -= items;  /*remove the argument list from the stack*/

		name = ST (0);
	
		list = xchat_list_get (ph, SvPV_nolen (name));
	  
		if (list == NULL) {
			XSRETURN_EMPTY;
		}
		
		context = GIMME_V;

		if( context == G_SCALAR ) {
		  while (xchat_list_next (ph, list)) {
			 count++;
		  }
		  xchat_list_free (ph, list);
		  XSRETURN_IV((IV)count);
		}
	  
		fields = xchat_list_fields (ph, SvPV_nolen (name));
		while (xchat_list_next (ph, list)) {
			i = 0;
			hash = newHV ();
			sv_2mortal ((SV*) hash);
			while (fields[i] != NULL) {
				switch (fields[i][0]) {
				case 's':
					field = xchat_list_str (ph, list, fields[i]+1);
					if (field != NULL)
					{
						hv_store (hash, fields[i]+1, strlen (fields[i]+1),
										newSVpvn (field, strlen (field)), 0);
/* 					xchat_printf (ph, */
/* 						"string: %s - %d - %s",  */
/* 						fields[i]+1, */
/* 						strlen(fields[i]+1), */
/* 						field, strlen(field) */
/* 							); */
					} else {
						hv_store (hash, fields[i]+1, strlen (fields[i]+1),
									&PL_sv_undef, 0);
/* 					xchat_printf (ph, */
/* 						"string: %s - %d - undef",  */
/* 							fields[i]+1, */
/* 							strlen(fields[i]+1) */
/* 							); */
					}
					break;
				case 'p':
/* 				xchat_printf (ph, "pointer: %s", fields[i]+1); */
					hv_store (hash, fields[i]+1, strlen (fields[i]+1),
								newSVuv(PTR2UV ( xchat_list_str (ph, list,
																				fields[i]+1)
									)), 0);
					break;
				case 'i': 
					hv_store (hash, fields[i]+1, strlen (fields[i]+1),
								newSVuv (xchat_list_int (ph, list, fields[i]+1)), 0);
/* 				xchat_printf (ph, "int: %s - %d",fields[i]+1, */
/* 				       xchat_list_int (ph, list, fields[i]+1) */
/* 						); */
					break;
				case 't':
					hv_store (hash, fields[i]+1, strlen (fields[i]+1),
								newSVnv(xchat_list_time(ph,list,fields[i]+1)), 0);
					break;
				}
				i++;
		}
	       
		XPUSHs(newRV_noinc ((SV*)hash));
		
	}
	xchat_list_free (ph, list);

	PUTBACK;
	return;
	}
}

/* xs_init is the second argument perl_parse. As the name hints, it
   initializes XS subroutines (see the perlembed manpage) */
static void
xs_init (pTHX)
{
	char *file = __FILE__;
	HV *stash;
	
	/* This one allows dynamic loading of perl modules in perl
	   scripts by the 'use perlmod;' construction*/
	newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
	/* load up all the custom IRC perl functions */
	newXS("Xchat::_register", XS_Xchat_register, "Xchat");
	newXS("Xchat::_hook_server", XS_Xchat_hook_server, "Xchat");
	newXS("Xchat::_hook_command", XS_Xchat_hook_command, "Xchat");
	newXS("Xchat::_hook_print", XS_Xchat_hook_print, "Xchat");
	newXS("Xchat::_hook_timer", XS_Xchat_hook_timer, "Xchat");
/* 	newXS("Xchat::_hook_fd", XS_Xchat_hook_fd, "Xchat"); */
	newXS("Xchat::unhook", XS_Xchat_unhook, "Xchat");
	newXS("Xchat::_print", XS_Xchat_print, "Xchat");
	newXS("Xchat::_command", XS_Xchat_command, "Xchat");
	newXS("Xchat::find_context", XS_Xchat_find_context, "Xchat");
	newXS("Xchat::get_context", XS_Xchat_get_context, "Xchat");
	newXS("Xchat::set_context", XS_Xchat_set_context, "Xchat");
	newXS("Xchat::get_info", XS_Xchat_get_info, "Xchat");
	newXS("Xchat::get_prefs", XS_Xchat_get_prefs, "Xchat");
	newXS("Xchat::emit_print", XS_Xchat_emit_print, "Xchat");
	newXS("Xchat::nickcmp", XS_Xchat_nickcmp, "Xchat");
	newXS("Xchat::get_list", XS_Xchat_get_list, "Xchat");
	
	stash = get_hv ("Xchat::", TRUE);
	if(stash == NULL ) {
	  exit(1);
	}

	newCONSTSUB (stash, "PRI_HIGHEST", newSViv (XCHAT_PRI_HIGHEST));
	newCONSTSUB (stash, "PRI_HIGH", newSViv (XCHAT_PRI_HIGH));
	newCONSTSUB (stash, "PRI_NORM", newSViv (XCHAT_PRI_NORM));
	newCONSTSUB (stash, "PRI_LOW", newSViv (XCHAT_PRI_LOW));
	newCONSTSUB (stash, "PRI_LOWEST", newSViv (XCHAT_PRI_LOWEST));
	
	newCONSTSUB (stash, "EAT_NONE", newSViv (XCHAT_EAT_NONE));
	newCONSTSUB (stash, "EAT_XCHAT", newSViv (XCHAT_EAT_XCHAT));
	newCONSTSUB (stash, "EAT_PLUGIN", newSViv (XCHAT_EAT_PLUGIN));
	newCONSTSUB (stash, "EAT_ALL", newSViv (XCHAT_EAT_ALL));
	newCONSTSUB (stash, "FD_READ", newSViv (XCHAT_FD_READ));
	newCONSTSUB (stash, "FD_WRITE", newSViv (XCHAT_FD_WRITE));
	newCONSTSUB (stash, "FD_EXCEPTION", newSViv (XCHAT_FD_EXCEPTION));
	newCONSTSUB (stash, "FD_NOTSOCKET", newSViv (XCHAT_FD_NOTSOCKET));
	newCONSTSUB (stash, "KEEP", newSViv(1));
	newCONSTSUB (stash, "REMOVE", newSViv(0));
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
	  /* Redefine the $SIG{__WARN__} handler to have XChat
	     printing warnings in the main window. (TheHobbit)*/

	  "BEGIN {\n"
	  "$INC{'Xchat.pm'} = 'DUMMY';\n"
	  "}\n"
	  "{\n"
	  "package Xchat;\n"
	  "use base qw(Exporter);\n"
	  "our @EXPORT = (qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST),\n"
	  "qw(EAT_NONE EAT_XCHAT EAT_PLUGIN EAT_ALL),\n"
	  ");\n"
	  "our @EXPORT_OK = (qw(register hook_server hook_command hook_print),\n"
	  "qw(hook_timer unhook print command find_context),\n"
	  "qw(get_context set_context get_info get_prefs emit_print),\n"
	  "qw(nickcmp get_list context_info strip_code),\n"
	  "qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST),\n"
	  "qw(EAT_NONE EAT_XCHAT EAT_PLUGIN EAT_ALL),\n"
	  ");\n"
	  "our %EXPORT_TAGS = ( all => [\n"
	  "qw(register hook_server hook_command),\n"
	  "qw(hook_print hook_timer unhook print command),\n"
	  "qw(find_context get_context set_context),\n"
	  "qw(get_info get_prefs emit_print nickcmp),\n"
	  "qw(get_list context_info strip_code),\n"
	  "qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW),\n"
	  "qw(PRI_LOWEST EAT_NONE EAT_XCHAT EAT_PLUGIN),\n"
	  "qw(EAT_ALL),\n"
	  "],\n"
	  "constants => [\n"
	  "qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW),\n"
	  "qw(PRI_LOWEST EAT_NONE EAT_XCHAT),\n"
	  "qw(EAT_PLUGIN EAT_ALL FD_READ FD_WRITE),\n"
	  "qw(FD_EXCEPTION FD_NOTSOCKET),\n"
	  "],\n"
	  "hooks => [\n"
	  "qw(hook_server hook_command),\n"
	  "qw(hook_print hook_timer unhook),\n"
	  "],\n"
	  "util => [\n"
	  "qw(register print command find_context),\n"
	  "qw(get_context set_context get_info get_prefs),\n"
	  "qw(emit_print nickcmp get_list context_info),\n"
	  "qw(strip_code),\n"
	  "],\n"
	  ");\n"
	  "}\n"
	  "sub Xchat::register {\n"
	  "my ($name, $version, $description, $callback) = @_;\n"
	  "$description = \"\" unless defined $description;\n"
	  "$callback = undef unless $callback;\n"
	  "my $filename = caller;\n"
	  "$filename =~ s/.*:://;\n"
	  "$filename =~ s/_([[:xdigit:]]{2})/+pack('H*',$1)/eg;\n"
	  "Xchat::_register( $name, $version, $description, $callback, $filename );\n"
	  "}\n"
	  "sub Xchat::hook_server {\n"
	  "return undef unless @_ >= 2;\n"
	  "my $message = shift;\n"
	  "my $callback = shift;\n"
	  "my $options = shift;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "my ($priority, $data) = ( Xchat::PRI_NORM, undef );\n"
	  "if ( ref( $options ) eq 'HASH' ) {\n"
	  "if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {\n"
	  "$priority = $options->{priority};\n"
	  "}\n"
	  "if ( exists( $options->{data} ) && defined( $options->{data} ) ) {\n"
	  "$data = $options->{data};\n"
	  "}\n"
	  "}\n"
	  "return Xchat::_hook_server( $message, $priority, $callback, $data);\n"
	  "}\n"
	  "sub Xchat::hook_command {\n"
	  "return undef unless @_ >= 2;\n"
	  "my $command = shift;\n"
	  "my $callback = shift;\n"
	  "my $options = shift;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "my ($priority, $help_text, $data) = ( Xchat::PRI_NORM, '', undef );\n"
	  "if ( ref( $options ) eq 'HASH' ) {\n"
	  "if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {\n"
	  "$priority = $options->{priority};\n"
	  "}\n"
	  "if ( exists( $options->{help_text} ) && defined( $options->{help_text} ) ) {\n"
	  "$help_text = $options->{help_text};\n"
	  "}\n"
	  "if ( exists( $options->{data} ) && defined( $options->{data} ) ) {\n"
	  "$data = $options->{data};\n"
	  "}\n"
	  "}\n"
	  "return Xchat::_hook_command( $command, $priority, $callback, $help_text,\n"
	  "$data);\n"
	  "}\n"
	  "sub Xchat::hook_print {\n"
	  "return undef unless @_ >= 2;\n"
	  "my $event = shift;\n"
	  "my $callback = shift;\n"
	  "my $options = shift;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "my ($priority, $data) = ( Xchat::PRI_NORM, undef );\n"
	  "if ( ref( $options ) eq 'HASH' ) {\n"
	  "if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {\n"
	  "$priority = $options->{priority};\n"
	  "}\n"
	  "if ( exists( $options->{data} ) && defined( $options->{data} ) ) {\n"
	  "$data = $options->{data};\n"
	  "}\n"
	  "}\n"
	  "return Xchat::_hook_print( $event, $priority, $callback, $data);\n"
	  "}\n"
	  "sub Xchat::hook_timer {\n"
	  "return undef unless @_ >= 2;\n"
	  "my ($timeout, $callback, $data) = @_;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "if( ref( $data ) eq 'HASH' && exists( $data->{data} )\n"
	  "&& defined( $data->{data} ) ) {\n"
	  "$data = $data->{data};\n"
	  "}\n"
	  "return Xchat::_hook_timer( $timeout, $callback, $data );\n"
	  "}\n"
	  "sub Xchat::print {\n"
	  "my $text = shift @_;\n"
	  "return 1 unless $text;\n"
	  "if( ref( $text ) eq 'ARRAY' ) {\n"
	  "if( $, ) {\n"
	  "$text = join $, , @$text;\n"
	  "} else {\n"
	  "$text = join \"\", @$text;\n"
	  "}\n"
	  "}\n"
	  "if( @_ >= 1 ) {\n"
	  "my $channel = shift @_;\n"
	  "my $server = shift @_;\n"
	  "my $old_ctx = Xchat::get_context();\n"
	  "my $ctx = Xchat::find_context( $channel, $server );\n"
	  "if( $ctx ) {\n"
	  "Xchat::set_context( $ctx );\n"
	  "Xchat::_print( $text );\n"
	  "Xchat::set_context( $old_ctx );\n"
	  "return 1;\n"
	  "} else {\n"
	  "return 0;\n"
	  "}\n"
	  "} else {\n"
	  "Xchat::_print( $text );\n"
	  "return 1;\n"
	  "}\n"
	  "}\n"
	  "sub Xchat::printf {\n"
	  "my $format = shift;\n"
	  "Xchat::print( sprintf( $format, @_ ) );\n"
	  "}\n"
	  "sub Xchat::command {\n"
	  "my $command = shift;\n"
	  "my @commands;\n"
	  "if( ref( $command ) eq 'ARRAY' ) {\n"
	  "@commands = @$command;\n"
	  "} else {\n"
	  "@commands = ($command);\n"
	  "}\n"
	  "if( @_ >= 1 ) {\n"
	  "my ($channel, $server) = @_;\n"
	  "my $old_ctx = Xchat::get_context();\n"
	  "my $ctx = Xchat::find_context( $channel, $server );\n"
	  "if( $ctx ) {\n"
	  "Xchat::set_context( $ctx );\n"
	  "Xchat::_command( $_ ) foreach @commands;\n"
	  "Xchat::set_context( $old_ctx );\n"
	  "return 1;\n"
	  "} else {\n"
	  "return 0;\n"
	  "}\n"
	  "} else {\n"
	  "Xchat::_command( $_ ) foreach @commands;\n"
	  "return 1;\n"
	  "}\n"
	  "}\n"
	  "sub Xchat::commandf {\n"
	  "my $format = shift;\n"
	  "Xchat::command( sprintf( $format, @_ ) );\n"
	  "}\n"
	  "sub Xchat::user_info {\n"
	  "my $nick = shift @_ || Xchat::get_info( \"nick\" );\n"
	  "my $user;\n"
	  "for(Xchat::get_list( \"users\" ) ) {\n"
	  "if( Xchat::nickcmp( $_->{nick}, $nick ) == 0 ) {\n"
	  "$user = $_;\n"
	  "last;\n"
	  "}\n"
	  "}\n"
	  "return $user;\n"
	  "}\n"
	  "sub Xchat::context_info {\n"
	  "my $ctx = shift @_;\n"
	  "my $old_ctx = Xchat::get_context;\n"
	  "my @fields = (qw(away channel host inputbox libdirfs network nick server),\n"
	  "qw(topic version win_status xchatdir xchatdirfs),\n"
	  ");\n"
	  "if(Xchat::set_context( $ctx )) {\n"
	  "my %info;\n"
	  "for my $field ( @fields ) {\n"
	  "$info{$field} = Xchat::get_info( $field );\n"
	  "}\n"
	  "Xchat::set_context( $old_ctx );\n"
	  "return %info if wantarray;\n"
	  "return \\%info;\n"
	  "} else {\n"
	  "return undef;\n"
	  "}\n"
	  "}\n"
	  "sub Xchat::strip_code {\n"
	  "my $pattern =\n"
	  "qr/\\cB| #Bold\n"
	  "\\cC\\d{0,2}(?:,\\d{0,2})?| #Color\n"
	  "\\cG| #Beep\n"
	  "\\cO| #Reset\n"
	  "\\cV| #Reverse\n"
	  "\\c_  #Underline\n"
	  "/x;\n"
	  "if( defined wantarray ) {\n"
	  "my $msg = shift;\n"
	  "$msg =~ s/$pattern//g;\n"
	  "return $msg;\n"
	  "} else {\n"
	  "$_[0] =~ s/$pattern//g;\n"
	  "}\n"
	  "}\n"
	  "sub Xchat::_fix_callback {\n"
	  "my ($package, $callback) = @_;\n"
	  "unless( ref $callback ) {\n"
	  "$callback =~ s/^.*:://;\n"
	  "$callback = qq[${package}::$callback];\n"
	  "}\n"
	  "return $callback;\n"
	  "}\n"
	  "$SIG{__WARN__} = sub {\n"
	  "my ($package, $file, $line, $sub) = caller(1);\n"
	  "$sub =~ s/^Xchat::Embed//;\n"
	  "$sub =~ s/^(.+)::.+$/$1/;\n"
	  "$sub =~ s/_([[:xdigit:]]+)/pack(\"H*\",$1)/eg;\n"
	  "$sub =~ s[::][/]g;\n"
	  "Xchat::print( \"Warning from ${sub}.\" );\n"
	  "Xchat::print( $_[0] );\n"
	  "};\n"
	  "sub Xchat::Embed::load {\n"
	  "my $file = shift @_;\n"
	  "my $package = Xchat::Embed::valid_package( $file );\n"
	  "if( exists $INC{$package} ) {\n"
	  "Xchat::print( qq{'$file' already loaded.} );\n"
	  "return 2;\n"
	  "}\n"
	  "if( open FH, $file ) {\n"
	  "my $data = do {local $/; <FH>};\n"
	  "close FH;\n"
	  "if( my @matches = $data =~ m/^\\s*package .*?;/mg ) {\n"
	  "if( @matches > 1 ) {\n"
	  "Xchat::print( \"Too many package defintions, only 1 is allowed\" );\n"
	  "return 1;\n"
	  "}\n"
	  "$data =~ s/^\\s*package .*?;/package $package;/m;\n"
	  "} else {\n"
	  "$data = \"package $package;\" . $data;\n"
	  "}\n"
	  "eval $data;\n"
	  "if( $@ ) {\n"
	  "Xchat::print( \"Error loading '$file':\\n$@\\n\" );\n"
	  "return 1;\n"
	  "}\n"
	  "$INC{$package} = 1;\n"
	  "} else {\n"
	  "Xchat::print( \"Error opening '$file': $!\\n\" );\n"
	  "return 2;\n"
	  "}\n"
	  "return 0;\n"
	  "}\n"
	  "sub Xchat::Embed::valid_package {\n"
	  "my $string = shift @_;\n"
	  "$string =~ s|([^A-Za-z0-9/])|'_'.unpack(\"H*\",$1)|eg;\n"
	  "$string =~ s|/|::|g;\n"
	  "return \"Xchat::Embed\" . $string;\n"
	  "}\n"
#ifdef OLD_PERL
	  "sub IRC::register {\n"
	  "my ($script_name, $version, $callback) = @_;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback) if $callback;\n"
	  "Xchat::register( $script_name, $version, undef, $callback );\n"
	  "}\n"
	  "sub IRC::add_command_handler {\n"
	  "my ($command, $callback) = @_;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "my $start_index = $command ? 1 : 0;\n"
	  "Xchat::hook_command( $command,\n"
	  "sub {\n"
	  "no strict 'refs';\n"
	  "return &{$callback}($_[1][$start_index]);\n"
	  "}\n"
	  ");\n"
	  "return;\n"
	  "}\n"
	  "sub IRC::add_message_handler {\n"
	  "my ($message, $callback) = @_;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "Xchat::hook_server( $message,\n"
	  "sub {\n"
	  "no strict 'refs';\n"
	  "return &{$callback}( $_[1][0] );\n"
	  "}\n"
	  ");\n"
	  "return;\n"
	  "}\n"
	  "sub IRC::add_print_handler {\n"
	  "my ($event, $callback) = @_;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "Xchat::hook_print( $event,\n"
	  "sub {\n"
	  "my @word = @{$_[0]};\n"
	  "no strict 'refs';\n"
	  "return &{$callback}( join( ' ', @word[0..3] ), @word );\n"
	  "}\n"
	  ");\n"
	  "return;\n"
	  "}\n"
	  "sub IRC::add_timeout_handler {\n"
	  "my ($timeout, $callback) = @_;\n"
	  "my $package = caller;\n"
	  "$callback = Xchat::_fix_callback( $package, $callback );\n"
	  "Xchat::hook_timer( $timeout,\n"
	  "sub {\n"
	  "no strict 'refs';\n"
	  "&{$callback};\n"
	  "return 0;\n"
	  "}\n"
	  ");\n"
	  "return;\n"
	  "}\n"
	  "sub IRC::command {\n"
	  "my $command = shift;\n"
	  "if( $command =~ m{^/} ) {\n"
	  "$command =~ s{^/}{};\n"
	  "Xchat::command( $command );\n"
	  "} else {\n"
	  "Xchat::command( qq[say $command] );\n"
	  "}\n"
	  "}\n"
	  "sub IRC::command_with_channel {\n"
	  "my ($command, $channel, $server) = @_;\n"
	  "my $old_ctx = Xchat::get_context;\n"
	  "my $ctx = Xchat::find_context( $channel, $server );\n"
	  "if( $ctx ) {\n"
	  "Xchat::set_context( $ctx );\n"
	  "IRC::command( $command );\n"
	  "Xchat::set_context( $ctx );\n"
	  "}\n"
	  "}\n"
	  "sub IRC::command_with_server {\n"
	  "my ($command, $server) = @_;\n"
	  "my $old_ctx = Xchat::get_context;\n"
	  "my $ctx = Xchat::find_context( undef, $server );\n"
	  "if( $ctx ) {\n"
	  "Xchat::set_context( $ctx );\n"
	  "IRC::command( $command );\n"
	  "Xchat::set_context( $ctx );\n"
	  "}\n"
	  "}\n"
	  "sub IRC::dcc_list {\n"
	  "my @dccs;\n"
	  "for my $dcc ( Xchat::get_list( 'dcc' ) ) {\n"
	  "push @dccs, $dcc->{nick};\n"
	  "push @dccs, $dcc->{file} ? $dcc->{file} : '';\n"
	  "push @dccs, @{$dcc}{qw(type status cps size)};\n"
	  "push @dccs, $dcc->{type} == 0 ? $dcc->{pos} : $dcc->{resume};\n"
	  "push @dccs, $dcc->{address32};\n"
	  "push @dccs, $dcc->{destfile} ? $dcc->{destfile} : '';\n"
	  "}\n"
	  "return @dccs;\n"
	  "}\n"
	  "sub IRC::channel_list {\n"
	  "my @channels;\n"
	  "for my $channel ( Xchat::get_list( 'channels' ) ) {\n"
	  "push @channels, @{$channel}{qw(channel server)},\n"
	  "Xchat::context_info( $channel->{context} )->{nick};\n"
	  "}\n"
	  "return @channels;\n"
	  "}\n"
	  "sub IRC::get_info {\n"
	  "my $id = shift;\n"
	  "my @ids = qw(version nick channel server xchatdir away network host topic);\n"
	  "if( $id >= 0 && $id <= 8 && $id != 5 ) {\n"
	  "my $info = Xchat::get_info($ids[$id]);\n"
	  "return defined $info ? $info : '';\n"
	  "} else {\n"
	  "if( $id == 5 ) {\n"
	  "return Xchat::get_info( 'away' ) ? 1 : 0;\n"
	  "} else {\n"
	  "return 'Error2';\n"
	  "}\n"
	  "}\n"
	  "}\n"
	  "sub IRC::get_prefs {\n"
	  "return 'Unknown variable' unless defined $_[0];\n"
	  "my $result = Xchat::get_prefs(shift);\n"
	  "return defined $result ? $result : 'Unknown variable';\n"
	  "}\n"
	  "sub IRC::ignore_list {\n"
	  "my @ignores;\n"
	  "for my $ignore ( Xchat::get_list( 'ignore' ) ) {\n"
	  "push @ignores, $ignore->{mask};\n"
	  "my $flags = $ignore->{flags};\n"
	  "push @ignores, $flags & 1, $flags & 2, $flags & 4, $flags & 8, $flags & 16,\n"
	  "$flags & 32, ':';\n"
	  "}\n"
	  "return @ignores;\n"
	  "}\n"
	  "sub IRC::print {\n"
	  "Xchat::print( $_ ) for @_;\n"
	  "return;\n"
	  "}\n"
	  "sub IRC::print_with_channel {\n"
	  "Xchat::print( @_ );\n"
	  "}\n"
	  "sub IRC::send_raw {\n"
	  "Xchat::commandf( qq[quote %s], shift );\n"
	  "}\n"
	  "sub IRC::server_list {\n"
	  "my @servers;\n"
	  "for my $channel ( Xchat::get_list( 'channels' ) ) {\n"
	  "push @servers, $channel->{server} if $channel->{server};\n"
	  "}\n"
	  "return @servers;\n"
	  "}\n"
	  "sub IRC::user_info {\n"
	  "my $user;\n"
	  "if( @_ > 0 ) {\n"
	  "$user = Xchat::user_info( shift );\n"
	  "} else {\n"
	  "$user = Xchat::user_info();\n"
	  "}\n"
	  "my @info;\n"
	  "if( $user ) {\n"
	  "push @info, $user->{nick};\n"
	  "if( $user->{host} ) {\n"
	  "push @info, $user->{host};\n"
	  "} else {\n"
	  "push @info, 'FETCHING';\n"
	  "}\n"
	  "push @info, $user->{prefix} eq '@' ? 1 : 0;\n"
	  "push @info, $user->{prefix} eq '+' ? 1 : 0;\n"
	  "}\n"
	  "return @info;\n"
	  "}\n"
	  "sub IRC::user_list {\n"
	  "my ($channel, $server) = @_;\n"
	  "my $ctx = Xchat::find_context( $channel, $server );\n"
	  "my $old_ctx = Xchat::get_context;\n"
	  "if( $ctx ) {\n"
	  "Xchat::set_context( $ctx );\n"
	  "my @users;\n"
	  "for my $user ( Xchat::get_list( 'users' ) ) {\n"
	  "push @users, $user->{nick};\n"
	  "if( $user->{host} ) {\n"
	  "push @users, $user->{host};\n"
	  "} else {\n"
	  "push @users, 'FETCHING';\n"
	  "}\n"
	  "push @users, $user->{prefix} eq '@' ? 1 : 0;\n"
	  "push @users, $user->{prefix} eq '+' ? 1 : 0;\n"
	  "push @users, ':';\n"
	  "}\n"
	  "Xchat::set_context( $old_ctx );\n"
	  "return @users;\n"
	  "} else {\n"
	  "return;\n"
	  "}\n"
	  "}\n"
	  "sub IRC::user_list_short {\n"
	  "my ($channel, $server) = @_;\n"
	  "my $ctx = Xchat::find_context( $channel, $server );\n"
	  "my $old_ctx = Xchat::get_context;\n"
	  "if( $ctx ) {\n"
	  "Xchat::set_context( $ctx );\n"
	  "my @users;\n"
	  "for my $user ( Xchat::get_list( 'users' ) ) {\n"
	  "my $nick = $user->{nick};\n"
	  "my $host = $user->{host} || 'FETCHING';\n"
	  "push @users, $nick, $host;\n"
	  "}\n"
	  "Xchat::set_context( $old_ctx );\n"
	  "return @users;\n"
	  "} else {\n"
	  "return;\n"
	  "}\n"
	  "}\n"
	  "sub IRC::add_user_list {}\n"
	  "sub IRC::sub_user_list {}\n"
	  "sub IRC::clear_user_list {}\n"
	  "sub IRC::notify_list {}\n"
	  "sub IRC::perl_script_list {}\n"
#endif

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

	eval_pv (perl_definitions, TRUE);

}


static int
perl_load_file (char *script_name)
{
#ifdef WIN32
	static HINSTANCE lib = NULL;

	if (!lib)
	{
		lib = LoadLibrary (PERL_DLL);
		if (!lib)
		{
			thread_mbox ("Cannot open " PERL_DLL "\n\n"
				     "You must have ActivePerl installed in order to\n"
				     "run perl scripts.\n\n"
				     "http://www.activestate.com/ActivePerl/\n\n"
				     "Make sure perl's bin directory is in your PATH.");
			return FALSE;
		}
		FreeLibrary (lib);
	}
#endif

	if (my_perl == NULL) {
		perl_init ();
	}

	return execute_perl (sv_2mortal (newSVpv ("Xchat::Embed::load", 0)),
								  script_name);
	
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
	PerlScript *scp;
	HookData *tmp;

	while (perl_list)
	{
		scp = perl_list->data;
		perl_list = g_slist_remove (perl_list, scp);

		if (SvTRUE (scp->shutdowncallback)) {
			execute_perl (scp->shutdowncallback, "");
		}
		xchat_plugingui_remove (ph, scp->gui_entry);
		free (scp->name);
		free (scp->version);
		free (scp->desc);
		SvREFCNT_dec(scp->shutdowncallback);
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
		if (tmp) {
			free (tmp);

		}
		hook_list = g_slist_remove (hook_list, hook_list->data);
	}

}

static int
perl_command_unloadall (char *word[], char *word_eol[], void *userdata)
{
	perl_end ();

	return XCHAT_EAT_XCHAT;
}

static int
perl_command_reloadall (char *word[], char *word_eol[], void *userdata)
{
	perl_end ();
	perl_auto_load ();

	return XCHAT_EAT_XCHAT;
}

static int
perl_command_unload (char *word[], char *word_eol[], void *userdata)
{
	int len;
	PerlScript *scp;
	GSList *list;

	/* try it by filename */
	len = strlen (word[2]);
	if (len > 3 && strcasecmp (".pl", word[2] + len - 3) == 0)
	{
		/* can't unload without risking memory leaks */
		xchat_print (ph, "Unloading individual perl scripts is not supported.\nYou may use /UNLOADALL to unload all Perl scripts.\n");
		return XCHAT_EAT_XCHAT;
	}

	/* try it by script name */
	list = perl_list;
	while (list)
	{
		scp = list->data;
		if (strcasecmp (scp->name, word[2]) == 0)
		{
			xchat_print (ph, "Unloading individual perl scripts is not supported.\nYou may use /UNLOADALL to unload all Perl scripts.\n");
			return XCHAT_EAT_XCHAT;
		}
		list = list->next;
	}

	return XCHAT_EAT_NONE;
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
		return XCHAT_EAT_XCHAT;
	}

	return XCHAT_EAT_NONE;
}


void xchat_plugin_get_info(char **name, char **desc, char **version, void **reserved)
{
   *name = "Perl";
   *desc = "Perl scripting interface";
   *version = VERSION;
   if (reserved)
      *reserved = NULL;
}


/* Reinit safeguard */

static int initialized = 0;
static int reinit_tried = 0;

int
xchat_plugin_init (xchat_plugin *plugin_handle,
				char **plugin_name, char **plugin_desc, char **plugin_version,
				char *arg)
{
	ph = plugin_handle;
	
	if (initialized != 0)
	{
		xchat_print (ph, "Perl interface already loaded\n");
		reinit_tried++;
		return 0;
	}
	initialized = 1;

	xchat_plugin_get_info (plugin_name, plugin_desc, plugin_version, NULL);

	xchat_hook_command (ph, "load", XCHAT_PRI_NORM, perl_command_load, 0, 0);
	xchat_hook_command (ph, "unload", XCHAT_PRI_NORM, perl_command_unload, 0,
							  0);
	xchat_hook_command (ph, "unloadall", XCHAT_PRI_NORM, perl_command_unloadall,
							  0, 0);
	xchat_hook_command (ph, "reloadall", XCHAT_PRI_NORM, perl_command_reloadall,
							  0, 0);

	perl_auto_load ();

	xchat_print (ph, "Perl interface loaded\n");

	return 1;
}

int
xchat_plugin_deinit (xchat_plugin *plugin_handle)
{
	if (reinit_tried)
	{
		reinit_tried--;
		return 1;
	}

	perl_end ();

	xchat_print (plugin_handle, "Perl interface unloaded\n");

	return 1;
}
