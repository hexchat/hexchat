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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef ENABLE_NLS
#include <locale.h>
#endif
#ifdef WIN32
#include <windows.h>
#include <stdbool.h>
#else
#include <dirent.h>
#endif

#include <glib.h>

#undef PACKAGE

#include "hexchat-plugin.h"

static hexchat_plugin *ph;		  /* plugin handle */

static int perl_load_file (char *script_name);

#ifdef WIN32
/* STRINGIFY is from perl's CORE/config.h */
#ifndef PERL_REQUIRED_VERSION
	#define PERL_REQUIRED_VERSION STRINGIFY(PERL_REVISION) "." STRINGIFY(PERL_VERSION)
#endif

#ifndef PERL_DLL
	#define PERL_DLL "perl" STRINGIFY(PERL_REVISION) STRINGIFY(PERL_VERSION) ".dll"
#endif

static DWORD
child (char *str)
{
	MessageBoxA (0, str, "Perl DLL Error",
					 MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
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

#ifdef WIN32
static void
perl_auto_load_from_path (const char *path)
{
	char *search_path = g_build_filename (path, "*.pl", NULL);
	WIN32_FIND_DATAA find_data;
	HANDLE find_handle = FindFirstFileA (search_path, &find_data);

	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && (find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
			{
				char *full_path = g_build_filename (path, find_data.cFileName, NULL);
				perl_load_file (full_path);
				g_free (full_path);
			}
		}
		while (FindNextFileA (find_handle, &find_data) != 0);
		FindClose (find_handle);
	}

	g_free (search_path);
}
#else
static void
perl_auto_load_from_path (const char *path)
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir (path);
	if (dir)
	{
		while ((ent = readdir (dir)))
		{
			int len = strlen (ent->d_name);
			if (len > 3 && strcasecmp (".pl", ent->d_name + len - 3) == 0)
			{
				char *file = g_build_filename (path, ent->d_name, NULL);
				perl_load_file (file);
				g_free (file);
			}
		}
		closedir (dir);
	}
}
#endif

static int
perl_auto_load (void *unused)
{
	const char *xdir;
	char *sub_dir;
#ifdef WIN32
	int copied = 0;
	char *slash = NULL;
#endif

	/* get the dir in local filesystem encoding (what opendir() expects!) */
	xdir = hexchat_get_info (ph, "configdir");

	/* don't pollute the filesystem with script files, this only causes misuse of the folders
	 * only use ~/.config/hexchat/addons/ and %APPDATA%\HexChat\addons */
	sub_dir = g_build_filename (xdir, "addons", NULL);
	perl_auto_load_from_path (sub_dir);
	g_free (sub_dir);

	return 0;
}

#include <EXTERN.h>
#define WIN32IOP_H
#include <perl.h>
#include <XSUB.h>

typedef struct
{
	SV *callback;
	SV *userdata;
	hexchat_hook *hook;   /* required for timers */
	hexchat_context *ctx; /* allow timers to remember their context */
	SV *package;      /* need to track the package name when removing hooks
	                       by returning REMOVE
							   */
	unsigned int depth;
} HookData;

static PerlInterpreter *my_perl = NULL;
extern void boot_DynaLoader (pTHX_ CV * cv);

/*
  this is used for autoload and shutdown callbacks
*/
static int
execute_perl (SV * function, char *args)
{

	int count, ret_value = 1;

	dSP;
	ENTER;
	SAVETMPS;

	PUSHMARK (SP);
	XPUSHs (sv_2mortal (newSVpv (args, 0)));
	PUTBACK;

	count = call_sv (function, G_EVAL | G_SCALAR);
	SPAGAIN;
	if (SvTRUE (ERRSV)) {
		hexchat_printf(ph, "Perl error: %s\n", SvPV_nolen (ERRSV));
		if (!SvOK (POPs)) {}		/* remove undef from the top of the stack */
	} else if (count != 1) {
		hexchat_printf (ph, "Perl error: expected 1 value from %s, "
						  "got: %d\n", SvPV_nolen (function), count);
	} else {
		ret_value = POPi;
	}
	PUTBACK;
	FREETMPS;
	LEAVE;

	return ret_value;
}

static char *
get_filename (char *word[], char *word_eol[])
{
	int len;
	char *file;

	/* if called as /load "filename.pl" the only difference between word and
	 * word_eol will be the two quotes
	 */
	
	if (strchr (word[2], ' ') != NULL
		|| (strlen (word_eol[2]) - strlen(word[2])) == 2 )
	{
		file = word[2];
	} else {
		file = word_eol[2];
	}

	len = strlen (file);

	if (len > 3 && strncasecmp (".pl", file + len - 3, 3) == 0) {
		return file;
	}

	return NULL;
}

static SV *
list_item_to_sv ( hexchat_list *list, const char *const *fields )
{
	HV *hash = newHV();
	SV *field_value;
	const char *field;
	int field_index = 0;
	const char *field_name;
	int name_len;

	while (fields[field_index] != NULL) {
		field_name = fields[field_index] + 1;
		name_len = strlen (field_name);

		switch (fields[field_index][0]) {
		case 's':
			field = hexchat_list_str (ph, list, field_name);
			if (field != NULL) {
				field_value = newSVpvn (field, strlen (field));
			} else {
				field_value = &PL_sv_undef;
			}
			break;
		case 'p':
			field_value = newSViv (PTR2IV (hexchat_list_str (ph, list,
																	 field_name)));
			break;
		case 'i':
			field_value = newSVuv (hexchat_list_int (ph, list, field_name));
			break;
		case 't':
			/* From perldoc for Perl's own timelocal() and timegm():
			 * <quote>
			 * On perl versions older than 5.12.0, the range of dates that can be actually be handled depends on the size of time_t (usually a signed integer) on the given platform.
			 * As of version 5.12.0, perl has stopped using the underlying time library of the operating system it's running on and has its own implementation of those routines with a
			 * safe range of at least +/ 2**52 (about 142 million years).
			 * </quote>
			 *
			 * This is further confirmed from looking at the source for Time::Local - it's a Perl module and the implementations of timelocal() and timegm() use simple addition and
			 * subtraction of numbers. Perl automatically promotes numbers from int32_t (IV) to uint32_t (UV) to 64-bit IEEE754 double (NV) as required.
			 *
			 * This means that using a double (NV) for our own time_t suffers from the same assumptions that Perl's own functions do.
			 */
			field_value = newSVnv ((const NV) hexchat_list_time (ph, list, field_name));
			break;
		default:
			field_value = &PL_sv_undef;
		}
		(void)hv_store (hash, field_name, name_len, field_value, 0);
		field_index++;
	}
	return sv_2mortal (newRV_noinc ((SV *) hash));
}

static AV *
array2av (char *array[])
{
	int count = 0;
	SV *temp = NULL;
	AV *av = newAV();
	sv_2mortal ((SV *)av);

	for (
		count = 1;
		count < 32 && array[count] != NULL && array[count][0] != 0;
		count++
	) {
		temp = newSVpv (array[count], 0);
		SvUTF8_on (temp);
		av_push (av, temp);
	}

	return av;
}

/* sets $HexChat::Embed::current_package */
static void
set_current_package (SV *package)
{
	SV *current_package = get_sv ("HexChat::Embed::current_package", 1);
	SvSetSV_nosteal (current_package, package);
}

static int
fd_cb (int fd, int flags, void *userdata)
{
	HookData *data = (HookData *) userdata;
	int retVal = 0;
	int count = 0;

	dSP;
	ENTER;
	SAVETMPS;

	PUSHMARK (SP);
	XPUSHs (data->userdata);
	PUTBACK;

	set_current_package (data->package);
	count = call_sv (data->callback, G_EVAL);
	set_current_package (&PL_sv_undef);
	SPAGAIN;

	if (SvTRUE (ERRSV)) {
		hexchat_printf (ph, "Error in fd callback %s", SvPV_nolen (ERRSV));
		if (!SvOK (POPs)) {}		  /* remove undef from the top of the stack */
		retVal = HEXCHAT_EAT_ALL;
	} else {
		if (count != 1) {
			hexchat_print (ph, "Fd handler should only return 1 value.");
			retVal = HEXCHAT_EAT_NONE;
		} else {
			retVal = POPi;
			if (retVal == 0) {
				/* if 0 is returned, the fd is going to get unhooked */
				PUSHMARK (SP);
				XPUSHs (sv_2mortal (newSViv (PTR2IV (data->hook))));
				PUTBACK;

				call_pv ("HexChat::unhook", G_EVAL);
				SPAGAIN;

				SvREFCNT_dec (data->callback);

				if (data->userdata) {
					SvREFCNT_dec (data->userdata);
				}
				g_free (data);
			}
		}

	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return retVal;
}

static int
timer_cb (void *userdata)
{
	HookData *data = (HookData *) userdata;
	int retVal = 0;
	int count = 0;

	dSP;
	ENTER;
	SAVETMPS;

	PUSHMARK (SP);
	XPUSHs (data->userdata);
	PUTBACK;

	if (data->ctx) {
		hexchat_set_context (ph, data->ctx);
	}

	set_current_package (data->package);
	count = call_sv (data->callback, G_EVAL | G_KEEPERR);
	set_current_package (&PL_sv_undef);
	SPAGAIN;

	if (SvTRUE (ERRSV)) {
		hexchat_printf (ph, "Error in timer callback %s", SvPV_nolen (ERRSV));
		if (!SvOK (POPs)) {}		  /* remove undef from the top of the stack */
		retVal = HEXCHAT_EAT_ALL;
	} else {
		if (count != 1) {
			hexchat_print (ph, "Timer handler should only return 1 value.");
			retVal = HEXCHAT_EAT_NONE;
		} else {
			retVal = POPi;
			if (retVal == 0) {
				/* if 0 is return the timer is going to get unhooked */
				PUSHMARK (SP);
				XPUSHs (sv_2mortal (newSViv (PTR2IV (data->hook))));
				XPUSHs (sv_mortalcopy (data->package));
				PUTBACK;

				call_pv ("HexChat::unhook", G_EVAL);
				SPAGAIN;
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
	HookData *data = (HookData *) userdata;
	int retVal = 0;
	int count = 0;

	dSP;
	ENTER;
	SAVETMPS;

	if (data->depth)
		return HEXCHAT_EAT_NONE;

	/*               hexchat_printf (ph, */
	/*                               "Received %d words in server callback", av_len (wd)); */
	PUSHMARK (SP);
	XPUSHs (newRV_noinc ((SV *) array2av (word)));
	XPUSHs (newRV_noinc ((SV *) array2av (word_eol)));
	XPUSHs (data->userdata);
	PUTBACK;

	data->depth++;
	set_current_package (data->package);
	count = call_sv (data->callback, G_EVAL | G_KEEPERR);
	set_current_package (&PL_sv_undef);
	data->depth--;
	SPAGAIN;
	if (SvTRUE (ERRSV)) {
		hexchat_printf (ph, "Error in server callback %s", SvPV_nolen (ERRSV));
		if (!SvOK (POPs)) {}		  /* remove undef from the top of the stack */
		retVal = HEXCHAT_EAT_NONE;
	} else {
		if (count != 1) {
			hexchat_print (ph, "Server handler should only return 1 value.");
			retVal = HEXCHAT_EAT_NONE;
		} else {
			retVal = POPi;
		}

	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return retVal;
}

static int
command_cb (char *word[], char *word_eol[], void *userdata)
{
	HookData *data = (HookData *) userdata;
	int retVal = 0;
	int count = 0;

	dSP;
	ENTER;
	SAVETMPS;
	
	if (data->depth)
		return HEXCHAT_EAT_NONE;

	/*               hexchat_printf (ph, "Received %d words in command callback", */
	/*                               av_len (wd)); */
	PUSHMARK (SP);
	XPUSHs (newRV_noinc ((SV *) array2av (word)));
	XPUSHs (newRV_noinc ((SV *) array2av (word_eol)));
	XPUSHs (data->userdata);
	PUTBACK;

	data->depth++;
	set_current_package (data->package);
	count = call_sv (data->callback, G_EVAL | G_KEEPERR);
	set_current_package (&PL_sv_undef);
	data->depth--;
	SPAGAIN;
	if (SvTRUE (ERRSV)) {
		hexchat_printf (ph, "Error in command callback %s", SvPV_nolen (ERRSV));
		if (!SvOK (POPs)) {}		  /* remove undef from the top of the stack */
		retVal = HEXCHAT_EAT_HEXCHAT;
	} else {
		if (count != 1) {
			hexchat_print (ph, "Command handler should only return 1 value.");
			retVal = HEXCHAT_EAT_NONE;
		} else {
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

	HookData *data = (HookData *) userdata;
	SV *temp = NULL;
	int retVal = 0;
	int count = 1;
	int last_index = 31;
	/* must be initialized after SAVETMPS */
	AV *wd = NULL;

	dSP;
	ENTER;
	SAVETMPS;

	if (data->depth)
		return HEXCHAT_EAT_NONE;

	wd = newAV ();
	sv_2mortal ((SV *) wd);

	/* need to scan backwards to find the index of the last element since some
	   events such as "DCC Timeout" can have NULL elements in between non NULL
	   elements */

	while (last_index >= 0
			 && (word[last_index] == NULL || word[last_index][0] == 0)) {
		last_index--;
	}

	for (count = 1; count <= last_index; count++) {
		if (word[count] == NULL) {
			av_push (wd, &PL_sv_undef);
		} else if (word[count][0] == 0) {
			av_push (wd, newSVpvn ("",0));	
		} else {
			temp = newSVpv (word[count], 0);
			SvUTF8_on (temp);
			av_push (wd, temp);
		}
	}

	/*hexchat_printf (ph, "Received %d words in print callback", av_len (wd)+1); */
	PUSHMARK (SP);
	XPUSHs (newRV_noinc ((SV *) wd));
	XPUSHs (data->userdata);
	PUTBACK;

	data->depth++;
	set_current_package (data->package);
	count = call_sv (data->callback, G_EVAL | G_KEEPERR);
	set_current_package (&PL_sv_undef);
	data->depth--;
	SPAGAIN;
	if (SvTRUE (ERRSV)) {
		hexchat_printf (ph, "Error in print callback %s", SvPV_nolen (ERRSV));
		if (!SvOK (POPs)) {}		  /* remove undef from the top of the stack */
		retVal = HEXCHAT_EAT_NONE;
	} else {
		if (count != 1) {
			hexchat_print (ph, "Print handler should only return 1 value.");
			retVal = HEXCHAT_EAT_NONE;
		} else {
			retVal = POPi;
		}

	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return retVal;
}

/* custom IRC perl functions for scripting */

/* HexChat::Internal::register (scriptname, version, desc, shutdowncallback, filename)
 *
 */

static
XS (XS_HexChat_register)
{
	char *name, *version, *desc, *filename;
	void *gui_entry;
	dXSARGS;
	if (items != 4) {
		hexchat_printf (ph,
						  "Usage: HexChat::Internal::register(scriptname, version, desc, filename)");
	} else {
		name = SvPV_nolen (ST (0));
		version = SvPV_nolen (ST (1));
		desc = SvPV_nolen (ST (2));
		filename = SvPV_nolen (ST (3));

		gui_entry = hexchat_plugingui_add (ph, filename, name,
													desc, version, NULL);

		XSRETURN_IV (PTR2IV (gui_entry));

	}
}


/* HexChat::print(output) */
static
XS (XS_HexChat_print)
{

	char *text = NULL;

	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::Internal::print(text)");
	} else {
		text = SvPV_nolen (ST (0));
		hexchat_print (ph, text);
	}
	XSRETURN_EMPTY;
}

static
XS (XS_HexChat_emit_print)
{
	char *event_name;
	int RETVAL;
	int count;

	dXSARGS;
	if (items < 1) {
		hexchat_print (ph, "Usage: HexChat::emit_print(event_name, ...)");
	} else {
		event_name = (char *) SvPV_nolen (ST (0));
		RETVAL = 0;

		/* we need to figure out the number of defined values passed in */
		for (count = 0; count < items; count++) {
			if (!SvOK (ST (count))) {
				break;
			}
		}

		switch (count) {
		case 1:
			RETVAL = hexchat_emit_print (ph, event_name, NULL);
			break;
		case 2:
			RETVAL = hexchat_emit_print (ph, event_name,
												SvPV_nolen (ST (1)), NULL);
			break;
		case 3:
			RETVAL = hexchat_emit_print (ph, event_name,
												SvPV_nolen (ST (1)),
												SvPV_nolen (ST (2)), NULL);
			break;
		case 4:
			RETVAL = hexchat_emit_print (ph, event_name,
												SvPV_nolen (ST (1)),
												SvPV_nolen (ST (2)),
												SvPV_nolen (ST (3)), NULL);
			break;
		case 5:
			RETVAL = hexchat_emit_print (ph, event_name,
												SvPV_nolen (ST (1)),
												SvPV_nolen (ST (2)),
												SvPV_nolen (ST (3)),
												SvPV_nolen (ST (4)), NULL);
			break;

		}

		XSRETURN_IV (RETVAL);
	}
}

static
XS (XS_HexChat_send_modes)
{
	AV *p_targets = NULL;
	int modes_per_line = 0;
	char sign;
	char mode;
	int i = 0;
	const char **targets;
	int target_count = 0;
	SV **elem;

	dXSARGS;
	if (items < 3 || items > 4) {
		hexchat_print (ph,
			"Usage: HexChat::send_modes( targets, sign, mode, modes_per_line)"
		);
	} else {
		if (SvROK (ST (0))) {
			p_targets = (AV*) SvRV (ST (0));
			target_count = av_len (p_targets) + 1;
			targets = g_new (const char *, target_count);
			for (i = 0; i < target_count; i++ ) {
				elem = av_fetch (p_targets, i, 0);

				if (elem != NULL) {
					targets[i] = SvPV_nolen (*elem);
				} else {
					targets[i] = "";
				}
			}
		} else{
			targets = g_new (const char *, 1);
			targets[0] = SvPV_nolen (ST (0));
			target_count = 1;
		}
		
		if (target_count == 0) {
			g_free ((char**) targets);
			XSRETURN_EMPTY;
		}

		sign = (SvPV_nolen (ST (1)))[0];
		mode = (SvPV_nolen (ST (2)))[0];

		if (items == 4 ) {
			modes_per_line = (int) SvIV (ST (3)); 
		}

		hexchat_send_modes (ph, targets, target_count, modes_per_line, sign, mode);
		g_free ((char**) targets);
	}
}
static
XS (XS_HexChat_get_info)
{
	SV *temp = NULL;
	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::get_info(id)");
	} else {
		SV *id = ST (0);
		const char *RETVAL;

		RETVAL = hexchat_get_info (ph, SvPV_nolen (id));
		if (RETVAL == NULL) {
			XSRETURN_UNDEF;
		}

		if (!strncmp ("win_ptr", SvPV_nolen (id), 7)
			|| !strncmp ("gtkwin_ptr", SvPV_nolen (id), 10))
		{
			XSRETURN_IV (PTR2IV (RETVAL));
		} else {
			
			if (
				!strncmp ("libdirfs", SvPV_nolen (id), 8) ||
				!strncmp ("xchatdirfs", SvPV_nolen (id), 10) ||
				!strncmp ("configdir", SvPV_nolen (id), 9)
			) {
				XSRETURN_PV (RETVAL);
			} else {
				temp = newSVpv (RETVAL, 0);
				SvUTF8_on (temp);
				PUSHMARK (SP);
				XPUSHs (sv_2mortal (temp));
				PUTBACK;
			}
		}
	}
}

static
XS (XS_HexChat_context_info)
{
	const char *const *fields;
	dXSARGS;

	if (items > 0 ) {
		hexchat_print (ph, "Usage: HexChat::Internal::context_info()");
	}
	fields = hexchat_list_fields (ph, "channels" );
	XPUSHs (list_item_to_sv (NULL, fields));
	XSRETURN (1);
}

static
XS (XS_HexChat_get_prefs)
{
	const char *str;
	int integer;
	SV *temp = NULL;
	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::get_prefs(name)");
	} else {


		switch (hexchat_get_prefs (ph, SvPV_nolen (ST (0)), &str, &integer)) {
		case 0:
			XSRETURN_UNDEF;
			break;
		case 1:
			temp = newSVpv (str, 0);
			SvUTF8_on (temp);
			SP -= items;
			sp = mark;
			XPUSHs (sv_2mortal (temp));
			PUTBACK;
			break;
		case 2:
			XSRETURN_IV (integer);
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

/* HexChat::Internal::hook_server(name, priority, callback, userdata) */
static
XS (XS_HexChat_hook_server)
{

	char *name;
	int pri;
	SV *callback;
	SV *userdata;
	SV *package;
	hexchat_hook *hook;
	HookData *data;

	dXSARGS;

	if (items != 5) {
		hexchat_print (ph,
						 "Usage: HexChat::Internal::hook_server(name, priority, callback, userdata, package)");
	} else {
		name = SvPV_nolen (ST (0));
		pri = (int) SvIV (ST (1));
		callback = ST (2);
		userdata = ST (3);
		package = ST (4);
		data = NULL;
		data = g_new (HookData, 1);
		data->callback = newSVsv (callback);
		data->userdata = newSVsv (userdata);
		data->depth = 0;
		data->package = newSVsv (package);

		hook = hexchat_hook_server (ph, name, pri, server_cb, data);

		XSRETURN_IV (PTR2IV (hook));
	}
}

/* HexChat::Internal::hook_command(name, priority, callback, help_text, userdata) */
static
XS (XS_HexChat_hook_command)
{
	char *name;
	int pri;
	SV *callback;
	char *help_text = NULL;
	SV *userdata;
	SV *package;
	hexchat_hook *hook;
	HookData *data;

	dXSARGS;

	if (items != 6) {
		hexchat_print (ph,
						 "Usage: HexChat::Internal::hook_command(name, priority, callback, help_text, userdata, package)");
	} else {
		name = SvPV_nolen (ST (0));
		pri = (int) SvIV (ST (1));
		callback = ST (2);

		/* leave the help text as NULL if the help text is undefined to avoid
		 * overriding the default help message for builtin commands */
		if (SvOK(ST (3))) {
			help_text = SvPV_nolen (ST (3));
		}

		userdata = ST (4);
		package = ST (5);
		data = NULL;

		data = g_new (HookData, 1);
		data->callback = newSVsv (callback);
		data->userdata = newSVsv (userdata);
		data->depth = 0;
		data->package = newSVsv (package);
		hook = hexchat_hook_command (ph, name, pri, command_cb, help_text, data);

		XSRETURN_IV (PTR2IV (hook));
	}

}

/* HexChat::Internal::hook_print(name, priority, callback, [userdata]) */
static
XS (XS_HexChat_hook_print)
{

	char *name;
	int pri;
	SV *callback;
	SV *userdata;
	SV *package;
	hexchat_hook *hook;
	HookData *data;
	dXSARGS;
	if (items != 5) {
		hexchat_print (ph,
						 "Usage: HexChat::Internal::hook_print(name, priority, callback, userdata, package)");
	} else {
		name = SvPV_nolen (ST (0));
		pri = (int) SvIV (ST (1));
		callback = ST (2);
		data = NULL;
		userdata = ST (3);
		package = ST (4);

		data = g_new (HookData, 1);
		data->callback = newSVsv (callback);
		data->userdata = newSVsv (userdata);
		data->depth = 0;
		data->package = newSVsv (package);
		hook = hexchat_hook_print (ph, name, pri, print_cb, data);

		XSRETURN_IV (PTR2IV (hook));
	}
}

/* HexChat::Internal::hook_timer(timeout, callback, userdata) */
static
XS (XS_HexChat_hook_timer)
{
	int timeout;
	SV *callback;
	SV *userdata;
	hexchat_hook *hook;
	SV *package;
	HookData *data;

	dXSARGS;

	if (items != 4) {
		hexchat_print (ph,
						 "Usage: HexChat::Internal::hook_timer(timeout, callback, userdata, package)");
	} else {
		timeout = (int) SvIV (ST (0));
		callback = ST (1);
		data = NULL;
		userdata = ST (2);
		package = ST (3);

		data = g_new (HookData, 1);
		data->callback = newSVsv (callback);
		data->userdata = newSVsv (userdata);
		data->ctx = hexchat_get_context (ph);
		data->package = newSVsv (package);
		hook = hexchat_hook_timer (ph, timeout, timer_cb, data);
		data->hook = hook;

		XSRETURN_IV (PTR2IV (hook));
	}
}

/* HexChat::Internal::hook_fd(fd, callback, flags, userdata) */
static
XS (XS_HexChat_hook_fd)
{
	int fd;
	SV *callback;
	int flags;
	SV *userdata;
	SV *package;
	hexchat_hook *hook;
	HookData *data;

	dXSARGS;

	if (items != 5) {
		hexchat_print (ph,
						 "Usage: HexChat::Internal::hook_fd(fd, callback, flags, userdata)");
	} else {
		fd = (int) SvIV (ST (0));
		callback = ST (1);
		flags = (int) SvIV (ST (2));
		userdata = ST (3);
		package = ST (4);
		data = NULL;

#ifdef WIN32
		if ((flags & HEXCHAT_FD_NOTSOCKET) == 0) {
			/* this _get_osfhandle if from win32iop.h in the perl distribution,
			 *  not the one provided by Windows
			 */ 
			fd = _get_osfhandle(fd);
			if (fd < 0) {
				hexchat_print(ph, "Invalid file descriptor");
				XSRETURN_UNDEF;
			}
		}
#endif

		data = g_new (HookData, 1);
		data->callback = newSVsv (callback);
		data->userdata = newSVsv (userdata);
		data->depth = 0;
		data->package = newSVsv (package);
		hook = hexchat_hook_fd (ph, fd, flags, fd_cb, data);
		data->hook = hook;

		XSRETURN_IV (PTR2IV (hook));
	}
}

static
XS (XS_HexChat_unhook)
{
	hexchat_hook *hook;
	HookData *userdata;
	int retCount = 0;
	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::unhook(hook)");
	} else {
		hook = INT2PTR (hexchat_hook *, SvUV (ST (0)));
		userdata = (HookData *) hexchat_unhook (ph, hook);

		if (userdata != NULL) {
			if (userdata->callback != NULL) {
				SvREFCNT_dec (userdata->callback);
			}

			if (userdata->userdata != NULL) {
				XPUSHs (sv_mortalcopy (userdata->userdata));
				SvREFCNT_dec (userdata->userdata);
				retCount = 1;
			}

			if (userdata->package != NULL) {
				SvREFCNT_dec (userdata->package);
			}

			g_free (userdata);
		}
		XSRETURN (retCount);
	}
	XSRETURN_EMPTY;
}

/* HexChat::Internal::command(command) */
static
XS (XS_HexChat_command)
{
	char *cmd = NULL;

	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::Internal::command(command)");
	} else {
		cmd = SvPV_nolen (ST (0));
		hexchat_command (ph, cmd);

	}
	XSRETURN_EMPTY;
}

static
XS (XS_HexChat_find_context)
{
	char *server = NULL;
	char *chan = NULL;
	hexchat_context *RETVAL;

	dXSARGS;
	if (items > 2)
		hexchat_print (ph, "Usage: HexChat::find_context ([channel, [server]])");
	{

		switch (items) {
		case 0:						  /* no server name and no channel name */
			/* nothing to do, server and chan are already NULL */
			break;
		case 1:						  /* channel name only */
			/* change channel value only if it is true or 0 */
			/* otherwise leave it as null */
			if (SvTRUE (ST (0)) || SvNIOK (ST (0))) {
				chan = SvPV_nolen (ST (0));
				/*                               hexchat_printf( ph, "XSUB - find_context( %s, NULL )", chan ); */
			}
			/* else { hexchat_print( ph, "XSUB - find_context( NULL, NULL )" ); } */
			/* chan is already NULL */
			break;
		case 2:						  /* server and channel */
			/* change channel value only if it is true or 0 */
			/* otherwise leave it as NULL */
			if (SvTRUE (ST (0)) || SvNIOK (ST (0))) {
				chan = SvPV_nolen (ST (0));
				/*                               hexchat_printf( ph, "XSUB - find_context( %s, NULL )", SvPV_nolen(ST(0) )); */
			}

			/* else { hexchat_print( ph, "XSUB - 2 arg NULL chan" ); } */
			/* change server value only if it is true or 0 */
			/* otherwise leave it as NULL */
			if (SvTRUE (ST (1)) || SvNIOK (ST (1))) {
				server = SvPV_nolen (ST (1));
				/*                               hexchat_printf( ph, "XSUB - find_context( NULL, %s )", SvPV_nolen(ST(1) )); */
			}
			/*  else { hexchat_print( ph, "XSUB - 2 arg NULL server" ); } */
			break;
		}

		RETVAL = hexchat_find_context (ph, server, chan);
		if (RETVAL != NULL) {
			/*                      hexchat_print (ph, "XSUB - context found"); */
			XSRETURN_IV (PTR2IV (RETVAL));
		} else {
			/*           hexchat_print (ph, "XSUB - context not found"); */
			XSRETURN_UNDEF;
		}
	}
}

static
XS (XS_HexChat_get_context)
{
	dXSARGS;
	if (items != 0) {
		hexchat_print (ph, "Usage: HexChat::get_context()");
	} else {
		XSRETURN_IV (PTR2IV (hexchat_get_context (ph)));
	}
}

static
XS (XS_HexChat_set_context)
{
	hexchat_context *ctx;
	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::set_context(ctx)");
	} else {
		ctx = INT2PTR (hexchat_context *, SvUV (ST (0)));
		XSRETURN_IV ((IV) hexchat_set_context (ph, ctx));
	}
}

static
XS (XS_HexChat_nickcmp)
{
	dXSARGS;
	if (items != 2) {
		hexchat_print (ph, "Usage: HexChat::nickcmp(s1, s2)");
	} else {
		XSRETURN_IV ((IV) hexchat_nickcmp (ph, SvPV_nolen (ST (0)),
													SvPV_nolen (ST (1))));
	}
}

static
XS (XS_HexChat_get_list)
{
	SV *name;
	hexchat_list *list;
	const char *const *fields;
	int count = 0;					  /* return value for scalar context */
	dXSARGS;

	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::get_list(name)");
	} else {
		SP -= items;				  /*remove the argument list from the stack */

		name = ST (0);

		list = hexchat_list_get (ph, SvPV_nolen (name));

		if (list == NULL) {
			XSRETURN_EMPTY;
		}

		if (GIMME_V == G_SCALAR) {
			while (hexchat_list_next (ph, list)) {
				count++;
			}
			hexchat_list_free (ph, list);
			XSRETURN_IV ((IV) count);
		}

		fields = hexchat_list_fields (ph, SvPV_nolen (name));
		while (hexchat_list_next (ph, list)) {
			XPUSHs (list_item_to_sv (list, fields));
		}
		hexchat_list_free (ph, list);

		PUTBACK;
		return;
	}
}

static
XS (XS_HexChat_Embed_plugingui_remove)
{
	void *gui_entry;
	dXSARGS;
	if (items != 1) {
		hexchat_print (ph, "Usage: HexChat::Embed::plugingui_remove(handle)");
	} else {
		gui_entry = INT2PTR (void *, SvUV (ST (0)));
		hexchat_plugingui_remove (ph, gui_entry);
	}
	XSRETURN_EMPTY;
}

static
XS (XS_HexChat_plugin_pref_set)
{
	dMARK;
	dAX;

	XSRETURN_IV ((IV) hexchat_pluginpref_set_str (ph, SvPV_nolen (ST (0)),
													SvPV_nolen (ST (1))));
}

static
XS (XS_HexChat_plugin_pref_get)
{
	int result;
	char value[512];

	dMARK;
	dAX;

	result = hexchat_pluginpref_get_str (ph, SvPV_nolen (ST (0)), value);

	if (result)
		XSRETURN_PV (value);

	XSRETURN_UNDEF;
}

static
XS (XS_HexChat_plugin_pref_delete)
{
	dMARK;
	dAX;
	
	XSRETURN_IV ((IV) hexchat_pluginpref_delete (ph, SvPV_nolen (ST (0))));
}

static
XS (XS_HexChat_plugin_pref_list)
{
	char list[4096];
	char value[512];
	char *token;

	dSP;
	dMARK;
	dAX;

	if (!hexchat_pluginpref_list (ph, list))
		XSRETURN_EMPTY;

	PUSHMARK (SP);

	token = strtok (list, ",");
	while (token != NULL)
	{
		hexchat_pluginpref_get_str (ph, token, value);

		XPUSHs (sv_2mortal (newSVpv (token, 0)));
		XPUSHs (sv_2mortal (newSVpv (value, 0)));

		token = strtok (NULL, ",");
	}

	PUTBACK;
}

/* xs_init is the second argument perl_parse. As the name hints, it
   initializes XS subroutines (see the perlembed manpage) */
static void
xs_init (pTHX)
{
	HV *stash;
	SV *version;
	/* This one allows dynamic loading of perl modules in perl
	   scripts by the 'use perlmod;' construction */
	newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
	/* load up all the custom IRC perl functions */
	newXS ("HexChat::Internal::register", XS_HexChat_register, __FILE__);
	newXS ("HexChat::Internal::hook_server", XS_HexChat_hook_server, __FILE__);
	newXS ("HexChat::Internal::hook_command", XS_HexChat_hook_command, __FILE__);
	newXS ("HexChat::Internal::hook_print", XS_HexChat_hook_print, __FILE__);
	newXS ("HexChat::Internal::hook_timer", XS_HexChat_hook_timer, __FILE__);
	newXS ("HexChat::Internal::hook_fd", XS_HexChat_hook_fd, __FILE__);
	newXS ("HexChat::Internal::unhook", XS_HexChat_unhook, __FILE__);
	newXS ("HexChat::Internal::print", XS_HexChat_print, __FILE__);
	newXS ("HexChat::Internal::command", XS_HexChat_command, __FILE__);
	newXS ("HexChat::Internal::set_context", XS_HexChat_set_context, __FILE__);
	newXS ("HexChat::Internal::get_info", XS_HexChat_get_info, __FILE__);
	newXS ("HexChat::Internal::context_info", XS_HexChat_context_info, __FILE__);
	newXS ("HexChat::Internal::get_list", XS_HexChat_get_list, __FILE__);

	newXS ("HexChat::Internal::plugin_pref_set", XS_HexChat_plugin_pref_set, __FILE__);
	newXS ("HexChat::Internal::plugin_pref_get", XS_HexChat_plugin_pref_get, __FILE__);
	newXS ("HexChat::Internal::plugin_pref_delete", XS_HexChat_plugin_pref_delete, __FILE__);
	newXS ("HexChat::Internal::plugin_pref_list", XS_HexChat_plugin_pref_list, __FILE__);
	
	newXS ("HexChat::find_context", XS_HexChat_find_context, __FILE__);
	newXS ("HexChat::get_context", XS_HexChat_get_context, __FILE__);
	newXS ("HexChat::get_prefs", XS_HexChat_get_prefs, __FILE__);
	newXS ("HexChat::emit_print", XS_HexChat_emit_print, __FILE__);
	newXS ("HexChat::send_modes", XS_HexChat_send_modes, __FILE__);
	newXS ("HexChat::nickcmp", XS_HexChat_nickcmp, __FILE__);

	newXS ("HexChat::Embed::plugingui_remove", XS_HexChat_Embed_plugingui_remove,
			 __FILE__);

	stash = get_hv ("HexChat::", TRUE);
	if (stash == NULL) {
		exit (1);
	}

	newCONSTSUB (stash, "PRI_HIGHEST", newSViv (HEXCHAT_PRI_HIGHEST));
	newCONSTSUB (stash, "PRI_HIGH", newSViv (HEXCHAT_PRI_HIGH));
	newCONSTSUB (stash, "PRI_NORM", newSViv (HEXCHAT_PRI_NORM));
	newCONSTSUB (stash, "PRI_LOW", newSViv (HEXCHAT_PRI_LOW));
	newCONSTSUB (stash, "PRI_LOWEST", newSViv (HEXCHAT_PRI_LOWEST));

	newCONSTSUB (stash, "EAT_NONE", newSViv (HEXCHAT_EAT_NONE));
	newCONSTSUB (stash, "EAT_HEXCHAT", newSViv (HEXCHAT_EAT_HEXCHAT));
	newCONSTSUB (stash, "EAT_XCHAT", newSViv (HEXCHAT_EAT_HEXCHAT)); /* for compatibility */
	newCONSTSUB (stash, "EAT_PLUGIN", newSViv (HEXCHAT_EAT_PLUGIN));
	newCONSTSUB (stash, "EAT_ALL", newSViv (HEXCHAT_EAT_ALL));
	newCONSTSUB (stash, "FD_READ", newSViv (HEXCHAT_FD_READ));
	newCONSTSUB (stash, "FD_WRITE", newSViv (HEXCHAT_FD_WRITE));
	newCONSTSUB (stash, "FD_EXCEPTION", newSViv (HEXCHAT_FD_EXCEPTION));
	newCONSTSUB (stash, "FD_NOTSOCKET", newSViv (HEXCHAT_FD_NOTSOCKET));
	newCONSTSUB (stash, "KEEP", newSViv (1));
	newCONSTSUB (stash, "REMOVE", newSViv (0));

	version = get_sv( "HexChat::VERSION", 1 );
	sv_setpv( version, PACKAGE_VERSION );
}

static void
perl_init (void)
{
	int warn;
	int arg_count;
	char *perl_args[] = { "", "-e", "0", "-w" };
	char *env[] = { "" };
	static const char xchat_definitions[] = {
		/* Redefine the $SIG{__WARN__} handler to have HexChat
		   printing warnings in the main window. (TheHobbit) */
#include "hexchat.pm.h"
	};
#ifdef OLD_PERL
	static const char irc_definitions[] = {
#include "irc.pm.h"
	};
#endif
#ifdef ENABLE_NLS

	/* Problem is, dynamicaly loaded modules check out the $]
	   var. It appears that in the embedded interpreter we get
	   5,00503 as soon as the LC_NUMERIC locale calls for a comma
	   instead of a point in separating integer and decimal
	   parts. I realy can't understant why... The following
	   appears to be an awful workaround... But it'll do until I
	   (or someone else :)) found the "right way" to solve this
	   nasty problem. (TheHobbit <thehobbit@altern.org>) */

	setlocale (LC_NUMERIC, "C");

#endif

	warn = 0;
	hexchat_get_prefs (ph, "perl_warnings", NULL, &warn);
	arg_count = warn ? 4 : 3;

	PERL_SYS_INIT3 (&arg_count, (char ***)&perl_args, (char ***)&env);
	my_perl = perl_alloc ();
	perl_construct (my_perl);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	perl_parse (my_perl, xs_init, arg_count, perl_args, (char **)NULL);

	/*
	   Now initialising the perl interpreter by loading the
	   perl_definition array.
	 */

	eval_pv (xchat_definitions, TRUE);
#ifdef OLD_PERL
	eval_pv (irc_definitions, TRUE);
#endif

}


static int
perl_load_file (char *filename)
{
#ifdef WIN32
	static HMODULE lib = NULL;

	if (!lib) {
		lib = LoadLibraryA (PERL_DLL);
		if (!lib) {
			if (GetLastError () == ERROR_BAD_EXE_FORMAT)
				/* http://forum.xchat.org/viewtopic.php?t=3277 */
				thread_mbox ("Cannot use this " PERL_DLL "\n\n"
#ifdef _WIN64
								 "64-bit HexChat Perl is required.");
#else
								 "32-bit HexChat Perl is required.");
#endif
			else {
				/* a lot of people install this old version */
				lib = LoadLibraryA ("perl56.dll");
				if (lib) {
					FreeLibrary (lib);
					lib = NULL;
					thread_mbox ("Cannot open " PERL_DLL "!\n\n"
									 "You must have a Visual C++ build of Perl "
									 PERL_REQUIRED_VERSION " installed in order to\n"
									 "run Perl scripts. A reboot may be required.\n\n"
									 "http://hexchat.github.io/downloads.html\n\n"
									 "I have found Perl 5.6, but that is too old.");
				} else {
					thread_mbox ("Cannot open " PERL_DLL "!\n\n"
									 "You must have a Visual C++ build of Perl "
									 PERL_REQUIRED_VERSION " installed in order to\n"
									 "run Perl scripts. A reboot may be required.\n\n"
									 "http://hexchat.github.io/downloads.html\n\n"
									 "Make sure Perl's bin directory is in your PATH.");
				}
			}
			/* failure */
			return FALSE;
		}

		/* success */
		FreeLibrary (lib);
	}
#endif

	if (my_perl == NULL) {
		perl_init ();
	}

	return execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::load", 0)),
								filename);

}

static void
perl_end (void)
{

	if (my_perl != NULL) {
		execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::unload_all", 0)), "");
		PL_perl_destruct_level = 1;
		perl_destruct (my_perl);
		perl_free (my_perl);
		PERL_SYS_TERM();
		my_perl = NULL;
	}

}

static int
perl_command_unloadall (char *word[], char *word_eol[], void *userdata)
{
	if (my_perl != NULL) {
		execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::unload_all", 0)), "");
		return HEXCHAT_EAT_HEXCHAT;
	}

	return HEXCHAT_EAT_HEXCHAT;
}

static int
perl_command_reloadall (char *word[], char *word_eol[], void *userdata)
{
	if (my_perl != NULL) {
		execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::reload_all", 0)), "");

		return HEXCHAT_EAT_HEXCHAT;
	} else {
		perl_auto_load( NULL );
	}
	return HEXCHAT_EAT_HEXCHAT;
}

static int
perl_command_load (char *word[], char *word_eol[], void *userdata)
{
	char *file = get_filename (word, word_eol);

	if (file != NULL )
	{
		perl_load_file (file);
		return HEXCHAT_EAT_HEXCHAT;
	}

	return HEXCHAT_EAT_NONE;
}

static int
perl_command_unload (char *word[], char *word_eol[], void *userdata)
{
	char *file = get_filename (word, word_eol);
	
	if (my_perl != NULL && file != NULL) {
		execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::unload", 0)), file);
		return HEXCHAT_EAT_HEXCHAT;
	}

	return HEXCHAT_EAT_NONE;
}

static int
perl_command_reload (char *word[], char *word_eol[], void *eat)
{
	char *file = get_filename (word, word_eol);
	
	if (my_perl != NULL && file != NULL) {
		execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::reload", 0)), file);
		return HEXCHAT_EAT_HEXCHAT;
	}
	
	if (eat)
		return HEXCHAT_EAT_HEXCHAT;
	else
		return HEXCHAT_EAT_NONE;
}

static int
perl_command_eval (char *word[], char *word_eol[], void *userdata)
{
	if (my_perl != NULL)
		execute_perl (sv_2mortal (newSVpv ("HexChat::Embed::evaluate", 0)), word_eol[2]);

	return HEXCHAT_EAT_HEXCHAT;
}

void
hexchat_plugin_get_info (char **name, char **desc, char **version,
							  void **reserved)
{
	*name = "Perl";
	*desc = "Perl scripting interface";
	*version = PACKAGE_VERSION;
	if (reserved)
		*reserved = NULL;
}


/* Reinit safeguard */

static int initialized = 0;

int
hexchat_plugin_init (hexchat_plugin * plugin_handle, char **plugin_name,
						 char **plugin_desc, char **plugin_version, char *arg)
{
	if (initialized != 0) {
		hexchat_print (plugin_handle, "Perl interface already loaded\n");
		return 0;
	}

	ph = plugin_handle;
	initialized = 1;

	*plugin_name = "Perl";
	*plugin_desc = "Perl scripting interface";
	*plugin_version = PACKAGE_VERSION;

	hexchat_hook_command (ph, "load", HEXCHAT_PRI_NORM, perl_command_load, 0, 0);
	hexchat_hook_command (ph, "unload", HEXCHAT_PRI_NORM, perl_command_unload, 0,
							  0);
	hexchat_hook_command (ph, "reload", HEXCHAT_PRI_NORM, perl_command_reload, 0,
							  0);
	hexchat_hook_command (ph, "pl_reload", HEXCHAT_PRI_NORM, perl_command_reload,
							  "Reloads a Perl script. Syntax: /pl_reload <filename.pl>", (int*)1);
	hexchat_hook_command (ph, "unloadall", HEXCHAT_PRI_NORM,
							  perl_command_unloadall, "Unloads all loaded Perl scripts.", 0);
	hexchat_hook_command (ph, "reloadall", HEXCHAT_PRI_NORM,
							  perl_command_reloadall, "Realoads all loaded Perl scripts.", 0);

	hexchat_hook_command (ph, "pl", HEXCHAT_PRI_NORM,
							  perl_command_eval, "Evaluates Perl code. Syntax: /pl <perl code>", 0);

	/*perl_init (); */
	hexchat_hook_timer (ph, 0, perl_auto_load, NULL );

	hexchat_print (ph, "Perl interface loaded\n");

	return 1;
}

int
hexchat_plugin_deinit (hexchat_plugin * plugin_handle)
{
	perl_end ();

	initialized = 0;
	hexchat_print (plugin_handle, "Perl interface unloaded\n");

	return 1;
}
