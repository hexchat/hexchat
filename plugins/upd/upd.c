/* XChat-WDK
 * Copyright (c) 2010 Berke Viktor.
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
#include <wininet.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */

char*
check_version ()
{
	HINTERNET hINet, hFile;
	hINet = InternetOpen ("Update Checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	
	if (!hINet)
	{
		return "Unknown";
	}

	hFile = InternetOpenUrl (hINet, "http://xchat-wdk.googlecode.com/hg/version.txt", NULL, 0, 0, 0);
	
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

void
print_version ()
{
	char *version = check_version ();

	if (strcmp (version, xchat_get_info (ph, "version")) == 0)
	{
		xchat_printf (ph, "You have the latest version of XChat-WDK installed!\n");
	}
	else if (strcmp (version, "Unknown") == 0)
	{
		xchat_printf (ph, "Unable to check for XChat-WDK updates!\n");
	}
	else
	{
		xchat_printf (ph, "An XChat-WDK update is available! You can download it from here:\nhttp://xchat-wdk.googlecode.com/files/XChat-WDK%%20%s.exe\n", version);
	}
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "Update Checker";
	*plugin_desc = "Plugin for checking for XChat-WDK updates";
	*plugin_version = "1.1";

	xchat_hook_command (ph, "UPDCHK", XCHAT_PRI_NORM, print_version, 0, 0);
	xchat_command (ph, "MENU -ietc\\download.png ADD \"Help/Check for Updates\" \"UPDCHK\"");

	xchat_print (ph, "Update Checker plugin loaded\n");
	print_version ();

	return 1;       /* return 1 for success */
}

int
xchat_plugin_deinit (void)
{
	xchat_command (ph, "MENU DEL \"Help/Check for updates\"");
	xchat_print (ph, "Update Checker plugin unloaded\n");
	return 1;
}
