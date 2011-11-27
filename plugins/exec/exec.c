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
#include <time.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */

static int
run_command (char *word[], char *word_eol[], void *userdata)
{
	char commandLine[1024];
	char buffer[4096];
	DWORD dwRead = 0;
	DWORD dwLeft = 0;
	DWORD dwAvail = 0;
	time_t start;
	double timeElapsed;

	HANDLE readPipe;
	HANDLE writePipe;
	STARTUPINFO sInfo; 
	PROCESS_INFORMATION pInfo; 
	SECURITY_ATTRIBUTES secattr; 

	ZeroMemory (&secattr, sizeof (secattr));
	secattr.nLength = sizeof (secattr);
	secattr.bInheritHandle = TRUE;

	if (strlen (word[2]) > 0)
	{
		strcpy (commandLine, "cmd.exe /c ");
		strcat (commandLine, word_eol[2]);

		CreatePipe (&readPipe, &writePipe, &secattr, 0); /* might be replaced with MyCreatePipeEx */

		ZeroMemory (&sInfo, sizeof (sInfo));
		ZeroMemory (&pInfo, sizeof (pInfo));
		sInfo.cb = sizeof (sInfo);
		sInfo.dwFlags = STARTF_USESTDHANDLES;
		sInfo.hStdInput = NULL;
		sInfo.hStdOutput = writePipe;
		sInfo.hStdError = writePipe;

		CreateProcess (0, commandLine, 0, 0, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, 0, 0, &sInfo, &pInfo);
		CloseHandle (writePipe);

		start = time (0);
		while (PeekNamedPipe (readPipe, buffer, 1, &dwRead, &dwAvail, &dwLeft) && timeElapsed < 10)
		{
			if (dwRead)
			{
				if (ReadFile (readPipe, buffer, sizeof (buffer) - 1, &dwRead, NULL) && dwRead != 0 )
				{
					/* avoid garbage */
					buffer[dwRead] = '\0';
					xchat_printf (ph, "%s", buffer);
				}
			}
			else
			{
				/* this way we'll more likely get full lines */
				SleepEx (100, TRUE);
			}
			timeElapsed = difftime (time (0), start);
		}
	}
	
	if (timeElapsed >= 10)
	{
		xchat_printf (ph, "Command took too much time to run, execution aborted.\n");
	}

	CloseHandle (readPipe);
	CloseHandle (pInfo.hProcess);
	CloseHandle (pInfo.hThread);

	return XCHAT_EAT_XCHAT;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "Exec";
	*plugin_desc = "Execute commands inside XChat";
	*plugin_version = "1.0";

	xchat_hook_command (ph, "EXEC", XCHAT_PRI_NORM, run_command, 0, 0);
	xchat_printf (ph, "%s plugin loaded\n", *plugin_name);

	return 1;       /* return 1 for success */
}

int
xchat_plugin_deinit (void)
{
	xchat_print (ph, "Exec plugin unloaded\n");
	return 1;
}
