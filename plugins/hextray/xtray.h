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

#ifndef _XTRAY_H
#define _XTRAY_H

/******************** Globals *************************/
extern HWND				g_hXchatWnd;
extern HWND				g_hHotkeyWnd;
extern HWND				g_hPrefDlg;
extern HMENU			g_hTrayMenu;
extern HICON			g_hIcons[11];
extern HANDLE			g_hInstance;
extern unsigned int		g_dwPrefs;
extern TCHAR			g_szAway[512];
extern int				g_iTime;
extern WNDPROC			g_hOldProc;
extern struct _xchat_plugin *ph;
/******************************************************/

/******************** Messages ************************/
#define WM_TRAYMSG WM_APP
/******************************************************/

/********************* Events *************************/
#define CHAN_HILIGHT			1
#define CHAN_INVITE				2
#define CHAN_TOPIC_CHANGE		3
#define CHAN_BANNED				4
#define CHAN_KICKED				5

#define CTCP_GENERIC			6
#define PMSG_RECEIVE			7

#define SERV_KILLED				8
#define SERV_NOTICE				9
#define SERV_DISCONNECT			10

/* new events */
#define CHAN_MESSAGE			21

#define PREF_AOM				11 // away on minimize
#define PREF_TOT				12 // Tray on Taskbar
#define PREF_AMAE				13 // alert me about events
#define PREF_OSBWM				14 // Only Show Balloon When Minimized
#define PREF_UWIOB				15 // Use Window Instead of Balloon
#define PREF_KAOI				16 // Keep alerts open indefinately
#define PREF_MIOC				17 // Minimize instead of close
#define PREF_BLINK				18 // blink icon
#define PREF_CICO				19 // change icon - not implemented
#define PREF_DNSIT				20 // Do not show in taskbar
/******************************************************/
#endif

#ifdef _WIN64
/* use replacement with the same value, and use SetWindowLongPtr instead
   of SetWindowLong. more info:

   http://msdn.microsoft.com/en-us/library/ms633591.aspx
   http://msdn.microsoft.com/en-us/library/ms644898.aspx */
#define GWL_HWNDPARENT GWLP_HWNDPARENT
#endif
