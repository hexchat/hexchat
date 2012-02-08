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
static const char version[] = "2.1";

static char*
check_version ()
{
#if 0
	HINTERNET hINet, hFile;
	hINet = InternetOpen ("Update Checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	if (!hINet)
	{
		return "Unknown";
	}

	hFile = InternetOpenUrl (hINet,
							"http://xchat-wdk.googlecode.com/git/version.txt?r=wdk",
							NULL,
							0,
							INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD,
							0);
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
		InternetCloseHandle (hINet);
		return buffer;
	}

	InternetCloseHandle (hINet);
	return "Unknown";
#endif

	/* Google Code's messing up with requests, use HTTP/1.0 as suggested. More info:

	   http://code.google.com/p/support/issues/detail?id=6095

	   Of course it would be still too simple, coz IE will override settings, so
	   you have to disable HTTP/1.1 manually and globally. More info:

	   http://support.microsoft.com/kb/258425

	   So this code's basically useless since disabling HTTP/1.1 will work with the
	   above code too.

	   Update: a Connection: close header seems to disable chunked encoding.
	*/

	HINTERNET hOpen, hConnect, hResource;

	hOpen = InternetOpen (TEXT ("Update Checker"),
						INTERNET_OPEN_TYPE_PRECONFIG,
						NULL,
						NULL,
						0);
	if (!hOpen)
	{
		return "Unknown";
	}

	hConnect = InternetConnect (hOpen,
								TEXT ("xchat-wdk.googlecode.com"),
								INTERNET_INVALID_PORT_NUMBER,
								NULL,
								NULL,
								INTERNET_SERVICE_HTTP,
								0,
								0);
	if (!hConnect)
	{
		InternetCloseHandle (hOpen);
		return "Unknown";
	}

	hResource = HttpOpenRequest (hConnect,
								TEXT ("GET"),
								TEXT ("/git/version.txt?r=wdk"),
								TEXT ("HTTP/1.0"),
								NULL,
								NULL,
								INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_AUTH,
								0);
	if (!hResource)
	{
		InternetCloseHandle (hConnect);
		InternetCloseHandle (hOpen);
		return "Unknown";
	}
	else
	{
		static char buffer[1024];
		DWORD dwRead;

		HttpAddRequestHeaders (hResource, TEXT ("Connection: close\r\n"), -1L, HTTP_ADDREQ_FLAG_ADD);	/* workaround for GC bug */
		HttpSendRequest (hResource, NULL, 0, NULL, 0);

		while (InternetReadFile (hResource, buffer, 1023, &dwRead))
		{
			if (dwRead == 0)
			{
				break;
			}
			buffer[dwRead] = 0;
		}

		InternetCloseHandle (hResource);
		InternetCloseHandle (hConnect);
		InternetCloseHandle (hOpen);
		return buffer;
	}
}

static int
print_version ()
{
	char *version = check_version ();

	if (strcmp (version, xchat_get_info (ph, "wdk_version")) == 0)
	{
		xchat_printf (ph, "You have the latest version of XChat-WDK installed!\n");
	}
	else if (strcmp (version, "Unknown") == 0)
	{
		xchat_printf (ph, "Unable to check for XChat-WDK updates!\n");
	}
	else
	{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for some reason */
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s%%20x64.exe\n", version);
#else
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s%%20x86.exe\n", version);
#endif
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
	if (print_version_quiet (NULL))
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
