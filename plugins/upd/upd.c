/* XChat-WDK
 * Copyright (c) 2010-2011 Berke Viktor.
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
#include <wininet.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */
static const char name[] = "Update Checker";
static const char desc[] = "Check for XChat-WDK updates automatically";
static const char version[] = "2.0";

/* we need this to store the result of the initial update check since the return value is preserved for XCHAT_EAT */
static int update_available;

static char*
check_version ()
{
	HINTERNET hINet, hFile;
	hINet = InternetOpen ("Update Checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	
	if (!hINet)
	{
		return "Unknown";
	}

	hFile = InternetOpenUrl (hINet, "http://xchat-wdk.googlecode.com/git/version.txt?r=wdk", NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
	
	if (hFile)
	{
		static char buffer[1024];
		DWORD dwRead;
		while (InternetReadFile (hFile, buffer, 1023, &dwRead))
		{
			if (dwRead == 0)
			{
				break;
			}
			buffer[dwRead] = 0;
		}

		InternetCloseHandle (hFile);
		return buffer;
	}
	
	InternetCloseHandle (hINet);
	return "Unknown";
}

static int
print_version ()
{
	char *version = check_version ();

	if (strcmp (version, xchat_get_info (ph, "wdk_version")) == 0)
	{
		xchat_printf (ph, "You have the latest version of XChat-WDK installed!\n");
		update_available = 0;
	}
	else if (strcmp (version, "Unknown") == 0)
	{
		xchat_printf (ph, "Unable to check for XChat-WDK updates!\n");
		update_available = 0;
	}
	else
	{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for some reason */
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s%%20x64.exe\n", version);
#else
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s%%20x86.exe\n", version);
#endif
		update_available = 1;
	}

	return XCHAT_EAT_XCHAT;
}

static int
print_version_quiet (void *userdata)
{
	char *version = check_version ();

	/* if it's not the current version AND not network error */
	if (!(strcmp (version, xchat_get_info (ph, "wdk_version")) == 0) && !(strcmp (version, "Unknown") == 0))
	{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for plugins for some reason */
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s%%20x64.exe\n", version);
#else
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s%%20x86.exe\n", version);
#endif
		/* print update url once, then stop the timer */
		return 0;
	}
	/* keep checking */
	return 1;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	xchat_hook_command (ph, "UPDCHK", XCHAT_PRI_NORM, print_version, 0, 0);
	xchat_command (ph, "MENU -ietc\\download.png ADD \"Help/Check for Updates\" \"UPDCHK\"");

	xchat_printf (ph, "%s plugin loaded\n", name);

	/* only start the timer if there's no update available during startup */
	if (!update_available)
	{
		/* check for updates every 6 hours */
		xchat_hook_timer (ph, 21600000, print_version_quiet, NULL);
	}

	return 1;       /* return 1 for success */
}

int
xchat_plugin_deinit (void)
{
	xchat_command (ph, "MENU DEL \"Help/Check for updates\"");
	xchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
