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

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */






/*static void
run_command (char *word[], char *word_eol[], void *userdata)
{
	char commandLine[1024];
	char buf[100];
	char buff1[256];
	DWORD dwRead,tdwRead,tdwAfail,tdwLeft, dwAvail, ctr = 0;

	HANDLE readPipe, writePipe;
	STARTUPINFO sInfo; 
	PROCESS_INFORMATION pInfo; 
	BOOL res;	
	DWORD reDword; 

	SECURITY_ATTRIBUTES secattr; 
	ZeroMemory(&secattr,sizeof(secattr));
	secattr.nLength = sizeof(secattr);
	secattr.bInheritHandle = TRUE;
	
	xchat_printf (ph, "%d", strlen(word[2]));
	if (strlen (word[2]) > 0)
	{
		strcpy (commandLine, "cmd.exe /c ");
		strcat (commandLine, word_eol[2]);

		CreatePipe(&readPipe,&writePipe,&secattr,0);

		
		ZeroMemory(&sInfo,sizeof(sInfo));
		ZeroMemory(&pInfo,sizeof(pInfo));
		sInfo.cb=sizeof(sInfo);
		sInfo.dwFlags=STARTF_USESTDHANDLES;
		sInfo.hStdInput=NULL; 
		sInfo.hStdOutput=writePipe; 
		sInfo.hStdError=writePipe;

		CreateProcess(0, commandLine, 0, 0, TRUE, NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW, 0, 0, &sInfo, &pInfo);
		CloseHandle(writePipe);

		//now read the output pipe here.

		//do
		//{
		//	res=ReadFile(readPipe,buf,100,&reDword,0);
			//csTemp=buf;
			//m_csOutput+=csTemp.Left(reDword);
			//xchat_printf (ph, "%s", buf);
			//strcpy(buf, "\0");
			//fflush(buf);
		//}while(res);
		

		while (PeekNamedPipe(readPipe, buff1, 1, &dwRead, &dwAvail, &tdwLeft))
		{
			if (dwRead)
			{
				if (ReadFile(readPipe, buff1, sizeof(buff1) - 1, &dwRead, NULL) && dwRead != 0 )
				{
				buff1[dwRead] = '\0';
				//cout << buff1;
				xchat_printf (ph, "%s\n", buff1);
				//cout.flush();
				//fflush(buff1);
				//memset(&buff1[0], 0, sizeof(buff1));
				strcpy (buff1, "");
				}
			}
		}
	}
	
	return XCHAT_EAT_ALL;

}*/

static void
run_command (char *word[], char *word_eol[], void *userdata)
{
	return XCHAT_EAT_ALL;
}








/*static void
run_command (char *word[], char *word_eol[], void *userdata)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char commandLine[1024];
	HANDLE pipeWriter;
	HANDLE pipeReader;

	char buff1[128];
	DWORD dwRead,tdwRead,tdwAfail,tdwLeft, dwAvail, ctr = 0;
	
	//CreatePipe (&pipeReader, &pipeWriter, NULL, NULL);

	ZeroMemory (&si, sizeof (si));
	si.cb = sizeof (si);
	ZeroMemory (&pi, sizeof (pi));
	si.hStdOutput = pipeWriter;
	
	strcpy (commandLine, "cmd.exe /c ");
	strcat (commandLine, word_eol[2]);

	if (!CreateProcess ( NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		xchat_print (ph, "Error launching the Command Line!");
	}

	while (PeekNamedPipe(pipeReader, buff1, 1, &dwRead, &dwAvail, &tdwLeft)) {
		if (dwRead)
		{
			if (ReadFile(pipeReader, buff1, sizeof(buff1) - 1, &dwRead, NULL) && dwRead != 0 )
			{
				buff1[dwRead] = '\0';
				xchat_printf (ph, "%s\n", buff1);
				//cout.flush();
			}
		}
		else
		{
			sprintf(buff1, "No data loop count = %d. Do something here\n", ++ctr);
			xchat_printf (ph, "%s\n", buff1);
			//cout.flush();
			SleepEx(1000, FALSE);
		}
	}

	CloseHandle (pipeWriter);
	CloseHandle (pipeReader);
	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);
}*/

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "Exec";
	*plugin_desc = "Execute commands inside XChat";
	*plugin_version = "1.0";

	xchat_hook_command (ph, "EXEC", XCHAT_PRI_NORM, run_command, 0, 0);
	xchat_printf (ph, "Exec plugin loaded\n");

	return 1;       /* return 1 for success */
}

int
xchat_plugin_deinit (void)
{
	xchat_print (ph, "Exec plugin unloaded\n");
	return 1;
}
