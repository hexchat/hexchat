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
	const char * const * fields;
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
	newXS("Xchat::_set_context", XS_Xchat_set_context, "Xchat");
	newXS("Xchat::_get_info", XS_Xchat_get_info, "Xchat");
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
#include "xchat.pm.h"
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
