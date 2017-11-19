/* HexChat
 * Copyright (c) 2011-2012 Berke Viktor.
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
#include <time.h>

#include "hexchat-plugin.h"

static hexchat_plugin *ph;   /* plugin handle */
static char name[] = "Exec";
static char desc[] = "Execute commands inside HexChat";
static char version[] = "1.2";

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

	char *token;
	char *context = NULL;
	int announce = 0;

	HANDLE readPipe;
	HANDLE writePipe;
	STARTUPINFO sInfo; 
	PROCESS_INFORMATION pInfo; 
	SECURITY_ATTRIBUTES secattr; 

	ZeroMemory (&secattr, sizeof (secattr));
	secattr.nLength = sizeof (secattr);
	secattr.bInheritHandle = TRUE;

	timeElapsed = 0.0;

	if (strlen (word[2]) > 0)
	{
		strcpy (commandLine, "cmd.exe /c ");

		if (!stricmp("-O", word[2]))
		{
			strcat (commandLine, word_eol[3]);
			announce = 1;
		}
		else
		{
			strcat (commandLine, word_eol[2]);
		}

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

					if (announce)
					{
						/* Say each line seperately, TODO: improve... */
						token = strtok_s (buffer, "\n", &context);
						while (token != NULL)
						{
							hexchat_commandf (ph, "SAY %s", token);
							token = strtok_s (NULL, "\n", &context);
						}
					}
					else
						hexchat_printf (ph, "%s", buffer);
				}
			}
			else
			{
				/* this way we'll more likely get full lines */
				SleepEx (100, TRUE);
			}
			timeElapsed = difftime (time (0), start);
		}

		/* display a newline to separate things */
		if (!announce)
			hexchat_printf (ph, "\n");

		if (timeElapsed >= 10)
		{
			hexchat_printf (ph, "Command took too much time to run, execution aborted.\n");
		}

		CloseHandle (readPipe);
		CloseHandle (pInfo.hProcess);
		CloseHandle (pInfo.hThread);
	}
	else
	{
		hexchat_command (ph, "help exec");
	}

	return HEXCHAT_EAT_HEXCHAT;
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	hexchat_hook_command (ph, "EXEC", HEXCHAT_PRI_NORM, run_command, "Usage: /EXEC [-O] - execute commands inside HexChat", 0);
	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;       /* return 1 for success */
}

int
hexchat_plugin_deinit (void)
{
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
