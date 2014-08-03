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
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wininet.h>
#include <cstdlib>
#include <string>
#include <glib.h>

#include "hexchat-plugin.h"


namespace{
	const int DEFAULT_DELAY = 30;	/* 30 seconds */
	const int DEFAULT_FREQ = 360;	/* 6 hours */
	const char DOWNLOAD_URL[] = "http://dl.hexchat.net/hexchat";
	static hexchat_plugin *ph;   /* plugin handle */
	static const char name[] = "Update Checker";
	static const char desc[] = "Check for HexChat updates automatically";
	static const char version[] = "4.0";
	static const char upd_help[] = "Update Checker Usage:\n  /UPDCHK, check for HexChat updates\n  /UPDCHK SET delay|freq, set startup delay or check frequency\n";

	struct inet_handle{
		HINTERNET handle;
		inet_handle(HINTERNET handle)
			:handle(handle){}
		~inet_handle()
		{
			InternetCloseHandle(handle);
		}
		operator HINTERNET()
		{
			return handle;
		}
	};
	static char*
		check_version()
	{
		inet_handle hOpen = InternetOpen(TEXT("Update Checker"),
			INTERNET_OPEN_TYPE_PRECONFIG,
			nullptr,
			nullptr,
			0);
		if (!hOpen)
		{
			return "Unknown";
		}

		inet_handle hConnect = InternetConnect(hOpen,
			TEXT("raw.github.com"),
			INTERNET_DEFAULT_HTTPS_PORT,
			nullptr,
			nullptr,
			INTERNET_SERVICE_HTTP,
			0,
			0);
		if (!hConnect)
		{
			return "Unknown";
		}

		inet_handle hResource = HttpOpenRequest(hConnect,
			TEXT("GET"),
			TEXT("/hexchat/hexchat/master/win32/version.txt"),
			nullptr, // use system setting to determine HTTP version
			nullptr,
			nullptr,
			INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_AUTH,
			0);
		if (!hResource)
		{
			return "Unknown";
		}
		else
		{
			static char buffer[1024];
			wchar_t infobuffer[32];
			int statuscode;

			DWORD dwRead;
			DWORD infolen = sizeof(infobuffer);

			HttpAddRequestHeaders(hResource, TEXT("Connection: close\r\n"), -1L, HTTP_ADDREQ_FLAG_ADD);	/* workaround for GC bug */
			HttpSendRequest(hResource, nullptr, 0, nullptr, 0);

			while (InternetReadFile(hResource, buffer, 1023, &dwRead))
			{
				if (dwRead == 0)
				{
					break;
				}
				buffer[dwRead] = 0;
			}

			HttpQueryInfo(hResource,
				HTTP_QUERY_STATUS_CODE,
				&infobuffer,
				&infolen,
				nullptr);
			::std::wstring status_code_string(infobuffer, 32);
			statuscode = std::stoi(status_code_string);
			if (statuscode == 200)
				return buffer;
			else
				return "Unknown";
		}
	}

	static int
		print_version(const char * const word[], const char * const word_eol[], void *userdata)
	{
		char *version;
		int prevbuf;
		int convbuf;

		if (!g_ascii_strcasecmp("HELP", word[2]))
		{
			hexchat_printf(ph, upd_help);
			return HEXCHAT_EAT_HEXCHAT;
		}
		else if (!g_ascii_strcasecmp("SET", word[2]))
		{
			if (!g_ascii_strcasecmp("", word_eol[4]))
			{
				hexchat_printf(ph, "%s\tEnter a value!\n", name);
				return HEXCHAT_EAT_HEXCHAT;
			}
			if (!g_ascii_strcasecmp("delay", word[3]))
			{
				convbuf = atoi(word[4]);	/* don't use word_eol, numbers must not contain spaces */

				if (convbuf > 0 && convbuf < INT_MAX)
				{
					prevbuf = hexchat_pluginpref_get_int(ph, "delay");
					hexchat_pluginpref_set_int(ph, "delay", convbuf);
					hexchat_printf(ph, "%s\tUpdate check startup delay is set to %d seconds (from %d).\n", name, convbuf, prevbuf);
				}
				else
				{
					hexchat_printf(ph, "%s\tInvalid input!\n", name);
				}
			}
			else if (!g_ascii_strcasecmp("freq", word[3]))
			{
				convbuf = atoi(word[4]);	/* don't use word_eol, numbers must not contain spaces */

				if (convbuf > 0 && convbuf < INT_MAX)
				{
					prevbuf = hexchat_pluginpref_get_int(ph, "freq");
					hexchat_pluginpref_set_int(ph, "freq", convbuf);
					hexchat_printf(ph, "%s\tUpdate check frequency is set to %d minutes (from %d).\n", name, convbuf, prevbuf);
				}
				else
				{
					hexchat_printf(ph, "%s\tInvalid input!\n", name);
				}
			}
			else
			{
				hexchat_printf(ph, "%s\tInvalid variable name! Use 'delay' or 'freq'!\n", name);
				return HEXCHAT_EAT_HEXCHAT;
			}

			return HEXCHAT_EAT_HEXCHAT;
		}
		else if (!g_ascii_strcasecmp("", word[2]))
		{
			version = check_version();

			if (strcmp(version, hexchat_get_info(ph, "version")) == 0)
			{
				hexchat_printf(ph, "%s\tYou have the latest version of HexChat installed!\n", name);
			}
			else if (strcmp(version, "Unknown") == 0)
			{
				hexchat_printf(ph, "%s\tUnable to check for HexChat updates!\n", name);
			}
			else
			{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for some reason */
				hexchat_printf(ph, "%s:\tA HexChat update is available! You can download it from here:\n%s/HexChat%%20%s%%20x64.exe\n", name, DOWNLOAD_URL, version);
#else
				hexchat_printf (ph, "%s:\tA HexChat update is available! You can download it from here:\n%s/HexChat%%20%s%%20x86.exe\n", name, DOWNLOAD_URL, version);
#endif
			}
			return HEXCHAT_EAT_HEXCHAT;
		}
		else
		{
			hexchat_printf(ph, upd_help);
			return HEXCHAT_EAT_HEXCHAT;
		}
	}

	static int
		print_version_quiet(void *userdata)
	{
		char *version = check_version();

		/* if it's not the current version AND not network error */
		if (!(strcmp(version, hexchat_get_info(ph, "version")) == 0) && !(strcmp(version, "Unknown") == 0))
		{
#ifdef _WIN64 /* use this approach, the wProcessorArchitecture method always returns 0 (=x86) for plugins for some reason */
			hexchat_printf(ph, "%s\tA HexChat update is available! You can download it from here:\n%s/HexChat%%20%s%%20x64.exe\n", name, DOWNLOAD_URL, version);
#else
			hexchat_printf (ph, "%s\tA HexChat update is available! You can download it from here:\n%s/HexChat%%20%s%%20x86.exe\n", name, DOWNLOAD_URL, version);
#endif
			/* print update url once, then stop the timer */
			return 0;
		}
		/* keep checking */
		return 1;
	}

	static int
		delayed_check(void *userdata)
	{
		int freq = hexchat_pluginpref_get_int(ph, "freq");

		/* only start the timer if there's no update available during startup */
		if (print_version_quiet(nullptr))
		{
			/* check for updates, every 6 hours by default */
			hexchat_hook_timer(ph, freq * 1000 * 60, print_version_quiet, nullptr);
		}

		return 0;	/* run delayed_check() only once */
	}
}

int
hexchat_plugin_init(hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	int delay;
	ph = plugin_handle;

	*plugin_name = const_cast<char*>(name);
	*plugin_desc = const_cast<char*>(desc);
	*plugin_version = const_cast<char*>(version);

	/* these are required for the very first run */
	delay = hexchat_pluginpref_get_int(ph, "delay");
	if (delay == -1)
	{
		delay = DEFAULT_DELAY;
		hexchat_pluginpref_set_int(ph, "delay", DEFAULT_DELAY);
	}

	if (hexchat_pluginpref_get_int(ph, "freq") == -1)
	{
		hexchat_pluginpref_set_int(ph, "freq", DEFAULT_FREQ);
	}

	hexchat_hook_command(ph, "UPDCHK", HEXCHAT_PRI_NORM, print_version, upd_help, nullptr);
	hexchat_hook_timer(ph, delay * 1000, delayed_check, nullptr);
	hexchat_command(ph, "MENU -ishare\\download.png ADD \"Help/Check for Updates\" \"UPDCHK\"");
	hexchat_printf(ph, "%s plugin loaded\n", name);

	return 1;       /* return 1 for success */
}

int
hexchat_plugin_deinit(void)
{
	hexchat_command(ph, "MENU DEL \"Help/Check for updates\"");
	hexchat_printf(ph, "%s plugin unloaded\n", name);
	return 1;
}
