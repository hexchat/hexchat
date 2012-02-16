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

#define _WIN32_IE 0x0601

#include <windows.h>
#include <list>
#include <string>
#include <shobjidl.h>

#include "xchat-plugin.h"
#include "resource.h"
#include "callbacks.h"
#include "utility.h"
#include "xtray.h"
#include "sdTray.h"
#include "sdAlerts.h"

/*****************************************************/
/**** Don't want to pollute the namespace do we? *****/
/*****************************************************/
std::list<xchat_hook *> g_vHooks;

/*****************************************************/
/************ Global Identifiers *********************/
/*****************************************************/
WNDPROC g_hOldProc;
xchat_plugin *ph;

/*****************************************************/
/***************** Resources *************************/
/*****************************************************/
HWND	g_hXchatWnd;
HWND	g_hHotkeyWnd;
HWND	g_hPrefDlg;
HMENU	g_hTrayMenu;
HICON	g_hIcons[11];
HANDLE	g_hInstance;
/*****************************************************/
/***************** Preferences ***********************/
/*****************************************************/
unsigned int g_dwPrefs;
TCHAR	g_szAway[512];
int		g_iTime;


BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpVoid)
{
	if((fdwReason == DLL_PROCESS_ATTACH) || (fdwReason == DLL_THREAD_ATTACH))
	{
		g_hInstance = hModule;
	}

	return TRUE;
}

int xchat_plugin_init(xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name	= "X-Tray";
	*plugin_desc	= "Minimize XChat to the Windows system tray";
	*plugin_version = "1.2.4";

	/***************************************************************************************************************************/
	/************************* Load our preferances from xTray.ini *************************************************************/
	/***************************************************************************************************************************/
	LoadPrefs();

	/***************************************************************************************************************************/
	/************************* Finds the xChat window and saves it for later use ***********************************************/
	/***************************************************************************************************************************/
	g_hXchatWnd = (HWND)xchat_get_info(ph, "win_ptr");

	if(g_hXchatWnd == NULL)
	{
		EnumThreadWindows(GetCurrentThreadId(), EnumWindowsProc, 0);
	}

	g_hOldProc	= (WNDPROC)GetWindowLongPtr(g_hXchatWnd, GWLP_WNDPROC);
	SetWindowLongPtr(g_hXchatWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);

	/***************************************************************************************************************************/	
	/************************* Grab the xChat Icon, Load our menu, create the window to receive the hotkey messages  ***********/
	/************************* and register the windows message so we know if explorer crashes                       ***********/
	/***************************************************************************************************************************/
	g_hTrayMenu		= GetSubMenu(LoadMenu((HINSTANCE)g_hInstance, MAKEINTRESOURCE(IDR_TRAY_MENU)), 0);
	g_hHotkeyWnd	= CreateDialog((HINSTANCE)g_hInstance, MAKEINTRESOURCE(IDD_ALERT), NULL,		(DLGPROC)HotKeyProc);
	g_hPrefDlg		= CreateDialog((HINSTANCE)g_hInstance, MAKEINTRESOURCE(IDD_PREF),  g_hXchatWnd, (DLGPROC)PrefProc);

	g_hIcons[0]	= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_XCHAT),			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[1]	= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_CHANMSG),		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[2]	= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_HIGHLIGHT),		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[5]	= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_BANNED),			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[6]	= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_KICKED),			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[8]	= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_PMSG),			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[10]= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_SNOTICE),		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	g_hIcons[11]= (HICON)LoadImage((HINSTANCE)g_hInstance, MAKEINTRESOURCE(ICO_DISCONNECTED),	IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	/***************************************************************************************************************************/
	/************************* Add our icon to the tray ************************************************************************/
	/***************************************************************************************************************************/
	char szVersion[64];
	_snprintf(szVersion, 64, "XChat-WDK %s", xchat_get_info(ph, "wdk_version"));
	AddIcon(g_hXchatWnd, 1, g_hIcons[0], szVersion, (NIF_ICON | NIF_MESSAGE | NIF_TIP), WM_TRAYMSG);

	/***************************************************************************************************************************/
	/***************************************************************************************************************************/
	/***************************************************************************************************************************/
	if(g_dwPrefs & (1<<PREF_DNSIT))
	{
		DWORD dwStyle;
		dwStyle = GetWindowLong(g_hXchatWnd, GWL_STYLE);
		dwStyle |= (1<<WS_CHILD);
		SetWindowLongPtr(g_hXchatWnd, GWL_STYLE, (LONG_PTR)dwStyle);
		SetWindowLongPtr(g_hXchatWnd, GWL_HWNDPARENT, (LONG_PTR)g_hHotkeyWnd);
	}

	/***************************************************************************************************************************/
	/************************* Set our hooks and save them for later so we can unhook them *************************************/
	/***************************************************************************************************************************/
	g_vHooks.push_back(xchat_hook_print(ph, "Channel Msg Hilight",			XCHAT_PRI_NORM, event_cb,	(void *)CHAN_HILIGHT));
	g_vHooks.push_back(xchat_hook_print(ph, "Channel Message",				XCHAT_PRI_NORM, event_cb,	(void *)CHAN_MESSAGE));
	g_vHooks.push_back(xchat_hook_print(ph, "Topic Change",					XCHAT_PRI_NORM, event_cb,	(void *)CHAN_TOPIC_CHANGE));
	g_vHooks.push_back(xchat_hook_print(ph, "Channel Action Hilight",		XCHAT_PRI_NORM, event_cb,	(void *)CHAN_HILIGHT));
	g_vHooks.push_back(xchat_hook_print(ph, "Channel INVITE",				XCHAT_PRI_NORM, event_cb,	(void *)CHAN_INVITE));
	g_vHooks.push_back(xchat_hook_print(ph, "You Kicked",					XCHAT_PRI_NORM, event_cb,	(void *)CHAN_KICKED));
	g_vHooks.push_back(xchat_hook_print(ph, "Banned",						XCHAT_PRI_NORM, event_cb,	(void *)CHAN_BANNED));
	g_vHooks.push_back(xchat_hook_print(ph, "CTCP Generic",					XCHAT_PRI_NORM, event_cb,	(void *)CTCP_GENERIC));
	g_vHooks.push_back(xchat_hook_print(ph, "Private Message",				XCHAT_PRI_NORM, event_cb,	(void *)PMSG_RECEIVE));
	g_vHooks.push_back(xchat_hook_print(ph, "Private Message to Dialog",	XCHAT_PRI_NORM, event_cb,	(void *)PMSG_RECEIVE));
	g_vHooks.push_back(xchat_hook_print(ph, "Disconnected",					XCHAT_PRI_NORM, event_cb,	(void *)SERV_DISCONNECT));
	g_vHooks.push_back(xchat_hook_print(ph, "Killed",						XCHAT_PRI_NORM, event_cb,	(void *)SERV_KILLED));
	g_vHooks.push_back(xchat_hook_print(ph, "Notice",						XCHAT_PRI_NORM, event_cb,	(void *)SERV_NOTICE));
	g_vHooks.push_back(xchat_hook_command(ph, "tray_alert",					XCHAT_PRI_NORM, command_cb,	"Create an Alert", NULL));

	return 1;
}

int xchat_plugin_deinit(xchat_plugin *plugin_handle)
{
	/******************************************/
	/****** Remove the Icon from the tray *****/
	/******************************************/
	StopBlink(g_hXchatWnd, 1, g_hIcons[0]);
	RemoveIcon(g_hXchatWnd, 1);
	
	/*******************************************/
	/*******************************************/
	/*******************************************/
	if(g_dwPrefs & (1<<PREF_DNSIT))
	{
		DWORD dwStyle;
		dwStyle = GetWindowLong(g_hXchatWnd, GWL_STYLE);
		dwStyle &= ~(1<<WS_CHILD);
		SetWindowLongPtr(g_hXchatWnd, GWL_STYLE, (LONG_PTR)dwStyle);
		SetWindowLongPtr(g_hXchatWnd, GWL_HWNDPARENT, NULL);
	}

	/******************************************/
	/****** Unload our resources **************/
	/******************************************/
	DestroyMenu(g_hTrayMenu);

	for(int i = 0; i <= 11; i++)
	{
		DestroyIcon(g_hIcons[i]);
	}

	/******************************************/
	/****** Remove our window hook ************/
	/******************************************/
	SetWindowLongPtr(g_hXchatWnd, GWLP_WNDPROC, (LONG_PTR)g_hOldProc);

	/******************************************/
	/****** Remove our hotkey, and destroy ****/
	/****** the window that receives its   ****/
	/****** messages                       ****/
	/******************************************/
	UnregisterHotKey(g_hHotkeyWnd, 1);
	DestroyWindow(g_hHotkeyWnd);
	DestroyWindow(g_hPrefDlg);

	/******************************************/
	/************* Clean up Isle 7 ************/
	/******************************************/
	if(sdAlertNum())
	{
		sdCloseAlerts();
	}
	/******************************************/
	/****** remove our xchat_hook_*s **********/
	/******************************************/
	while(!g_vHooks.empty())
	{
		if(g_vHooks.back() != NULL)
		{
			xchat_unhook(ph, g_vHooks.back());
		}
		g_vHooks.pop_back();
	}

	return 1;
}
