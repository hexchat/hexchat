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

#define THREAD_CREATED 1
#define THREAD_CREATION_FAILED 0

#define RESULT_OK 1
#define RESULT_CREATE_PROCESS_FAILED 2
#define RESULT_NO_DATA 3
#define RESULT_TIMEOUT 4

typedef struct execution_context
{
	int pending;

	hexchat_context *context;
	int to_channel;
	char commandline[MAX_COMMANDLINE_LENGTH];
	
	PROCESS_INFORMATION process_info;
	DWORD exit_code;
	HANDLE thread;
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
	
	if (WaitForSingleObject(userdata->thread, 0) == WAIT_OBJECT_0)
	{
		char error[64] = { 0 };
		DWORD result;
		hexchat_context *backup = hexchat_get_context (ph);
		int force_print = 0;
		char *token = NULL;
		char *c = NULL;

		if (backup != userdata->context)
		{
			force_print = !hexchat_set_context (ph, userdata->context);
		}

		if (GetExitCodeThread (userdata->thread, &result))
		{
			if (result == RESULT_OK)
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
				}
			}
			else if (result == RESULT_NO_DATA)
			{
				strcat (error, "No output available.");
			}
			else if (result == RESULT_TIMEOUT)
			{
				sprintf (error, "Timeout after %dms", TIMEOUT_SINGLE * TIMEOUT_TRIES);
			}
			else if (result == RESULT_CREATE_PROCESS_FAILED)
			{
				strcat (error, "Failed to start process.");
			}
			else
			{
				strcat (error, "Unknown error.");
			}
		}
		else
		{
			strcat (error, "Failed to retrive thread result.");
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

		CloseHandle (userdata->thread);
		CloseHandle (userdata->pipe);
		userdata->pending = 0;
		return 0;
	}
	return 1;
}

static int 
do_run_command (execution_context *context)
{
	context->pending = 1;

	HANDLE read_pipe;
	HANDLE write_pipe;
	STARTUPINFO startup_info;
	SECURITY_ATTRIBUTES attr;

	ZeroMemory (&attr, sizeof (attr));
	attr.nLength = sizeof (attr);
	attr.bInheritHandle = TRUE;

	CreatePipe (&read_pipe, &write_pipe, &attr, 0);
	context->pipe = read_pipe;

	ZeroMemory (&startup_info, sizeof (startup_info));
	startup_info.cb = sizeof (startup_info);
	startup_info.dwFlags = STARTF_USESTDHANDLES;
	startup_info.hStdInput = NULL;
	startup_info.hStdOutput = write_pipe;
	startup_info.hStdError = write_pipe;

	BOOL created = CreateProcess (0, context->commandline, 0, 0, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, 0, 0, &startup_info, &context->process_info);
	CloseHandle (write_pipe);

	int thread_result;

	if (created)
	{		
		char short_buffer[1];
		DWORD dwRead = 0;
		DWORD dwLeft = 0;
		DWORD dwAvail = 0;

		int tries = 0;
		while (1)
		{
			++tries;

			if (WaitForSingleObject (context->process_info.hProcess, TIMEOUT_SINGLE) == WAIT_OBJECT_0)
			{			
				PeekNamedPipe (read_pipe, short_buffer, 1, &dwRead, &dwAvail, &dwLeft);
				if (dwRead)
				{
					GetExitCodeProcess (context->process_info.hProcess, &context->exit_code);
					thread_result = RESULT_OK;
				}
				else
				{
					thread_result = RESULT_NO_DATA;
				}

				break;
			}
			else if (tries == TIMEOUT_TRIES)
			{
				thread_result = RESULT_TIMEOUT;
				break;
			}
		}

		CloseHandle (context->process_info.hProcess);
		CloseHandle (context->process_info.hThread);
	}
	else
	{
		thread_result = RESULT_CREATE_PROCESS_FAILED;
	}

	return thread_result;
}

static int
run_command_threaded (hexchat_context *contex, char *commandline, int to_channel)
{
	prepare_execution_context (&current);
	strncpy (current.commandline, commandline, MAX_COMMANDLINE_LENGTH);
	current.context = contex;
	current.to_channel = to_channel;
	
	DWORD thread_id;
	HANDLE handle = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)do_run_command, &current, 0, &thread_id);
	if (handle)
	{
		current.thread = handle;		
		current.hook = hexchat_hook_timer (ph, 500, timer_callback, &current);
		return THREAD_CREATED;
	}

	return THREAD_CREATION_FAILED;
}

static int
run_command (char *word[], char *word_eol[], void *userdata)
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
			int result = run_command_threaded (xcontext, commandline, to_channel);

			if (result == THREAD_CREATION_FAILED)
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

	hexchat_hook_command (ph, "EXEC", HEXCHAT_PRI_NORM, run_command, "Usage: /EXEC [-O] - execute commands inside HexChat", 0);
	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
