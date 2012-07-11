/* XChat-WDK
 * Copyright (c) 2011 Berke Viktor.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
