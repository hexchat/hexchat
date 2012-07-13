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

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include "utility.h"
#include "resource.h"
#include "sdAlerts.h"

int g_iAlerts = 0;

void sdSystemAlert(HINSTANCE hModule, UINT uiDialog, char *szMsg, char *szName, unsigned int iTime)
{
	TCHAR wszMsg[256];
	TCHAR wszName[64];

	HWND hDialog;
	RECT rcWorkArea, rcDlg;
	int ixPos, iyPos;
	int iNumPerCol;
	
	hDialog = CreateDialog(hModule, MAKEINTRESOURCE(uiDialog), NULL, (DLGPROC)sdAlertProc);

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
	GetWindowRect(hDialog, &rcDlg);

	iNumPerCol = ((rcWorkArea.bottom - rcWorkArea.top) / (rcDlg.bottom - rcDlg.top));
	ixPos = rcWorkArea.right - (rcDlg.right - rcDlg.left) + 1;
	iyPos = rcWorkArea.bottom - (rcDlg.bottom - rcDlg.top);

	if((g_iAlerts >= iNumPerCol) && (iNumPerCol > 0))
	{
		ixPos -= ((g_iAlerts / iNumPerCol) * (rcDlg.right - rcDlg.left));
		iyPos -= ((g_iAlerts - (iNumPerCol * (g_iAlerts / iNumPerCol))) * (rcDlg.bottom - rcDlg.top));
	}
	else
	{
		iyPos -= (g_iAlerts * (rcDlg.bottom - rcDlg.top));
	}
	SetWindowPos(hDialog, HWND_TOPMOST, ixPos, iyPos, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
	
	ConvertString(szName, wszName, 64);
	ConvertString(szMsg, wszMsg, 256);

	SetWindowText(hDialog, wszName);
	SetDlgItemText(hDialog, IDC_ALERT_MSG, wszMsg);
	ShowWindow(hDialog, SW_SHOWNA);

	if(iTime > 0)
	{
		SetTimer(hDialog, 1, iTime, NULL);
	}

	g_iAlerts++;
}

void sdCloseAlerts()
{
	PostMessage(HWND_BROADCAST, RegisterWindowMessage(TEXT("xTray:CloseAllAlertWindows")), 0, 0);
}

LRESULT CALLBACK sdAlertProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_CLOSE:
		if(g_iAlerts > 0){ g_iAlerts--; }
		DestroyWindow(hwnd);
		return TRUE;
		break;
	case WM_TIMER:
		if(g_iAlerts > 0){ g_iAlerts--; }
		AnimateWindow(hwnd, 600, AW_SLIDE | AW_HIDE | AW_VER_POSITIVE);
		DestroyWindow(hwnd);
		return TRUE;
		break;
	default:
		if(msg == RegisterWindowMessage(TEXT("xTray:CloseAllAlertWindows")))
		{
			if(g_iAlerts > 0){ g_iAlerts--; }
			DestroyWindow(hwnd);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

int sdAlertNum()
{
	return g_iAlerts;
}