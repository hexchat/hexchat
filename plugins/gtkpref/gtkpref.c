/* XChat-WDK
 * Copyright (c) 2011 Berke Viktor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */

static void
launch_tool ()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory (&si, sizeof (si));
	si.cb = sizeof (si);
	ZeroMemory (&pi, sizeof (pi));

	if (!CreateProcess ( NULL, "gtk2-prefs.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		xchat_print (ph, "Error launching the GTK+ Preference Tool! Maybe the executable is missing?");
	}

	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "GTKPref";
	*plugin_desc = "GTK+ Preference Tool Launcher";
	*plugin_version = "1.0";

	xchat_hook_command (ph, "GTKPREF", XCHAT_PRI_NORM, launch_tool, 0, 0);
	xchat_command (ph, "MENU -ietc\\gtkpref.png ADD \"Settings/GTK+ Preferences\" \"GTKPREF\"");

	return 1;       /* return 1 for success */
}

int
xchat_plugin_deinit (void)
{
	xchat_command (ph, "MENU DEL \"Settings/GTK+ Preferences\"");

	return 1;
}
