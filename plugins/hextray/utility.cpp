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
#include <stdio.h>
#include <commctrl.h>
#include <tchar.h>

#include "xchat-plugin.h"
#include "utility.h"
#include "xtray.h"
#include "callbacks.h"
#include "resource.h"

TCHAR BACKUP_INI_FILE[] = _T(".\\plugins\\config\\xtray.conf");
struct HOTKEY g_hHotKey;

/* we need to convert ALT and SHIFT modifiers
// from <winuser.h>
#define MOD_ALT         0x0001
#define MOD_CONTROL     0x0002
#define MOD_SHIFT       0x0004
// from <commctrl.h>
#define HOTKEYF_SHIFT           0x01
#define HOTKEYF_CONTROL         0x02
#define HOTKEYF_ALT             0x04
*/

WORD HotkeyfToMod(WORD modifiers)
{
	WORD newmods = 0;

	if (modifiers & HOTKEYF_SHIFT)
		newmods |= MOD_SHIFT;

	if (modifiers & HOTKEYF_CONTROL)
		newmods |= MOD_CONTROL;

	if (modifiers & HOTKEYF_ALT)
		newmods |= MOD_ALT;

	return newmods;
}

WORD ModToHotkeyf(WORD modifiers)
{
	WORD newmods = 0;

	if (modifiers & MOD_SHIFT)
		newmods |= HOTKEYF_SHIFT;

	if (modifiers & MOD_CONTROL)
		newmods |= HOTKEYF_CONTROL;

	if (modifiers & MOD_ALT)
		newmods |= HOTKEYF_ALT;

	return newmods;
}

void SavePrefs(int iDlg)
{
	/**************************************************************************************************/
	/********** allocate space for our string, and then set it to the currently logged on user ********/
	/**************************************************************************************************/
	DWORD dBuff = 257;
	TCHAR szUser[257];

	GetUserName(szUser, &dBuff);

	/**************************************************************************************************/
	/*********************** Test if the file exists, If it doesn't, Create It ************************/
	/**************************************************************************************************/
	TCHAR XTRAY_INI_FILE[1024];
	
#ifdef UNICODE
	char temp[1024];
    TCHAR TEMP_INI_FILE[1024];
	
	_snprintf(temp, 1024, "%s\\xtray.conf", xchat_get_info(ph, "xchatdir"));
	ConvertString(temp, TEMP_INI_FILE, 1024);

	// ok this one is really ugly
	// it checks to see if the file exists in two locations
	// X-Chat default config dir, if that fails it trys xchat\plugins\config
	// if neither one exists it tries to create it in
	// X-Chat default config dir, if that fails it trys xchat\plugins\config
	// In either case it writes \xFF\xFE to the file ( on creation )
	// so that we can save unicode away messages WritePrivateProfile doesn't 
	// do this for us, though I think it really should

	if(FileExists(TEMP_INI_FILE))
	{
		_tcscpy(XTRAY_INI_FILE, TEMP_INI_FILE);
	}
	else
	{
		if(FileExists(BACKUP_INI_FILE))
		{
			_tcscpy(XTRAY_INI_FILE, BACKUP_INI_FILE);
		}
		else
		{
			HANDLE xTemp;
			DWORD dwBytesTemp;

			if(xTemp = CreateFile(TEMP_INI_FILE, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL))
			{
				if(GetLastError() != ERROR_ALREADY_EXISTS)
				{
					WriteFile(xTemp, _T("\xFF\xFE"), 4, &dwBytesTemp, NULL);
				}
				CloseHandle(xTemp);
			}

			if(FileExists(TEMP_INI_FILE))
			{
				_tcscpy(XTRAY_INI_FILE, TEMP_INI_FILE);
			}
			else
			{
				HANDLE xBackup;
				DWORD dwBytesBackup;

				if(xBackup = CreateFile(TEMP_INI_FILE, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL))
				{
					if(GetLastError() != ERROR_ALREADY_EXISTS)
					{
						WriteFile(xBackup, _T("\xFF\xFE"), 4, &dwBytesBackup, NULL);
					}

					CloseHandle(xBackup);
				}

				if(FileExists(BACKUP_INI_FILE))
				{
					_tcscpy(XTRAY_INI_FILE, BACKUP_INI_FILE);
				}
			}
		}
	}

#else
	_tcscpy(XTRAY_INI_FILE, BACKUP_INI_FILE);
#endif

	WritePrivateProfileInt(szUser, _T("SETTINGS"),		g_dwPrefs,		XTRAY_INI_FILE);
	WritePrivateProfileInt(szUser, _T("AOT"),			g_iTime,		XTRAY_INI_FILE);
	WritePrivateProfileInt(szUser, _T("KEY"),			g_hHotKey.key,	XTRAY_INI_FILE);
	WritePrivateProfileInt(szUser, _T("MOD"),			g_hHotKey.mod,	XTRAY_INI_FILE);
	WritePrivateProfileString(szUser, _T("AWAY"),		g_szAway,		XTRAY_INI_FILE);
}

void LoadPrefs()
{
	/**************************************************************************************************/
	/*********************** Our Settings Section *****************************************************/
	/**************************************************************************************************/
	DWORD dBuff = 257;
	TCHAR szUser[257];

	GetUserName(szUser, &dBuff);

	/**************************************************************************************************/
	/*********************** Test if the file exists, If it doesn't, Create It ************************/
	/**************************************************************************************************/
	TCHAR XTRAY_INI_FILE[1024];

#ifdef UNICODE
	char temp[1024];
	TCHAR TEMP_INI_FILE[1024];

	_snprintf(temp, 1024, "%s\\xtray.conf", xchat_get_info(ph, "xchatdir"));
	ConvertString(temp, TEMP_INI_FILE, 1024);

	if(FileExists(TEMP_INI_FILE))
	{
		_tcscpy(XTRAY_INI_FILE, TEMP_INI_FILE);
	}
	else
	{
		if(FileExists(BACKUP_INI_FILE))
		{
			_tcscpy(XTRAY_INI_FILE, BACKUP_INI_FILE);
		}
		else
		{
			HANDLE xTemp;
			DWORD dwBytesTemp;

			if(xTemp = CreateFile(TEMP_INI_FILE, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL))
			{
				if(GetLastError() != ERROR_ALREADY_EXISTS)
				{
					WriteFile(xTemp, _T("\xFF\xFE"), 4, &dwBytesTemp, NULL);
				}

				CloseHandle(xTemp);
			}

			if(FileExists(TEMP_INI_FILE))
			{
				_tcscpy(XTRAY_INI_FILE, TEMP_INI_FILE);
			}
			else
			{
				HANDLE xBackup;
				DWORD dwBytesBackup;

				if(xBackup = CreateFile(TEMP_INI_FILE, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL))
				{
					if(GetLastError() != ERROR_ALREADY_EXISTS)
					{
						WriteFile(xBackup, _T("\xFF\xFE"), 4, &dwBytesBackup, NULL);
					}

					CloseHandle(xBackup);
				}

				if(FileExists(BACKUP_INI_FILE))
				{
					_tcscpy(XTRAY_INI_FILE, BACKUP_INI_FILE);
				}
			}
		}
	}

#else
	_tcscpy(XTRAY_INI_FILE, BACKUP_INI_FILE);
#endif

	/**************************************************************************************************/
	/*************************** Get the value for each of our preferances ****************************/
	/**************************************************************************************************/
	g_dwPrefs = GetPrivateProfileInt(szUser, _T("SETTINGS"),	0,  XTRAY_INI_FILE);

	// backwards compatability
	// also allows us to set defaults if its a new installation
	// disable topic change, channel message and server notice by default
	if(g_dwPrefs == 0)
	{
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("HILIGHT"),		1,  XTRAY_INI_FILE)<<1);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("INVITE"),		1,  XTRAY_INI_FILE)<<2);
		/* g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("TOPIC"),		1,  XTRAY_INI_FILE)<<3); */
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("BANNED"),		1,  XTRAY_INI_FILE)<<4);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("KICKED"),		1,  XTRAY_INI_FILE)<<5);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("CTCP"),		1,  XTRAY_INI_FILE)<<6);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("PMSG"),		1,  XTRAY_INI_FILE)<<7);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("KILLED"),		1,  XTRAY_INI_FILE)<<8);
		/* g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("NOTICE"),		1,  XTRAY_INI_FILE)<<9); */
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("DISCONNECT"),	1,  XTRAY_INI_FILE)<<10);

		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("AOM"),			0,  XTRAY_INI_FILE)<<11);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("TOT"),			1,  XTRAY_INI_FILE)<<12);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("AMAE"),		1,  XTRAY_INI_FILE)<<13);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("OSBWM"),		0,  XTRAY_INI_FILE)<<14);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("UWIOB"),		0,  XTRAY_INI_FILE)<<15);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("KAOI"),		0,  XTRAY_INI_FILE)<<16);
		g_dwPrefs	|= (GetPrivateProfileInt(szUser, _T("MIOC"),		0,  XTRAY_INI_FILE)<<17);
	}
	
	g_hHotKey.key	= GetPrivateProfileInt(szUser, _T("KEY"),	84, XTRAY_INI_FILE);
	g_hHotKey.mod	= GetPrivateProfileInt(szUser, _T("MOD"),	(MOD_CONTROL | MOD_SHIFT),  XTRAY_INI_FILE);
	g_iTime			= GetPrivateProfileInt(szUser, _T("AOT"),	5,  XTRAY_INI_FILE);

	GetPrivateProfileString(szUser, _T("AWAY"), _T(""),	g_szAway, 1024, XTRAY_INI_FILE);
	
	/**************************************************************************************************/
	/******************************** Register our hotkey with windows ********************************/
	/**************************************************************************************************/
	if(g_dwPrefs & (1<<PREF_UWIOB))
	{
		RegisterHotKey(g_hHotkeyWnd, 1, g_hHotKey.mod, g_hHotKey.key);
	}
}

void CheckPrefs(HWND hwnd, int iDlg)
{
	/**************************************************************************************************/
	/**************** save the preferances based on the checkmarks/options ****************************/
	/**************************************************************************************************/
	switch(iDlg)
	{
	case IDD_EVENTS:
		{
			SetOption(hwnd, CHAN_HILIGHT,		CHAN_HILIGHT);
			SetOption(hwnd, CHAN_INVITE,		CHAN_INVITE);
			SetOption(hwnd, CHAN_TOPIC_CHANGE,	CHAN_TOPIC_CHANGE);
			SetOption(hwnd, CHAN_BANNED,		CHAN_BANNED);
			SetOption(hwnd, CHAN_KICKED,		CHAN_KICKED);
			SetOption(hwnd, CTCP_GENERIC,		CTCP_GENERIC);
			SetOption(hwnd, PMSG_RECEIVE,		PMSG_RECEIVE);
			SetOption(hwnd, SERV_KILLED,		SERV_KILLED);
			SetOption(hwnd, SERV_NOTICE,		SERV_NOTICE);
			SetOption(hwnd, SERV_DISCONNECT,	SERV_DISCONNECT);
			SetOption(hwnd, CHAN_MESSAGE,		CHAN_MESSAGE);
		}
		break;
	case IDD_ALERTS:
		{
			SetOption(hwnd, PREF_AMAE,	PREF_AMAE);
			SetOption(hwnd, PREF_OSBWM,	PREF_OSBWM);
			SetOption(hwnd, PREF_UWIOB,	PREF_UWIOB);
			SetOption(hwnd, PREF_KAOI,	PREF_KAOI);
			SetOption(hwnd, PREF_BLINK,	PREF_BLINK);

			/**************************************************************************/
			/**************************************************************************/
			/**************************************************************************/
			TCHAR tTime[512];

			GetWindowText(GetDlgItem(hwnd, IDC_ALERT_TIME), tTime, 511);
			
			g_iTime = _tstoi(tTime);
			
			/**************************************************************************/
			/**************** Get our Hotkey and save it                     **********/
			/**************** then remove the old hotkey and add the new one **********/
			/**************************************************************************/
			DWORD hHotkey;
			hHotkey = SendDlgItemMessage(hwnd, IDC_ALERT_HOTKEY, HKM_GETHOTKEY, 0, 0);
			
			g_hHotKey.key = LOBYTE(hHotkey);
			g_hHotKey.mod = HotkeyfToMod(HIBYTE(hHotkey));
			
			if(IsDlgButtonChecked(hwnd, PREF_UWIOB) == BST_CHECKED)
			{
				UnregisterHotKey(g_hHotkeyWnd, 1);
				RegisterHotKey(g_hHotkeyWnd, 1, g_hHotKey.mod, g_hHotKey.key);
			}
			else
			{
				UnregisterHotKey(g_hHotkeyWnd, 1);
			}

			/*************************************************************************/
			/*********** Get and save the away msg and alert time ********************/
			/*************************************************************************/
		}
		break;
	case IDD_SETTINGS:
		{
			SetOption(hwnd, PREF_AOM, PREF_AOM);
			SetOption(hwnd, PREF_TOT, PREF_TOT);
			SetOption(hwnd, PREF_MIOC, PREF_MIOC);
			SetOption(hwnd, PREF_DNSIT, PREF_DNSIT);

			GetDlgItemText(hwnd, IDC_AWAY_MSG, g_szAway, 511);

			if(g_dwPrefs & (1<<PREF_DNSIT))
			{
				DWORD dwStyle;
				dwStyle = GetWindowLong(g_hXchatWnd, GWL_STYLE);
				dwStyle |= (1<<WS_CHILD);
				SetWindowLongPtr(g_hXchatWnd, GWL_STYLE, (LONG_PTR)dwStyle);
				SetWindowLongPtr(g_hXchatWnd, GWL_HWNDPARENT, (LONG_PTR)g_hHotkeyWnd);
			}
			else
			{
				DWORD dwStyle;
				dwStyle = GetWindowLong(g_hXchatWnd, GWL_STYLE);
				dwStyle &= ~(1<<WS_CHILD);
				SetWindowLongPtr(g_hXchatWnd, GWL_STYLE, (LONG_PTR)dwStyle);
				SetWindowLongPtr(g_hXchatWnd, GWL_HWNDPARENT, NULL);
			}
		}
		break;
	} 
}

void SetDialog(HWND hwnd, int iDlg)
{
	switch(iDlg)
	{
	case IDD_EVENTS:
		{
			SetCheck(hwnd, CHAN_HILIGHT,		CHAN_HILIGHT);
			SetCheck(hwnd, CHAN_INVITE,			CHAN_INVITE);
			SetCheck(hwnd, CHAN_TOPIC_CHANGE,	CHAN_TOPIC_CHANGE);
			SetCheck(hwnd, CHAN_BANNED,			CHAN_BANNED);
			SetCheck(hwnd, CHAN_KICKED,			CHAN_KICKED);
			SetCheck(hwnd, CTCP_GENERIC,		CTCP_GENERIC);
			SetCheck(hwnd, PMSG_RECEIVE,		PMSG_RECEIVE);
			SetCheck(hwnd, SERV_KILLED,			SERV_KILLED);
			SetCheck(hwnd, SERV_NOTICE,			SERV_NOTICE);
			SetCheck(hwnd, SERV_DISCONNECT,		SERV_DISCONNECT);
			SetCheck(hwnd, CHAN_MESSAGE,		CHAN_MESSAGE);
		}
		break;
	case IDD_SETTINGS:
		{
			SetCheck(hwnd, PREF_TOT,	PREF_TOT);
			SetCheck(hwnd, PREF_MIOC,	PREF_MIOC);
			SetCheck(hwnd, PREF_AOM,	PREF_AOM);
			SetCheck(hwnd, PREF_DNSIT,	PREF_DNSIT);

			SetDlgItemText(hwnd, IDC_AWAY_MSG, g_szAway);
		}
		break;
	case IDD_ALERTS:
		{
			
			SetCheck(hwnd, PREF_BLINK,	PREF_BLINK);
			SetCheck(hwnd, PREF_OSBWM,	PREF_OSBWM);
			SetCheck(hwnd, PREF_UWIOB,	PREF_UWIOB);
			SetCheck(hwnd, PREF_KAOI,	PREF_KAOI);

			/**********************************************************/
			/**********************************************************/
			/**********************************************************/
			if(SetCheck(hwnd, PREF_AMAE, PREF_AMAE) == false)
			{
				SetToggle(hwnd, PREF_OSBWM,				PREF_AMAE, TRUE);
				SetToggle(hwnd, PREF_UWIOB,				PREF_AMAE, TRUE);
				SetToggle(hwnd, PREF_KAOI,				PREF_AMAE, TRUE);
				SetToggle(hwnd, IDC_ALERT_TIME,			PREF_AMAE, TRUE);
				SetToggle(hwnd, IDC_ALERT_TIME_TEXT,	PREF_AMAE, TRUE);
				SetToggle(hwnd, IDC_ALERT_HOTKEY,		PREF_AMAE, TRUE);
				SetToggle(hwnd, IDC_ALERT_HOTKEY_TEXT,	PREF_AMAE, TRUE);
			}
			else
			{

				SetToggle(hwnd, IDC_ALERT_HOTKEY,		PREF_UWIOB, TRUE);
				SetToggle(hwnd, IDC_ALERT_HOTKEY_TEXT,	PREF_UWIOB, TRUE);
				SetToggle(hwnd, IDC_ALERT_TIME,			PREF_KAOI, FALSE);
				SetToggle(hwnd, IDC_ALERT_TIME_TEXT,	PREF_KAOI, FALSE);
			}

			/**********************************************************/
			/**********************************************************/
			/**********************************************************/
			TCHAR tTime[255];
			SendDlgItemMessage(hwnd, IDC_ALERT_TIME,	WM_SETTEXT, 0, (LPARAM)_itot(g_iTime, tTime, 10));
			SendDlgItemMessage(hwnd, IDC_ALERT_HOTKEY,	HKM_SETHOTKEY, MAKEWORD(g_hHotKey.key, ModToHotkeyf(g_hHotKey.mod)), 0);
		}
		break;
	}
}

int SetCheck(HWND hDialog, unsigned int uiCheckBox, unsigned int uiPref)
{
	if((g_dwPrefs & (1<<uiPref)))
	{
		CheckDlgButton(hDialog, uiCheckBox, BST_CHECKED);
		return 1;
	}
	else
	{
		CheckDlgButton(hDialog, uiCheckBox, BST_UNCHECKED);
		return 0;
	}

	return 0;
}

int SetToggle(HWND hDialog, unsigned int uiCheckBox, unsigned int uiTestbox, bool offeqoff)
{
	/**************************************************************************************************/
	/*********************** if(true) then if option is off turn toggle off ***************************/
	/*********************** if(false) then if option is off turn toggle on ***************************/
	/**************************************************************************************************/
	if(offeqoff)
	{
		if(IsDlgButtonChecked(hDialog, uiTestbox) == BST_CHECKED)
		{
			EnableWindow(GetDlgItem(hDialog, uiCheckBox), TRUE);
			return 1;
		}
		else
		{
			EnableWindow(GetDlgItem(hDialog, uiCheckBox), FALSE);
			return 0;
		}
	}
	else
	{
		if(IsDlgButtonChecked(hDialog, uiTestbox) == BST_CHECKED)
		{
			EnableWindow(GetDlgItem(hDialog, uiCheckBox), FALSE);
			return 1;
		}
		else
		{
			EnableWindow(GetDlgItem(hDialog, uiCheckBox), TRUE);
			return 0;
		}
	}

	return 0;
}

int SetOption(HWND hDialog, unsigned int uiCheckBox, unsigned int uiPref)
{
	if(IsDlgButtonChecked(hDialog, uiCheckBox) == BST_CHECKED)
	{
		g_dwPrefs |= (1<<uiPref);
	}
	else
	{
		g_dwPrefs &= ~(1<<uiPref);
	}

	return (g_dwPrefs & (1<<uiPref));
}

int WritePrivateProfileIntA(char *szSection, char *szItem, int iData, char *szPath)
{
	char szData[33];
	itoa(iData, szData, 10);
	return WritePrivateProfileStringA(szSection, szItem, szData, szPath);
}

int WritePrivateProfileIntW(wchar_t *wszSection, wchar_t *wszItem, int iData, wchar_t *wszPath)
{
	wchar_t wszData[33];
	_itow(iData, wszData, 10);
	return WritePrivateProfileStringW(wszSection, wszItem, wszData, wszPath);
}

// For cleanup ( Closing windows and the such )
void HoldClose()
{
	HANDLE hcThread;
	DWORD dwThreadID;
	hcThread = CreateThread(NULL, 0, HoldCloseThread, 0, 0, &dwThreadID);
}

DWORD WINAPI HoldCloseThread(LPVOID lpParam)
{
	Sleep(1000);
	PostMessage(g_hXchatWnd, WM_CLOSE, 0, 0);
	return 0;
}

bool FileExists(TCHAR *file)
{
	HANDLE hTemp = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int nRet = GetLastError();
	CloseHandle(hTemp);

	if(nRet == 0)
	{
		return true;
	}
	else
	{
		return false;
	}

	return false;
}

void ConvertString(const char *in, wchar_t *out, int size)
{
	MultiByteToWideChar(CP_UTF8, 0, in,  -1, out, size);
}

void ConvertString(const wchar_t *in, char *out, int size)
{
	WideCharToMultiByte(CP_UTF8, 0, in, (size - 1), out, size, NULL, NULL);
}

void ConvertString(const char *in, char *out, int size)
{
	strncpy(out, in, size);
}

void ErrorDebug(LPTSTR lpszFunction)
{ 
    TCHAR szBuf[80]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    _stprintf(szBuf, 
        _T("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
 
    MessageBox(NULL, szBuf, _T("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
}

