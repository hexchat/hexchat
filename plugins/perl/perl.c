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
	xchat_hook *hook;
	
} HookData;

static PerlInterpreter *my_perl = NULL;
static GSList *perl_list = NULL;
static GSList *hook_list = NULL;

#ifdef OLD_PERL
static GSList *compat_hook_list = NULL;
#endif

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
	XPUSHs ( newSVpvn (args, strlen (args)));
	PUTBACK;

	count = call_sv(function, G_EVAL | G_KEEPERR| G_SCALAR);
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

	count =	call_sv (data->callback, G_EVAL | G_KEEPERR);
	SPAGAIN;
	if (SvTRUE (ERRSV))
	{
		xchat_printf (ph, "Error in server callback %s",
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

	AV* wd = newAV ();
	AV* wd_eol = newAV ();

	dSP;
	ENTER;
	SAVETMPS;
	
	for (count = 1;
	(count < 32) && (word[count] != NULL) && (word[count][0] != 0);
	count++)
	{
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

	count =	call_sv (data->callback, G_EVAL | G_KEEPERR);
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

	AV* wd = newAV ();
	AV* wd_eol = newAV ();

	dSP;
	ENTER;
	SAVETMPS;
	
	for (count = 1;
	(count < 32) && (word[count] != NULL) && (word[count][0] != 0);
	count++)
	{
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

	count =	call_sv (data->callback, G_EVAL | G_KEEPERR);
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

	AV* wd = newAV ();

	dSP;
	ENTER;
	SAVETMPS;
	
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

	count =	call_sv (data->callback, G_EVAL | G_KEEPERR);
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

/* Xchat::register (scriptname, version, desc, shutdowncallback,)
 *  all scripts should call this at startup
 *
 */

static XS (XS_Xchat_register)
{
	char *name, *ver, *desc;
	SV *callback;
	PerlScript *scp;
	dXSARGS;
	if (items < 2 || items > 4) {
		xchat_printf (ph,
		"Usage: Xchat::register(scriptname, version, [desc,[shutdowncallback]]) %d",items);
	} else {
		name = SvPV_nolen (ST (0));
		ver = SvPV_nolen (ST (1));
		desc = NULL;
		callback = &PL_sv_undef;

		switch(items)
		{
		case 3:
			desc = SvPV_nolen (ST (2));
			break;
		case 4:
			desc = SvPV_nolen (ST (2));
			callback = ST (3);
			break;
		}
	  
		if(desc == NULL ) {
			desc = "";
		}
		scp = malloc (sizeof (PerlScript));
		scp->name = strdup (name);
		scp->version = strdup (ver);
		scp->desc = strdup (desc);
	  
		scp->shutdowncallback = sv_mortalcopy (callback);
		SvREFCNT_inc (scp->shutdowncallback);
		/* FIXME: no filename */
		scp->gui_entry = xchat_plugingui_add (ph, scp->name, scp->name,
															scp->desc, scp->version, NULL);
	  
		perl_list = g_slist_prepend (perl_list, scp);
	  
		XSRETURN_EMPTY;
	}
}


/* Xchat::print(output) */
static XS (XS_Xchat_print)
{
  
	char *text = NULL;
/* 	char *server = NULL; */
/* 	char *channel = NULL; */
/*  	xchat_context *oldctx; */
/* 	xchat_context *newctx; */

	dXSARGS;
	if (items != 1 ) {
		xchat_print (ph, "Usage: Xchat::_print(text)");
	} else {
		text = SvPV_nolen (ST (0));

/* 	if (items == 1) { */
		xchat_print (ph, text);
/* 	} */
/* 	else */
/* 	{ */

/* 		switch (items) */
/* 		{ */
/* 		case 2: /\* server name only *\/ */
/* 			server = SvPV_nolen (ST (0)); */
/* 			/\* chan is already NULL *\/ */
/* 			break; */
/* 		case 3: /\* server and channel *\/ */
/* 			server = SvPV_nolen (ST (0)); */
/* 			channel = SvPV_nolen (ST (1)); */
/* 			break; */
/* 		} */
    
/* 		oldctx = xchat_get_context (ph); */
/* 		newctx = xchat_find_context (ph, server, channel); */

/* 		if (newctx != NULL) */
/* 		{ */
/* 			xchat_set_context (ph, newctx); */
/* 			xchat_print (ph, text); */
/* 			xchat_set_context (ph, oldctx); */
/* 		} */
/* 		else */
/* 		{ */
/* 			xchat_printf (ph, */
/* 			"print: context not found (server = %s, channel = %s", */
/* 					server, channel); */
/* 		} */
/* 	} */
	}
	XSRETURN_EMPTY;
}

/* static XS (XS_Xchat_printf) */
/* { */
/* 	SV *	format; */
/* 	AV *args; */
/* 	SV * print_str; */
/* 	int i = 0; */

/* 	dXSARGS; */

/* 	if (items < 1) */
/* 		xchat_print (ph, "Usage: Xchat::printf(format, ...)"); */
/* 	{ */
/* 	format = ST (0); */
/* 	args = newAV (); */
/* 	print_str = newSV (0); */

/* 	if (items == 1) */
/* 	{ */
/* 		xchat_print (ph, SvPV_nolen (format)); */
/* 		XSRETURN_EMPTY; */
/* 	} */
/* 	for (i = 1; i < items; i++) */
/* 	{ */
/* 		av_push (args, ST (i)); */
/* 	} */

/* 	sv_vsetpvfn (print_str, SvPV_nolen (format), sv_len (format), NULL, */
/* 			AvARRAY (args), av_len (args) + 1, NULL); */
/* 	xchat_print (ph, SvPV_nolen (print_str)); */

/* 	} */
/* 	XSRETURN_EMPTY; */
/* } */

static XS (XS_Xchat_emit_print)
{
	char *event_name;
	int RETVAL;

	dXSARGS;
	if (items < 1) {
		xchat_print (ph, "Usage: Xchat::emit_print(event_name, ...)");
	} else {
		event_name = (char *)SvPV_nolen (ST (0));
		RETVAL = 0;
	  
		switch  (items) {
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
							SvPV_nolen (ST (2)), 
							SvPV_nolen (ST (3)),
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

		XSRETURN_IV(PTR2IV(RETVAL));
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

		XSRETURN_IV (PTR2IV(RETVAL));
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

		XSRETURN_IV(PTR2IV(RETVAL));
	}
}

/* Xchat::_hook_timer(timeout, callback, [userdata]) */
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

		data->callback = sv_mortalcopy (callback);
		SvREFCNT_inc (data->callback);
		data->userdata = sv_mortalcopy (userdata);
		SvREFCNT_inc (data->userdata);
		RETVAL = xchat_hook_timer (ph, timeout, timer_cb, data);
		data->hook = RETVAL;
		hook_list = g_slist_append (hook_list, RETVAL);

		XSRETURN_IV (PTR2IV(RETVAL));
	}
}

static XS(XS_Xchat_unhook)
{
	xchat_hook *	hook;
	HookData *userdata;
	dXSARGS;
	if (items != 1) {
		xchat_print (ph, "Usage: Xchat::unhook(hook)");
	} else {
		hook = INT2PTR(xchat_hook *,SvIV(ST (0)));
		userdata = (HookData*)xchat_unhook (ph, hook);
		hook_list = g_slist_remove (hook_list, hook);

		XPUSHs(userdata->userdata);
	}
	XSRETURN(1);
}

/* Xchat::_command(command) */
static XS (XS_Xchat_command)
{
	char *cmd = NULL;
/* 	char *server = NULL; */
/* 	char *channel = NULL; */
/* 	xchat_context *oldctx; */
/* 	xchat_context *newctx; */

	dXSARGS;
	if (items != 1 ) {
		xchat_print (ph, "Usage: Xchat::_command(command)");
	} else {
		cmd = SvPV_nolen (ST (0));
    
/* 	if (items == 1) { */
		xchat_command (ph, cmd);
/* 	} */
/* 	else */
/* 	{ */

/* 		switch (items) */
/* 		{ */
/* 		case 2: /\* server name only *\/ */
/* 			server = SvPV_nolen (ST (0)); */
/* 			/\* chan is already NULL *\/ */
/* 			break; */
/* 		case 3: /\* server and channel *\/ */
/* 			server = SvPV_nolen (ST (0)); */
/* 			channel = SvPV_nolen (ST (1)); */
/* 			break; */
/* 		} */

/* 		oldctx = xchat_get_context (ph); */
/* 		newctx = xchat_find_context (ph, server, channel); */

/* 		if (newctx != NULL) */
/* 		{ */
/* 			xchat_set_context (ph, newctx); */
/* 			xchat_command (ph, cmd); */
/* 			xchat_set_context (ph, oldctx); */
/* 		} */
/* 		else */
/* 		{ */
/* 			xchat_printf (ph, */
/* 		     "command: context not found (server = %s, channel = %s", */
/* 				     server, channel); */
/* 		} */
/* 	} */
	}
	XSRETURN_EMPTY;
}

/* static XS (XS_Xchat_commandf) */
/* { */
/* 	SV *format; */
/* 	AV *args; */
/* 	SV *command; */
/* 	int i = 1; */
/* 	dXSARGS; */
/* 	if (items < 1) */
/* 		xchat_print (ph, "Usage: Xchat::commandf(format, ...)"); */
/* 	{ */
/* 	format = ST (0); */
/* 	args = newAV (); */
/* 	command = newSV(0); */
	
/* 	for (i = 1; i < items; i++) */
/* 	{ */
/* 		av_push (args, ST (i)); */
/* 	} */

/* 	sv_vsetpvfn (command, SvPV_nolen (format), sv_len (format), NULL, */
/* 			AvARRAY (args), av_len (args) + 1, NULL); */
/* 	xchat_command (ph, SvPV_nolen (command)); */

/* 	} */
/* 	XSRETURN_EMPTY; */
/* } */

static XS (XS_Xchat_find_context)
{
	char * server = NULL;
	char * chan = NULL;
	xchat_context * RETVAL;

	dXSARGS;
	if (items > 2)
		xchat_print (ph, "Usage: Xchat::find_context (channel, server)");
	{

	switch (items)
	{
	case 0: /* no server name and no channel name */
		/* nothing to do, server and chan are already NULL */
		break;
	case 1: /* channel name only */
		/* change channel value only if it is defined */
		/* otherwise leave it as null */
		if (ST (0) != &PL_sv_undef){
			chan = SvPV_nolen (ST (0));
/* 			xchat_printf( ph, "XSUB - find_context( %s, NULL )", chan ); */
		}/*  else { xchat_print( ph, "XSUB - find_context( NULL, NULL )" ); } */
		/* chan is already NULL */
		break;
	case 2: /* server and channel */
		/* change channel value only if it is defined */
		/* otherwise leave it as NULL */
		if (ST (0) != &PL_sv_undef){
			chan = SvPV_nolen (ST (0));
/* 			xchat_print( ph, "XSUB - 2 arg NULL chan" ); */
		}
		/* change server value only if it is defined */
		/* otherwise leave it as NULL */
		if (ST (1) != &PL_sv_undef){
			server = SvPV_nolen (ST (1));
/* 			xchat_print( ph, "XSUB - 2 arg NULL server" ); */
		}

		break;
	}

	RETVAL = xchat_find_context (ph, server, chan);
	if (RETVAL != NULL)
	{
/*  		xchat_print (ph, "XSUB - context found"); */
		XSRETURN_IV(PTR2IV(RETVAL));
	}
	else
	{
/*  		xchat_print (ph, "XSUB - context not found"); */
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
		XSRETURN_IV(PTR2IV(xchat_get_context (ph)));
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
	int i = 0;

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
								newSViv(PTR2IV ( xchat_list_str (ph, list,
																				fields[i]+1)
									)), 0);
					break;
				case 'i': 
					hv_store (hash, fields[i]+1, strlen (fields[i]+1),
								newSViv (xchat_list_int (ph, list, fields[i]+1)), 0);
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

#ifdef OLD_PERL
static int
compat_execute_perl (char *function, char *args)
{
	char *perl_args[2];
	int count, ret_value = 1;
	SV *sv;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(sp);
	perl_args[0] = args;
	perl_args[1] = NULL;
	count = call_argv(function, G_EVAL | G_SCALAR, perl_args);
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
	compat_execute_perl (perl_callback, "");
	compat_hook_list = g_slist_remove (compat_hook_list, perl_callback);
	free (perl_callback);
	return 0;	/* returning zero removes the timeout handler */
}

static int
perl_server_cb (char *word[], char *word_eol[], void *perl_callback)
{
	return compat_execute_perl (perl_callback, word_eol[1]);
}

static int
perl_command_cb (char *word[], char *word_eol[], void *perl_callback)
{
	return compat_execute_perl (perl_callback, word_eol[2]);
}

static int
perl_print_cb (char *word[], void *perl_callback)
{
	char *arg;
	int count = 1, retVal = 0;

	dSP;
	ENTER;
	SAVETMPS;

	arg = g_strdup_printf ("%s %s %s %s", word[1], word[2], word[3],
								word[4]);

	PUSHMARK( SP );
	XPUSHs( sv_2mortal( newSVpv( arg, strlen(arg) ) ) );

	for( count = 1;
	(count < 32) && (word[count] != NULL) && (word[count][0] != 0);
	count++ )
	{
		XPUSHs( sv_2mortal( newSVpv( word[count], 0 ) ) );
	}
	PUTBACK;

	count = call_pv( (char*)perl_callback, G_EVAL | G_KEEPERR );

	SPAGAIN;

	if (SvTRUE (ERRSV))
	{
		xchat_printf( ph, "Error in print callback %s",
		SvPV_nolen(ERRSV) );
		POPs;
		retVal = XCHAT_EAT_NONE;
	} else
	{
		if (count != 1)
		{
			xchat_print (ph,
				"Print handler should only return 1 value.");
			retVal = XCHAT_EAT_XCHAT;
		} else
		{
			retVal = POPi;
		}
	}

	g_free(arg);
	PUTBACK;
	FREETMPS;
	LEAVE;
	return retVal;
}

/* custom IRC perl functions for scripting */

/* IRC::register (scriptname, version, shutdowncallback, unused)

 *  all scripts should call this at startup
 *
 */

static XS (XS_IRC_register)
{
	char *name, *ver, *desc; 
	SV *callback;
	PerlScript *scp;
	dXSARGS;
	
	if (items != 4) {
	     xchat_print (ph,
	"Usage: IRC::register(scriptname, version, shutdowncallback, desc)");
		XSRETURN_EMPTY;
	} else {
		name = SvPV_nolen (ST (0));
		ver = SvPV_nolen (ST (1));
		callback = ST (2);
		desc = SvPV_nolen (ST (3));

		scp = malloc (sizeof (PerlScript));
		scp->name = strdup (name);
		scp->version = strdup (ver);
		scp->desc = strdup (desc);
		scp->shutdowncallback = sv_mortalcopy (callback);
		SvREFCNT_inc( scp->shutdowncallback );
		/* FIXME: no filename */
		scp->gui_entry = xchat_plugingui_add (ph, scp->name,
							scp->name, scp->desc,
							  scp->version, NULL);

		perl_list = g_slist_prepend (perl_list, scp);

		XST_mPV (0, VERSION);
		XSRETURN (1);
	}
}


/* print to main window */
/* IRC::main_print(output) */
static XS (XS_IRC_print)
{
	int i;
	char *output;
	dXSARGS;

	for (i = 0; i < items; ++i)
	{
		output = SvPV_nolen (ST (i));
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
	void *ctx, *old_ctx;
	char *server = NULL;
	dXSARGS;

	if (items > 2)
	{
		server = SvPV_nolen (ST (2));
		if (!server[0])
			server = NULL;
	}

	old_ctx = xchat_get_context (ph);
	ctx = xchat_find_context (ph, server, SvPV_nolen (ST (1)));
	if (ctx)
	{
		xchat_set_context (ph, ctx);
		xchat_print (ph, SvPV_nolen (ST (0)));
		xchat_set_context (ph, old_ctx);
		XSRETURN_YES;
	}

	XSRETURN_NO;
}

static XS (XS_IRC_get_info)
{
	char *ret;
	static const char *ids[] = {"version", "nick", "channel", "server",
				 "xchatdir", NULL, "network", "host", "topic"};
	int i;
	dXSARGS;

	i = SvIV (ST (0));
	if( items != 1 ) {
		xchat_print (ph, "Usage: IRC::get_info(id)");
		XSRETURN_EMPTY;
	} else {
		if (i < 9 && i >= 0 && i != 5) {
			ret = (char *)xchat_get_info (ph, ids[i]);
		} else {
			switch (i) {
				case 5:
					if (xchat_get_info (ph, "away")) {
						XST_mIV (0, 1);
					} else {
						XST_mIV (0, 0);
					}
					XSRETURN (1);

				default:
					ret = "Error2";
			}
		}

		if (ret) {
			XST_mPV (0, ret);
		} else {
			XST_mPV (0, "");	/* emulate 1.8.x behaviour */
		}
		XSRETURN (1);
	}
}

/* Added by TheHobbit <thehobbit@altern.org>*/
/* IRC::get_prefs(var) */
static XS (XS_IRC_get_prefs)
{
	const char *str;
	int integer;
	dXSARGS;

	if( items != 1 ) {
		xchat_print (ph, "Usage: IRC::get_prefs(name)");
		XSRETURN_EMPTY;
	}
	switch (xchat_get_prefs (ph, SvPV_nolen (ST (0)), &str, &integer))
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
	char *tmp;
	char *name;
	void *hook;
	dXSARGS;

	if( items != 2 ) {
		xchat_print (ph,
			"Usage: IRC::add_message_handler(message,callback)");
	} else {
		tmp = strdup (SvPV_nolen (ST (1)));
		name = SvPV_nolen (ST (0));
		if (strcasecmp (name, "inbound") == 0) {
			name = "RAW LINE";
		}
		hook = xchat_hook_server (ph, name, XCHAT_PRI_NORM, perl_server_cb,
									tmp);

		compat_hook_list = g_slist_prepend (compat_hook_list, hook);
	
	}
	XSRETURN_EMPTY;
}

/* add handler for commands with command_name */
/* IRC::add_command_handler(command_name, handler_name) */
static XS (XS_IRC_add_command_handler)
{
	char *tmp;
	void *hook;
	dXSARGS;

	if( items != 2 ) {
		xchat_print (ph,
			"Usage: IRC::add_command_handler(command,callback)");
	} else {
		tmp = strdup (SvPV_nolen (ST (1)));

		/* use perl_server_cb when it's a "" hook, so that it gives
		   word_eol[1] as the arg, instead of word_eol[2] */
		hook = xchat_hook_command (ph, SvPV_nolen (ST (0)),
						XCHAT_PRI_NORM,
						SvPV_nolen (ST (0))[0] == 0 ? 
					perl_server_cb : perl_command_cb,
								NULL, tmp);

		compat_hook_list = g_slist_prepend (compat_hook_list, hook);
		
	}
	XSRETURN_EMPTY;
}

/* add handler for commands with print_name */
/* IRC::add_print_handler(print_name, handler_name) */
static XS (XS_IRC_add_print_handler)
{
	char *tmp;
	void *hook;
	dXSARGS;

	if( items != 2 ) {
		xchat_print (ph,
			"Usage: IRC::add_print_handler(event,callback)");
	} else {
		tmp = strdup (SvPV_nolen (ST (1)));

		hook = xchat_hook_print (ph, SvPV_nolen (ST (0)),
						XCHAT_PRI_NORM,
						perl_print_cb, tmp);

		compat_hook_list = g_slist_prepend (compat_hook_list, hook);
	}
	XSRETURN_EMPTY;
}

static XS (XS_IRC_add_timeout_handler)
{
	void *hook;
	dXSARGS;

	if( items != 2 ) {
		xchat_print (ph,
			"Usage: IRC::add_timeout_handler(interval,callback)");
	} else {
		hook = xchat_hook_timer (ph, SvIV (ST (0)), perl_timer_cb,
						strdup (SvPV_nolen (ST (1))));
		compat_hook_list = g_slist_prepend (compat_hook_list, hook);
	}
	XSRETURN_EMPTY;
}

/* send raw data to server */
/* IRC::send_raw(data) */
static XS (XS_IRC_send_raw)
{
	dXSARGS;
	
	if( items != 1 ) {
		xchat_print (ph, "Usage: IRC::send_raw(message)");
	} else {
		xchat_commandf (ph, "quote %s", SvPV_nolen (ST (0)));
	}
	XSRETURN_EMPTY;
}

static XS (XS_IRC_channel_list)
{
	int i = 0;
	xchat_list *list;
	xchat_context *old = xchat_get_context (ph);
	
	dXSARGS;
	
	if( items != 0 ) {
		xchat_print (ph, "Usage: IRC::channel_list()");
		XSRETURN_EMPTY;
	} else {
		list = xchat_list_get (ph, "channels");
		if (list) {
			while (xchat_list_next (ph, list)) {
				XST_mPV (i, xchat_list_str
							(ph, list, "channel"));
				i++;
				XST_mPV (i, xchat_list_str
							(ph, list, "server"));
				i++;
				xchat_set_context (ph,
					(xchat_context *) xchat_list_str (ph, list, "context"));
				XST_mPV (i, xchat_get_info (ph, "nick"));
				i++;
			}

			xchat_list_free (ph, list);
		}

		xchat_set_context (ph, old);

		XSRETURN (i);
	}
}

static XS (XS_IRC_server_list)
{
	int i = 0;
	xchat_list *list;
	dXSARGS;

	if( items != 0 ) {
		xchat_print (ph, "Usage: IRC::server_list()");
		XSRETURN_EMPTY;
	} else {
		list = xchat_list_get (ph, "channels");
		if (list) {
			while (xchat_list_next (ph, list)) {
				XST_mPV (i, xchat_list_str
							(ph, list, "server"));
				i++;
			}

			xchat_list_free (ph, list);
		}

		XSRETURN (i);
	}
}

static XS (XS_IRC_ignore_list)
{
	int i = 0, flags;
	xchat_list *list;
	
	dXSARGS;

	if( items != 0 ) {
		xchat_print (ph, "Usage: IRC::ignore_list()");
		XSRETURN_EMPTY;
	} else {
		list = xchat_list_get (ph, "ignore");
		if (list) {
			while (xchat_list_next (ph, list)) {
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
}

static XS (XS_IRC_notify_list)
{
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

	XSRETURN_IV (items);
}


/*

   IRC::user_info( nickname )

 */

static XS (XS_IRC_user_info)
{
	dXSARGS;
	xchat_list *list;
	int i = 0;
	const char *nick;
	const char *find_nick;
	const char *host, *prefix;

	if( items > 1 ) {
		xchat_print (ph, "Usage: IRC::user_info([nick])");
		XSRETURN_EMPTY;
	} else {
		find_nick = SvPV_nolen (ST (0));
		if (find_nick[0] == 0) {
			find_nick = xchat_get_info (ph, "nick");
		}

		list = xchat_list_get (ph, "users");
		if (list) {
			while (xchat_list_next (ph, list)) {
				nick = xchat_list_str (ph, list, "nick");

				if (xchat_nickcmp (ph, 
					(char *)nick, (char *)find_nick) == 0)
				{
					XST_mPV (i, nick);
					i++;
					host = xchat_list_str (ph, list,
								"host");
					if (!host) {
						host = "FETCHING";
					}
					XST_mPV (i, host);
					i++;

					prefix = xchat_list_str
							(ph, list, "prefix");

					XST_mIV (i,(prefix[0] == '@') ? 1 : 0);
					i++;

					XST_mIV (i,(prefix[0] == '+') ? 1 : 0);
					i++;

					xchat_list_free (ph, list);
					XSRETURN (4);
				}
			}

			xchat_list_free (ph, list);
		}
		
		XSRETURN (0);
	}
}

/*
 * IRC::add_user_list(ul_channel, ul_server, nick, user_host,
 * 		      realname, server)
 */
static XS (XS_IRC_add_user_list)
{
	dXSARGS;

	XSRETURN_IV(items);
}

/*
 * IRC::sub_user_list(ul_channel, ul_server, nick)
 */
static XS (XS_IRC_sub_user_list)
{
	dXSARGS;

	XSRETURN_IV(items);
}

/*
 * IRC::clear_user_list(channel, server)
 */
static XS (XS_IRC_clear_user_list)
{
	dXSARGS;

	XSRETURN_IV(items);
};

/*

   IRC::user_list( channel, server )

 */

static XS (XS_IRC_user_list)
{
	const char *host;
	const char *prefix;
	int i = 0;
	dXSARGS;
	xchat_list *list;
	xchat_context *ctx, *old = xchat_get_context (ph);

	if( items != 2 ) {
		xchat_print (ph, "Usage: IRC::user_list(channel,server)");
		XSRETURN_EMPTY;
	} else {
		ctx = xchat_find_context (ph, SvPV_nolen (ST (1)),
							SvPV_nolen (ST (0)));
		if (!ctx) {
			XSRETURN (0);
		}
		xchat_set_context (ph, ctx);

		list = xchat_list_get (ph, "users");
		if (list) {
			while (xchat_list_next (ph, list)) {
				/* Make sure there is room on the stack */
				EXTEND(SP, i + 9);

				XST_mPV (i, xchat_list_str (ph, list, "nick"));
				i++;
				host = xchat_list_str (ph, list, "host");
				if (!host) {
					host = "FETCHING";
				}
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
}

static XS (XS_IRC_dcc_list)
{
	int i = 0;
	xchat_list *list;
	const char *file;
	int type;

	dXSARGS;
	if( items != 0 ) {
		xchat_print (ph, "Usage: IRC::dcc_list()");
		XSRETURN_EMPTY;
	} else {
		list = xchat_list_get (ph, "dcc");
		if (list) {
			while (xchat_list_next (ph, list)) {
				/* Make sure there is room on the stack */
				EXTEND (SP, i + 11);

				XST_mPV (i, xchat_list_str (ph, list, "nick"));
				i++;

				file = xchat_list_str (ph, list, "file");
				if (!file) {
					file = "";
				}

				XST_mPV (i, file);
				i++;
				XST_mIV (i, xchat_list_int (ph, list, "type"));
				i++;
				XST_mIV (i, xchat_list_int
							(ph, list, "status"));
				i++;
				XST_mIV (i, xchat_list_int (ph, list, "cps"));
				i++;
				XST_mIV (i, xchat_list_int (ph, list, "size"));
				i++;
				type = xchat_list_int (ph, list, "type");
				if (type == 0) {
					XST_mIV (i, xchat_list_int
							(ph, list, "pos"));
				} else {
					XST_mIV (i, xchat_list_int
							(ph, list, "resume"));
				}

				i++;
				XST_mIV (i, xchat_list_int
						(ph, list, "address32"));
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
}

/* run internal xchat command */
/* IRC::command(command) */
static XS (XS_IRC_command)
{
	char *cmd;
	dXSARGS;

	if( items != 1 ) {
		xchat_print (ph, "Usage: IRC::command(command)");
	} else {
		cmd = SvPV_nolen (ST (0));
		if (cmd[0] == '/') {
			/* skip the forward slash */
			xchat_command (ph, cmd + 1);
		} else {
			xchat_commandf (ph, "say %s", cmd);
		}
	}
	XSRETURN_EMPTY;
}

/* command_with_server(command, server) */
static XS (XS_IRC_command_with_server)
{
	void *ctx, *old_ctx;
	char *cmd;

	dXSARGS;

	if( items != 2 ) {
		xchat_print (ph,
			"Usage: IRC::command_with_server(command,server)");
		XSRETURN_EMPTY;
	} else {
		cmd = SvPV_nolen (ST (0));
		old_ctx = xchat_get_context (ph);
		ctx = xchat_find_context (ph, SvPV_nolen (ST (1)), NULL);
		if (ctx) {
			xchat_set_context (ph, ctx);

			if (cmd[0] == '/') {
				/* skip the forward slash */
				xchat_command (ph, cmd + 1);
			} else {
				xchat_commandf (ph, "say %s", cmd);
			}
			xchat_set_context (ph, old_ctx);
			XSRETURN_YES;
		}

		XSRETURN_NO;
	}
}

/* command_with_channel( command, channel, [server] ) */
static XS (XS_IRC_command_with_channel)
{
	
	void *ctx, *old_ctx;
	char *cmd;
	char *server = NULL;

	dXSARGS;

	cmd = SvPV_nolen (ST (0));

	if (items > 2)
	{
		server = SvPV_nolen (ST (2));
		if (!server[0])
			server = NULL;
	}

	old_ctx = xchat_get_context (ph);
	ctx = xchat_find_context (ph, server, SvPV_nolen (ST (1)));
	if (ctx)
	{
		xchat_set_context (ph, ctx);

		if (cmd[0] == '/')
			/* skip the forward slash */
			xchat_command (ph, cmd + 1);
		else
			xchat_commandf (ph, "say %s", cmd);

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
	int i = 0;
	
	xchat_list *list;
	xchat_context *ctx, *old = xchat_get_context (ph);

	dXSARGS;
	
	if( items != 2 ) {
		xchat_print (ph,
			"Usage: IRC::user_list_short(channel,server)");
		XSRETURN_EMPTY;
	} else {
		ctx = xchat_find_context (ph, SvPV_nolen (ST (1)), SvPV_nolen (ST (0)));
		if (!ctx) {
			XSRETURN (0);
		}
		xchat_set_context (ph, ctx);

		list = xchat_list_get (ph, "users");
		if (list) {
			while (xchat_list_next (ph, list)) {
				/* Make sure there is room on the stack */
				EXTEND(SP, i + 5);

				XST_mPV (i, xchat_list_str (ph, list, "nick"));
				i++;
				host = xchat_list_str (ph, list, "host");
				if (!host) {
					host = "FETCHING";
				}
				XST_mPV (i, host);
				i++;
			}

			xchat_list_free (ph, list);
		}

		xchat_set_context (ph, old);

		XSRETURN (i);
	}
}

static XS (XS_IRC_perl_script_list)
{
	int i = 0;
	GSList *handler;
	dXSARGS;
	if (items != 0) {
		xchat_print (ph, "Usage: IRC::perl_script_list()");
	} else {
	  handler = perl_list;
	  while (handler) {
	    PerlScript *scp = handler->data;
	    
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
}
#endif

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
	newXS("Xchat::register", XS_Xchat_register, "Xchat");
	newXS("Xchat::_hook_server", XS_Xchat_hook_server, "Xchat");
	newXS("Xchat::_hook_command", XS_Xchat_hook_command, "Xchat");
	newXS("Xchat::_hook_print", XS_Xchat_hook_print, "Xchat");
	newXS("Xchat::_hook_timer", XS_Xchat_hook_timer, "Xchat");
	newXS("Xchat::unhook", XS_Xchat_unhook, "Xchat");
	newXS("Xchat::_print", XS_Xchat_print, "Xchat");
/* 	newXS("Xchat::printf", XS_Xchat_printf, "Xchat"); */
	newXS("Xchat::_command", XS_Xchat_command, "Xchat");
/* 	newXS("Xchat::commandf", XS_Xchat_commandf, "Xchat"); */
	newXS("Xchat::find_context", XS_Xchat_find_context, "Xchat");
	newXS("Xchat::get_context", XS_Xchat_get_context, "Xchat");
	newXS("Xchat::set_context", XS_Xchat_set_context, "Xchat");
	newXS("Xchat::get_info", XS_Xchat_get_info, "Xchat");
	newXS("Xchat::get_prefs", XS_Xchat_get_prefs, "Xchat");
	newXS("Xchat::emit_print", XS_Xchat_emit_print, "Xchat");
	newXS("Xchat::nickcmp", XS_Xchat_nickcmp, "Xchat");
	newXS("Xchat::get_list", XS_Xchat_get_list, "Xchat");
	
#ifdef OLD_PERL
	/* for old interface compatibility */
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
	newXS ("IRC::command_with_channel", XS_IRC_command_with_channel, "IRC");
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
#endif
	stash = get_hv ("Xchat::", FALSE);
	newCONSTSUB (stash, "PRI_HIGHEST", newSViv (XCHAT_PRI_HIGHEST));
	newCONSTSUB (stash, "PRI_HIGH", newSViv (XCHAT_PRI_HIGH));
	newCONSTSUB (stash, "PRI_NORM", newSViv (XCHAT_PRI_NORM));
	newCONSTSUB (stash, "PRI_LOW", newSViv (XCHAT_PRI_LOW));
	newCONSTSUB (stash, "PRI_LOWEST", newSViv (XCHAT_PRI_LOWEST));
	
	newCONSTSUB (stash, "EAT_NONE", newSViv (XCHAT_EAT_NONE));
	newCONSTSUB (stash, "EAT_XCHAT", newSViv (XCHAT_EAT_XCHAT));
	newCONSTSUB (stash, "EAT_PLUGIN", newSViv (XCHAT_EAT_PLUGIN));
	newCONSTSUB (stash, "EAT_ALL", newSViv (XCHAT_EAT_ALL));

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

"use File::Basename();\n"
"\n"
"sub Xchat::hook_server {\n"
"  return undef unless @_ >= 2;\n"
"  \n"
"  my $message = shift;\n"
"  my $callback = shift;\n"
"  my $options = shift;\n"
"  \n"
"  my ($priority, $data) = ( Xchat::PRI_NORM, undef );\n"
"  \n"
"  if ( ref( $options ) eq 'HASH' ) {\n"
"    if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {\n"
"      $priority = $options->{priority};\n"
"    }\n"
"    if ( exists( $options->{data} ) && defined( $options->{data} ) ) {\n"
"      $data = $options->{data};\n"
"    }\n"
"  }\n"
"\n"
"  return Xchat::_hook_server( $message, $priority, $callback, $data);\n"
"\n"
"}\n"
"\n"
"sub Xchat::hook_command {\n"
"  return undef unless @_ >= 2;\n"
"\n"
"  my $command = shift;\n"
"  my $callback = shift;\n"
"  my $options = shift;\n"
"\n"
"  my ($priority, $help_text, $data) = ( Xchat::PRI_NORM, '', undef );\n"
"\n"
"  if ( ref( $options ) eq 'HASH' ) {\n"
"    if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {\n"
"      $priority = $options->{priority};\n"
"    }\n"
"    if ( exists( $options->{help_text} ) && defined( $options->{help_text} ) ) {\n"
"      $help_text = $options->{help_text};\n"
"    }\n"
"    if ( exists( $options->{data} ) && defined( $options->{data} ) ) {\n"
"      $data = $options->{data};\n"
"    }\n"
"  }\n"
"\n"
"  return Xchat::_hook_command( $command, $priority, $callback, $help_text,\n"
"			       $data);\n"
"\n"
"}\n"
"\n"
"sub Xchat::hook_print {\n"
"  return undef unless @_ >= 2;\n"
"\n"
"  my $event = shift;\n"
"  my $callback = shift;\n"
"  my $options = shift;\n"
"\n"
"  my ($priority, $data) = ( Xchat::PRI_NORM, undef );\n"
"\n"
"  if ( ref( $options ) eq 'HASH' ) {\n"
"    if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {\n"
"      $priority = $options->{priority};\n"
"    }\n"
"    if ( exists( $options->{data} ) && defined( $options->{data} ) ) {\n"
"      $data = $options->{data};\n"
"    }\n"
"  }\n"
"\n"
"  return Xchat::_hook_print( $event, $priority, $callback, $data);\n"
"\n"
"}\n"
"\n"
"\n"
"sub Xchat::hook_timer {\n"
"  return undef unless @_ >= 2;\n"
"\n"
"  my $timeout = shift;\n"
"  my $callback = shift;\n"
"  my $data = shift;\n"
"\n"
"  if( ref( $data ) eq 'HASH' && exists( $data->{data} )\n"
"      && defined( $data->{data} ) ) {\n"
"    $data = $data->{data};\n"
"  }\n"
"\n"
"  return Xchat::_hook_timer( $timeout, $callback, $data );\n"
"\n"
"}\n"
"\n"
"sub Xchat::print {\n"
"\n"
"  my $text = shift;\n"
"  if( ref( $text ) eq 'ARRAY' ) {\n"
"    if( $, ) {\n"
"      $text = join $, , @$text;\n"
"    } else {\n"
"      $text = join \"\", @$text;\n"
"    }\n"
"  }\n"
"\n"
"  \n"
"  if( @_ >= 1 ) {\n"
"    my ($channel, $server) = @_;\n"
"    my $old_ctx = Xchat::get_context();\n"
"    my $ctx = Xchat::find_context( $channel, $server );\n"
"    \n"
"    if( $ctx ) {\n"
"      Xchat::set_context( $ctx );\n"
"      Xchat::_print( $text );\n"
"      Xchat::set_context( $old_ctx );\n"
"      return 1;\n"
"    } else {\n"
"      return 0;\n"
"    }\n"
"  } else {\n"
"    Xchat::_print( $text );\n"
"    return 1;\n"
"  }\n"
"\n"
"}\n"
"\n"
"sub Xchat::printf {\n"
"  my $format = shift;\n"
"  Xchat::print( sprintf( $format, @_ ) );\n"
"}\n"
"\n"
"sub Xchat::command {\n"
"\n"
"  my $command = shift;\n"
"\n"
"  if( @_ >= 1 ) {\n"
"    my ($channel, $server) = @_;\n"
"    my $old_ctx = Xchat::get_context();\n"
"    my $ctx = Xchat::find_context( $channel, $server );\n"
"\n"
"    if( $ctx ) {\n"
"      Xchat::set_context( $ctx );\n"
"      Xchat::_command( $command );\n"
"      Xchat::set_context( $old_ctx );\n"
"      return 1;\n"
"    } else {\n"
"      return 0;\n"
"    }\n"
"  } else {\n"
"    Xchat::_command( $command );\n"
"    return 1;\n"
"  }\n"
"\n"
"}\n"
"\n"
"sub Xchat::commandf {\n"
"  my $format = shift;\n"
"  Xchat::command( sprintf( $format, @_ ) );\n"
"}\n"
"\n"
"\n"
"$SIG{__WARN__} = sub {\n"
"  local $, = \"\n\";\n"
"  Xchat::print( @_ );\n"
"};\n"
"\n"
"sub load {\n"
"  my $file = shift;\n"
"\n"
"  if( open FH, $file ) {\n"
"	 my $data = do {local $/; <FH>};\n"
"	 close FH;\n"
"\n"
"	 eval $data;\n"
"\n"
"	 if( $@ ) {\n"
"		# something went wrong\n"
"		Xchat::print( \"Error loading '$file':\n$@\n\" );\n"
"		return 1;\n"
"	 }\n"
"\n"
"  } else {\n"
"\n"
"	 Xchat::print( \"Error opening '$file': $!\n\" );\n"
"	 return 2;\n"
"  }\n"
"\n"
"  return 0;\n"
"}\n"

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

	if (my_perl == NULL)
		perl_init ();

	return execute_perl (newSVpvn ("load", 11), script_name);
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

#ifdef OLD_PERL
	while (compat_hook_list)
	{
		tmp = xchat_unhook (ph, compat_hook_list->data);
		if (tmp)
			free(tmp);
		compat_hook_list = g_slist_remove (compat_hook_list, compat_hook_list->data);
	}
#endif
}

static int
perl_command_unloadall (char *word[], char *word_eol[], void *userdata)
{
	perl_end ();

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
		/* if only unloading was possible with this shitty interface */
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

	*plugin_name = "Perl";
	*plugin_version = VERSION;
	*plugin_desc = "Perl scripting interface";

	xchat_hook_command (ph, "load", XCHAT_PRI_NORM, perl_command_load, 0, 0);
	xchat_hook_command (ph, "unload", XCHAT_PRI_NORM, perl_command_unload, 0, 0);
	xchat_hook_command (ph, "unloadall", XCHAT_PRI_NORM, perl_command_unloadall, 0, 0);

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
