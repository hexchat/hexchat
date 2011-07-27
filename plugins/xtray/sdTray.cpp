/* X-Tray
 * Copyright (C) 2005 Michael Hotaling <Mike.Hotaling@SinisterDevelopments.com>
 *
 * X-Tray is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * X-Tray is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with X-Tray; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define _WIN32_IE 0x601
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "sdTray.h"
#include "utility.h"

HANDLE hThread;
iBlink *tData	= NULL;

int AddIcon(HWND hwnd, UINT id, HICON hicon, char *szTip, unsigned short flags, unsigned int cbMsg)
{
	NOTIFYICONDATA nidata;

	nidata.cbSize			= NOTIFYICONDATA_V2_SIZE;
	nidata.hIcon			= hicon;
	nidata.hWnd				= hwnd;
	nidata.uCallbackMessage = cbMsg;
	nidata.uFlags			= flags;
	nidata.uID				= id;

	if(szTip != NULL)
	{
		TCHAR *szTemp = new TCHAR[64];

		ConvertString(szTip, szTemp, 64);
		_tcsncpy(nidata.szTip, szTemp, 64);

		delete[] szTemp;
	}

	return Shell_NotifyIcon(NIM_ADD, &nidata);
}

int RemoveIcon(HWND hwnd, UINT id)
{
	if(hThread != NULL)
	{
		TerminateThread(hThread, 0);
		hThread = NULL;

		delete tData;
	}

	NOTIFYICONDATA nidata;

	nidata.cbSize = NOTIFYICONDATA_V2_SIZE;
	nidata.hWnd   = hwnd;
	nidata.uID    = id;

	return Shell_NotifyIcon(NIM_DELETE, &nidata);
}

int SetIcon(HWND hwnd, UINT id, HICON hicon)
{
	NOTIFYICONDATA nidata;

	nidata.cbSize = NOTIFYICONDATA_V2_SIZE;
	nidata.hWnd   = hwnd;
	nidata.uID    = id;
	nidata.hIcon  = hicon;
	nidata.uFlags = NIF_ICON;

	return Shell_NotifyIcon(NIM_MODIFY, &nidata);
}

int SetTooltip(HWND hwnd, UINT id, char *szTip)
{
	NOTIFYICONDATA nidata;

	nidata.cbSize = NOTIFYICONDATA_V2_SIZE;
	nidata.hWnd   = hwnd;
	nidata.uID    = id;
	nidata.uFlags = NIF_TIP;

	if(szTip != NULL)
	{
		TCHAR *szTemp = new TCHAR[64];
		ConvertString(szTip, szTemp, 64);
		_tcsncpy(nidata.szTip, szTemp, 64);
		delete[] szTemp;
	}

	return Shell_NotifyIcon(NIM_MODIFY, &nidata);
}

int ShowBalloon(HWND hwnd, UINT id, char *szInfo, char *szTitle, UINT time, UINT infoFlags)
{
	NOTIFYICONDATA nidata;

	nidata.cbSize	= NOTIFYICONDATA_V2_SIZE;
	nidata.hWnd		= hwnd;
	nidata.uID		= id;
	nidata.uFlags	= NIF_INFO;
	nidata.dwInfoFlags = infoFlags;

	if(time > 0)
		nidata.uTimeout = time;
	else
		nidata.uTimeout = 500000;

	if(szInfo != NULL)
	{
		TCHAR *szTemp = new TCHAR[255];

		ConvertString(szInfo, szTemp, 251);
		szTemp[250] = 0;
		
		if(strlen(szInfo) > 255)
		{
			_sntprintf(szTemp, 255, _T("%s..."), szTemp);
		}
		_tcsncpy(nidata.szInfo, szTemp, 255);

		delete[] szTemp;
	}
	if(szTitle != NULL)
	{
		TCHAR *wszTitle = new TCHAR[64];
		ConvertString(szTitle, wszTitle, 64);
		_tcsncpy(nidata.szInfoTitle, wszTitle, 64);
		delete[] wszTitle;
	}

	return Shell_NotifyIcon(NIM_MODIFY, &nidata);
}


DWORD WINAPI BlinkThread(LPVOID lpParam)
{
	NOTIFYICONDATA nidata;

	nidata.cbSize = NOTIFYICONDATA_V2_SIZE;
	nidata.hWnd   = tData->hwnd;
	nidata.uID    = tData->id;
	nidata.uFlags = NIF_ICON;

	for(UINT i = 0; i < tData->num; i++)
	{
		nidata.hIcon = tData->hBlink;
		Shell_NotifyIcon(NIM_MODIFY, &nidata);

		Sleep(tData->time);

		nidata.hIcon = tData->hBase;
		Shell_NotifyIcon(NIM_MODIFY, &nidata);

		Sleep(tData->time);
	}

	delete tData;
	hThread = NULL;

	return 0;
}

int BlinkIcon(HWND hwnd, UINT id, HICON hBase, HICON hBlink, UINT time, UINT num)
{
	if(hThread != NULL)
	{
		StopBlink(hwnd, id, hBase);
	}

	DWORD dwThreadID;
	tData = new iBlink;

	tData->hwnd		= hwnd;
	tData->id		= id;
	tData->hBase	= hBase;
	tData->hBlink	= hBlink;
	tData->time		= time;
	tData->num		= num;

	hThread = CreateThread(NULL, 0, BlinkThread, tData, 0, &dwThreadID);

	return 0;
}

void StopBlink(HWND hwnd, UINT id, HICON hBase)
{
	if(hThread != NULL)
	{
		TerminateThread(hThread, 0);
		hThread = NULL;

		delete tData;
	}

	SetIcon(hwnd, id, hBase);
}