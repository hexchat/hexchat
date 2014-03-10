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
#include <stdio.h>

#include "hexchat-plugin.h"

static hexchat_plugin *ph;
static char name[] = "Exec";
static char desc[] = "Execute commands inside HexChat";
static char version[] = "2.0";

#define MAX_COMMANDLINE_LENGTH 1024

#define TIMEOUT_SINGLE 250
#define TIMEOUT_TRIES 40
#define TIMEOUT_IN_SECONDS 10

#define PROCESS_CREATED 1
#define PROCESS_CREATION_FAILED 0

typedef struct execution_context
{
	int pending;

	hexchat_context *context;
	int to_channel;
	char commandline[MAX_COMMANDLINE_LENGTH];
	
	PROCESS_INFORMATION process_info;
	time_t startup;
	HANDLE pipe;

	hexchat_hook *hook;
} execution_context;

static execution_context current;

static void
prepare_execution_context (execution_context *context)
{
	ZeroMemory (context, sizeof(execution_context));
}

static int
timer_callback (execution_context *userdata)
{
	char buffer[1024];	
	double elapsed = difftime (time (NULL), userdata->startup);
	DWORD wait_result = WAIT_FAILED;

	if ((elapsed > TIMEOUT_IN_SECONDS) 
		|| (wait_result = WaitForSingleObject (userdata->process_info.hProcess, 0)) == WAIT_OBJECT_0)
	{
		char error[64] = { 0 };
		DWORD result;
		DWORD total = 0;
		hexchat_context *backup = hexchat_get_context (ph);
		int force_print = 0;
		char *token = NULL;
		char *c = NULL;

		if (backup != userdata->context)
		{
			force_print = !hexchat_set_context (ph, userdata->context);
		}

		if (wait_result == WAIT_OBJECT_0)
		{			
			while (ReadFile (userdata->pipe, buffer, sizeof(buffer)-1, &result, NULL) && result > 0)
			{
				buffer[result] = '\0';

				token = strtok_s (buffer, "\n", &c);
				while (token != NULL)
				{
					if (userdata->to_channel && !force_print)
					{
						hexchat_commandf (ph, "SAY %s", token);
					}
					else
					{
						hexchat_print (ph, token);
					}
					token = strtok_s (NULL, "\n", &c);
				}

				total += result;
			}

			if (total == 0)
			{
				strcat (error, "No output available.");
			}
		}
		else
		{
			sprintf (error, "Timeout after %dms", TIMEOUT_IN_SECONDS);
		}

		if (error[0])
		{
			if (force_print || !userdata->to_channel)
			{
				hexchat_print (ph, error);
			}
			else
			{
				hexchat_commandf (ph, "SAY %s", error);
			}			
		}

		if (backup != userdata->context)
		{
			hexchat_set_context (ph, backup);
		}

		CloseHandle (userdata->pipe);
		CloseHandle (userdata->process_info.hProcess);
		CloseHandle (userdata->process_info.hThread);
		userdata->pending = 0;
		return 0;
	}

	return 1;
}

static int
exec_command (hexchat_context *contex, char *commandline, int to_channel)
{
	prepare_execution_context (&current);
	strncpy (current.commandline, commandline, MAX_COMMANDLINE_LENGTH);
	current.context = contex;
	current.to_channel = to_channel;
	current.pending = 1;

	HANDLE read_pipe;
	HANDLE write_pipe;
	STARTUPINFO startup_info;
	SECURITY_ATTRIBUTES attr;

	ZeroMemory (&attr, sizeof (attr));
	attr.nLength = sizeof (attr);
	attr.bInheritHandle = TRUE;

	CreatePipe (&read_pipe, &write_pipe, &attr, 0);
	current.pipe = read_pipe;

	ZeroMemory (&startup_info, sizeof (startup_info));
	startup_info.cb = sizeof (startup_info);
	startup_info.dwFlags = STARTF_USESTDHANDLES;
	startup_info.hStdInput = NULL;
	startup_info.hStdOutput = write_pipe;
	startup_info.hStdError = write_pipe;

	BOOL created = CreateProcess (0, current.commandline, 0, 0, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, 0, 0, &startup_info, &current.process_info);
	CloseHandle (write_pipe);

	int result = PROCESS_CREATION_FAILED;

	if (created)
	{
		current.startup = time (NULL);
		current.hook = hexchat_hook_timer (ph, 500, timer_callback, &current);
		result = PROCESS_CREATED;
	}

	return result;
}

static int
exec_hook (char *word[], char *word_eol[], void *userdata)
{
	if (!current.pending)
	{
		if (strlen (word[2]) > 0)
		{
			char *commandline = malloc(MAX_COMMANDLINE_LENGTH);
			int to_channel = 0;

			strcpy (commandline, "cmd.exe /c ");

			if (!stricmp ("-O", word[2]))
			{
				strcat (commandline, word_eol[3]);
				to_channel = 1;
			}
			else
			{
				strcat (commandline, word_eol[2]);
			}

			hexchat_context *xcontext = hexchat_get_context (ph);
			if (!exec_command (xcontext, commandline, to_channel))
			{
				hexchat_print (ph, "Failed to create thread.");
			}

			free (commandline);
		}
		else
		{
			hexchat_command (ph, "help exec");
		}
	}
	else
	{
		hexchat_print (ph, "Plugin already busy.");
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

	prepare_execution_context (&current);

	hexchat_hook_command (ph, "EXEC", HEXCHAT_PRI_NORM, exec_hook, "Usage: /EXEC [-O] - execute commands inside HexChat", 0);
	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
