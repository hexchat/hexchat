/* HexChat
 * Copyright (c) 2010-2012 Berke Viktor.
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

#include <glib.h>

#include "hexchat-plugin.h"

#define DEFAULT_DELAY 10	/* 10 seconds */
#define DEFAULT_FREQ 360	/* 6 hours */

static hexchat_plugin *ph;   /* plugin handle */
static char name[] = "Update Checker";
static char desc[] = "Check for HexChat updates automatically";
static char version[] = "4.0";
static const char upd_help[] = "Update Checker Usage:\n  /UPDCHK, check for HexChat updates\n  /UPDCHK SET delay|freq, set startup delay or check frequency\n";

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
							"https://raw.github.com/hexchat/hexchat/master/win32/version.txt",
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
		if (strlen (buffer) == 5)
			return buffer;
		else
			return "Unknown";
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
								TEXT ("raw.github.com"),
								INTERNET_DEFAULT_HTTPS_PORT,
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
								TEXT ("/hexchat/hexchat/master/win32/version.txt"),
								TEXT ("HTTP/1.0"),
								NULL,
								NULL,
								INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_AUTH,
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
		if (strlen (buffer) == 5)
			return buffer;
		else
			return "Unknown";
	}
}

static int
print_version (char *word[], char *word_eol[], void *userdata)
{
	char *version;
	int prevbuf;
	int convbuf;

	if (!g_ascii_strcasecmp ("HELP", word[2]))
	{
		hexchat_printf (ph, upd_help);
		return HEXCHAT_EAT_HEXCHAT;
	}
	else if (!g_ascii_strcasecmp ("SET", word[2]))
	{
		if (!g_ascii_strcasecmp ("", word_eol[4]))
		{
			hexchat_printf (ph, "%s\tEnter a value!\n", name);
			return HEXCHAT_EAT_HEXCHAT;
		}
		if (!g_ascii_strcasecmp ("delay", word[3]))
		{
			convbuf = atoi (word[4]);	/* don't use word_eol, numbers must not contain spaces */

			if (convbuf > 0 && convbuf < INT_MAX)
			{
				prevbuf = hexchat_pluginpref_get_int (ph, "delay");
				hexchat_pluginpref_set_int (ph, "delay", convbuf);
				hexchat_printf (ph, "%s\tUpdate check startup delay is set to %d seconds (from %d).\n", name, convbuf, prevbuf);
			}
			else
			{
				hexchat_printf (ph, "%s\tInvalid input!\n", name);
			}
		}
		else if (!g_ascii_strcasecmp ("freq", word[3]))
		{
			convbuf = atoi (word[4]);	/* don't use word_eol, numbers must not contain spaces */

			if (convbuf > 0 && convbuf < INT_MAX)
			{
				prevbuf = hexchat_pluginpref_get_int (ph, "freq");
				hexchat_pluginpref_set_int (ph, "freq", convbuf);
				hexchat_printf (ph, "%s\tUpdate check frequency is set to %d minutes (from %d).\n", name, convbuf, prevbuf);
			}
			else
			{
				hexchat_printf (ph, "%s\tInvalid input!\n", name);
			}
		}
		else
		{
			hexchat_printf (ph, "%s\tInvalid variable name! Use 'delay' or 'freq'!\n", name);
			return HEXCHAT_EAT_HEXCHAT;
		}

		return HEXCHAT_EAT_HEXCHAT;
	}
	else if (!g_ascii_strcasecmp ("", word[2]))
	{
		version = check_version ();

		if (strcmp (version, hexchat_get_info (ph, "version")) == 0)
		{
			hexchat_printf (ph, "%s\tYou have the latest version of HexChat installed!\n", name);
		}
		else if (strcmp (version, "Unknown") == 0)
		{
			hexchat_printf (ph, "%s\tUnable to check for HexChat updates!\n", name);
		}
		else
		{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for some reason */
			hexchat_printf (ph, "%s\tA HexChat update is available! You can download it from here:\nhttp://dl.hexchat.net/hexchat/HexChat%%20%s%%20x64.exe\n", name, version);
#else
			hexchat_printf (ph, "%s\tA HexChat update is available! You can download it from here:\nhttp://dl.hexchat.net/hexchat/HexChat%%20%s%%20x86.exe\n", name, version);
#endif
		}
		return HEXCHAT_EAT_HEXCHAT;
	}
	else
	{
		hexchat_printf (ph, upd_help);
		return HEXCHAT_EAT_HEXCHAT;
	}
}

static int
print_version_quiet (void *userdata)
{
	char *version = check_version ();

	/* if it's not the current version AND not network error */
	if (!(strcmp (version, hexchat_get_info (ph, "version")) == 0) && !(strcmp (version, "Unknown") == 0))
	{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for plugins for some reason */
		hexchat_printf (ph, "%s\tA HexChat update is available! You can download it from here:\nhttps://github.com/downloads/hexchat/hexchat/HexChat%%20%s%%20x64.exe\n", name, version);
#else
		hexchat_printf (ph, "%s\tA HexChat update is available! You can download it from here:\nhttps://github.com/downloads/hexchat/hexchat/HexChat%%20%s%%20x86.exe\n", name, version);
#endif
		/* print update url once, then stop the timer */
		return 0;
	}
	/* keep checking */
	return 1;
}

static int
delayed_check (void *userdata)
{
	int freq = hexchat_pluginpref_get_int (ph, "freq");

	/* only start the timer if there's no update available during startup */
	if (print_version_quiet (NULL))
	{
		/* check for updates, every 6 hours by default */
		hexchat_hook_timer (ph, freq * 1000 * 60, print_version_quiet, NULL);
	}

	return 0;	/* run delayed_check() only once */
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	int delay;
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	/* these are required for the very first run */
	delay = hexchat_pluginpref_get_int (ph, "delay");
	if (delay == -1)
	{
		delay = DEFAULT_DELAY;
		hexchat_pluginpref_set_int (ph, "delay", DEFAULT_DELAY);
	}

	if (hexchat_pluginpref_get_int (ph, "freq") == -1)
	{
		hexchat_pluginpref_set_int (ph, "freq", DEFAULT_FREQ);
	}

	hexchat_hook_command (ph, "UPDCHK", HEXCHAT_PRI_NORM, print_version, upd_help, NULL);
	hexchat_hook_timer (ph, delay * 1000, delayed_check, NULL);
	hexchat_command (ph, "MENU -ishare\\download.png ADD \"Help/Check for Updates\" \"UPDCHK\"");
	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;       /* return 1 for success */
}

int
hexchat_plugin_deinit (void)
{
	hexchat_command (ph, "MENU DEL \"Help/Check for updates\"");
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
