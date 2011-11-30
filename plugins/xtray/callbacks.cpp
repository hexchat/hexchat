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
#include <stdio.h>
#include <commctrl.h>
#include <tchar.h>

#include "xchat-plugin.h"
#include "utility.h"
#include "xtray.h"
#include "xchat.h"
#include "callbacks.h"
#include "resource.h"
#include "sdTray.h"
#include "sdAlerts.h"

HWND	g_hPrefTabEvents;
HWND	g_hPrefTabSettings;
HWND	g_hPrefTabAlerts;
HWND	g_hPrefTabAbout;
bool	g_bCanQuit;
int		g_iIsActive = 1;


BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	TCHAR szTitle[10];
	GetWindowText(hWnd, szTitle, 9);

	if(_tcsstr(szTitle, _T("XChat [")))
	{
		g_hXchatWnd = hWnd;
		return false;
	}
	
	return true;
}

/***********************************************************************************************/
/******* our xchat event call back, get the name and info for each event and save it ***********/
/******* for our alerts later														***********/
/***********************************************************************************************/
int event_cb(char *word[], void *userdata)
{
	int iEvent = (int)userdata;

	if(iEvent > 10 && iEvent != 21)
		return XCHAT_EAT_NONE;

	/***************************************************************************************/
	/***** if the window is minimized or if we're allowed to show alerts when its not	 **/
	/***** and if the option to show the specified alert is true and if we're even		**/
	/***** allowed to show alerts at all then we show them (a bit confusing but it works) **/
	/***************************************************************************************/
	if(((g_iIsActive == 0) || !(g_dwPrefs & (1<<PREF_OSBWM))) && (g_dwPrefs & (1<<PREF_AMAE)) && (g_dwPrefs & (1<<iEvent)))
	{	
		/*********************************/
		/*********** Our Buffers *********/
		/*********************************/
		char			szInfo[512];
		char			szName[64];
		DWORD			dwInfoFlags;
		int iTime		= g_iTime*1000;
		char *szTemp	= NULL;

		if(g_dwPrefs & (1<<PREF_KAOI))
		{
			iTime = 0;
		}

		switch(iEvent)
		{
		case CHAN_HILIGHT:
			_snprintf(szInfo, 512, "%s:\r\n%s", word[1], word[2]);
			_snprintf(szName, 64, "Highlight: %s", xchat_get_info (ph, "channel"));
			dwInfoFlags = NIIF_INFO;
			break;
		case CHAN_MESSAGE:
			_snprintf(szInfo, 512, "%s:\r\n%s", word[1], word[2]);
			_snprintf(szName, 64, "Channel Message: %s", xchat_get_info (ph, "channel"));
			dwInfoFlags = NIIF_INFO;
			break;
		case CHAN_TOPIC_CHANGE:
			_snprintf(szInfo, 512, "%s has changed the topic to %s", word[1], word[2]);
			_snprintf(szName, 64, "Topic Change: %s", word[3]);
			dwInfoFlags = NIIF_INFO;
			break;
		case CHAN_INVITE:
			_snprintf(szInfo, 512, "%s has invited you into %s", word[1], word[2]);
			_snprintf(szName, 64, "Invite");
			dwInfoFlags = NIIF_INFO;
			break;
		case CHAN_KICKED:
			_snprintf(szInfo, 512, "Kicked from %s by %s:\r\n%s", word[2], word[3], word[4]);
			_snprintf(szName, 64, "Kick");
			dwInfoFlags = NIIF_WARNING;
			break;
		case CHAN_BANNED:
			_snprintf(szInfo, 512, "Cannot join #%s You are banned.", word[1]);
			_snprintf(szName, 64, "Banned");
			dwInfoFlags = NIIF_WARNING;
			break;
		case CTCP_GENERIC:
			_snprintf(szInfo, 512, "%s:\r\nCTCP %s", word[2], word[1]);
			_snprintf(szName, 64, "CTCP");
			dwInfoFlags = NIIF_INFO;
			break;
		case PMSG_RECEIVE:
			_snprintf(szInfo, 512, "%s:\r\n%s", word[1], word[2]);
			_snprintf(szName, 64, "Private Message");
			dwInfoFlags = NIIF_INFO;
			break;
		case SERV_DISCONNECT:
			_snprintf(szInfo, 512, "Disconnected\r\nError: %s", word[1]);
			_snprintf(szName, 64, "Disconnect");
			dwInfoFlags = NIIF_ERROR;
			break;
		case SERV_KILLED:
			_snprintf(szInfo, 512, "Killed(%s(%s))", word[1], word[2]);
			_snprintf(szName, 64, "Server Admin has killed you");
			dwInfoFlags = NIIF_ERROR;
			break;
		case SERV_NOTICE:
			_snprintf(szInfo, 512, "Notice:\r\n%s: %s", word[1], word[2]);
			_snprintf(szName, 64, "Notice");
			dwInfoFlags = NIIF_INFO;
			break;
		case 11:
			_snprintf(szInfo, 512, ":\r\n%s: %s", word[1], word[2]);
			_snprintf(szName, 64, "Notice");
			dwInfoFlags = NIIF_INFO;
			break;
		}

		/**************************************************************************************/
		/***** Use windows instead of balloons, and if its a window should we keep it open ****/
		/***** indefinately?															   ****/
		/**************************************************************************************/
		szTemp = xchat_strip_color(szInfo);

		if(g_dwPrefs & (1<<PREF_UWIOB))
		{
			sdSystemAlert((HINSTANCE)g_hInstance, IDD_ALERT, szTemp, szName, iTime);
		}
		else
		{
			ShowBalloon(g_hXchatWnd, 1, szTemp, szName, iTime, dwInfoFlags);
		}

		free(szTemp);
	}

	if(g_dwPrefs & (1<<PREF_BLINK) && (g_dwPrefs & (1<<iEvent)))
	{
		BlinkIcon(g_hXchatWnd, 1, g_hIcons[0], g_hIcons[(iEvent+1)], 700, 5);
	}

	/***********************************/
	/***** pass the events to xchat ****/
	/***********************************/
	return XCHAT_EAT_NONE;
}

int command_cb(char *word[], char *word_eol[], void *userdata)
{
	char szInfo[512];
	char *szTemp	= NULL;
	int iTime		= g_iTime*1000;

	_snprintf(szInfo, 512, word_eol[2]);
	szTemp = xchat_strip_color(szInfo);

	if(g_dwPrefs & (1<<PREF_KAOI))
	{
		iTime = 0;
	}

	if(((g_iIsActive == 0) || !(g_dwPrefs & (1<<PREF_OSBWM))) && (g_dwPrefs & (1<<PREF_AMAE)))
	{
		if(g_dwPrefs & (1<<PREF_UWIOB))
		{
			sdSystemAlert((HINSTANCE)g_hInstance, IDD_ALERT, szTemp, "Alert", iTime);
		}
		else
		{
			ShowBalloon(g_hXchatWnd, 1, szTemp, "Alert", iTime, NIIF_INFO);
		}
	}

	free(szTemp);

	return XCHAT_EAT_ALL;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_CLOSE:
		{
			if((g_dwPrefs & (1<<PREF_MIOC)) && (g_bCanQuit == false))
			{
				/*******************************************/
				/**** to autoaway or not to autoaway...  ***/
				/*******************************************/
				if(g_dwPrefs & (1<<PREF_AOM))
				{
					xchat_globally_away(g_szAway);
				}

				/**************************************************/
				/**** Win32 API call to hide the window and	     **/
				/**** save the fact that its minimized for later **/
				/**************************************************/
				g_iIsActive = 0;
				ShowWindow(hWnd, SW_HIDE);

				return 0;
			}
			else
			{
				if(g_hPrefDlg != NULL)
				{
					DestroyWindow(g_hPrefDlg);
				}

				StopBlink(hWnd, 1, g_hIcons[0]);
				
				if(sdAlertNum())
				{
					sdCloseAlerts();
					HoldClose();
					return 0;
				}
			}
		}
		break;
	case WM_SIZE:
		{
			/******************************************/
			/***** User wants to minimize xChat, ******/
			/***** are we allowed to go to tray? ******/
			/******************************************/
			if((g_dwPrefs & (1<<PREF_TOT)) && (wparam == SIZE_MINIMIZED))
			{
				/*******************************************/
				/**** to autoaway or not to autoaway...  ***/
				/*******************************************/
				if(g_dwPrefs & (1<<PREF_AOM))
				{
					xchat_globally_away(g_szAway);
				}

				/**************************************************/
				/**** Win32 API call to hide the window and	     **/
				/**** save the fact that its minimized for later **/
				/**************************************************/
				g_iIsActive = 0;
				ShowWindow(hWnd, SW_HIDE);
			}
		}
		break;
	/**********************************/
	/*** user clicked the tray icon ***/
	/**********************************/
	case WM_TRAYMSG:
		{
			switch(lparam)
			{
			case WM_LBUTTONDOWN:
				{
					if(!g_iIsActive)
					{
						/*********************************************************/
						/*** 0: its hiden, restore it and show it, if autoaway ***/
						/*** is on, set us as back							 ***/
						/*********************************************************/
						SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
						SetForegroundWindow(hWnd);
						g_iIsActive = 1;

						if(g_dwPrefs & (1<<PREF_AOM))
						{
							xchat_globally_back();
						}
					}
					else
					{
						SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
					}
				}
				break;
			case WM_RBUTTONDOWN:
				{
					/******************************************/
					/*** user wants to see the menu find out **/
					/*** where the mouse is and show it	  **/
					/******************************************/
					POINT pt;
					int iRet;

					GetCursorPos(&pt);
					SetForegroundWindow(hWnd);

					ModifyMenu(g_hTrayMenu, 2, (MF_POPUP | MF_BYPOSITION), (UINT)setServerMenu(), _T("Away"));

					Sleep(175);

					iRet = TrackPopupMenuEx(g_hTrayMenu, (TPM_RETURNCMD | TPM_LEFTALIGN), pt.x, pt.y, hWnd, NULL);

					/***********************************/
					/*** nRet is the users selection, **/
					/*** process it				   **/
					/***********************************/
					sdTrayProc(hWnd, iRet);
				}
				break;
			}
		}
		break;
	default:
		{
			/*****************************************************/
			/*** the taskbar has been restarted, re-add our icon */
			/*****************************************************/
			if(msg == RegisterWindowMessage(_T("TaskbarCreated")))
			{
				char szVersion[64];
				_snprintf(szVersion, 64, "XChat-WDK [%s]", xchat_get_info(ph, "version"));
				AddIcon(g_hXchatWnd, 1, g_hIcons[0], szVersion, (NIF_ICON | NIF_MESSAGE | NIF_TIP), WM_TRAYMSG);
			}
		}
		break;
	}

	return CallWindowProc(g_hOldProc, hWnd, msg, wparam, lparam);
}

/****************************************************/
/*** process messages from the tray menu ************/
/****************************************************/
LRESULT CALLBACK sdTrayProc(HWND hWnd, int msg)
{
	switch(msg)
	{
	case ACT_EXIT:
		{
			g_bCanQuit = true;
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	case ACT_RESTORE:
		{
			/***********************************************/
			/** user wants us to restore the xchat window **/
			/** and of autoaway is on, set as back		**/
			/***********************************************/
			SendMessage(g_hXchatWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			SetForegroundWindow(hWnd);
			
			if((!g_iIsActive) && (g_dwPrefs & (1<<PREF_AOM)))
			{
				xchat_globally_back();
				g_iIsActive = 1;
			}
		}
		break;
	case ACT_SETTINGS:
		{
			ShowWindow(g_hPrefDlg, SW_SHOW);
		}
		break;
	case ACT_AWAY:
		{
			xchat_globally_away(g_szAway);
		}
		break;
	case ACT_BACK:
		{
			xchat_globally_back();
		}
		break;
	default:
		{
			if(msg > 0)
			{
				xchat_set_context(ph, xchat_find_server(msg-1));

				if(!xchat_get_info(ph, "away"))
				{
					xchat_away(g_szAway);
				}
				else
				{
					xchat_back();
				}
			}
		}
		break;
	}

	return 1;
}

int CALLBACK PrefProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			TCITEM tci1;
			TCITEM tci2;
			TCITEM tci3;
			TCITEM tci4;

			tci1.mask		= TCIF_TEXT;
			tci1.pszText	= _T("Settings");
			tci1.cchTextMax	= strlen("Settings");
			SendDlgItemMessage(hWnd, IDC_TAB_CONTROL, TCM_INSERTITEM, 0, (LPARAM)&tci1);

			tci2.mask		= TCIF_TEXT;
			tci2.pszText	= _T("Alerts");
			tci2.cchTextMax	= strlen("Alerts");
			SendDlgItemMessage(hWnd, IDC_TAB_CONTROL, TCM_INSERTITEM, 1, (LPARAM)&tci2);

			tci3.mask		= TCIF_TEXT;
			tci3.pszText	= _T("Events");
			tci3.cchTextMax	= strlen("Events");
			SendDlgItemMessage(hWnd, IDC_TAB_CONTROL, TCM_INSERTITEM, 2, (LPARAM)&tci3);

			tci4.mask		= TCIF_TEXT;
			tci4.pszText	= _T("About");
			tci4.cchTextMax	= strlen("About");
			SendDlgItemMessage(hWnd, IDC_TAB_CONTROL, TCM_INSERTITEM, 3, (LPARAM)&tci4);


			/***********************************************************************************/
			/***********************************************************************************/
			/***********************************************************************************/

			g_hPrefTabSettings	= CreateDialog((HINSTANCE)g_hInstance,
									MAKEINTRESOURCE(IDD_SETTINGS),
									hWnd,		
									(DLGPROC)SettingsProc);
			SetDialog(g_hPrefTabSettings,	IDD_SETTINGS);

			g_hPrefTabAlerts	= CreateDialog((HINSTANCE)g_hInstance,
									MAKEINTRESOURCE(IDD_ALERTS),
									hWnd,
									(DLGPROC)AlertsProc);
			SetDialog(g_hPrefTabAlerts,		IDD_ALERTS);

			g_hPrefTabEvents	= CreateDialog((HINSTANCE)g_hInstance,
									MAKEINTRESOURCE(IDD_EVENTS),
									hWnd,		
									(DLGPROC)EventsProc);
			SetDialog(g_hPrefTabEvents,		IDD_EVENTS);

			g_hPrefTabAbout		= CreateDialog((HINSTANCE)g_hInstance,
									MAKEINTRESOURCE(IDD_ABOUT),
									hWnd,
									(DLGPROC)AboutProc);
		}
		break;
	case WM_SHOWWINDOW:
		{
			if(wparam)
			{
				SendDlgItemMessage(hWnd, IDC_TAB_CONTROL, TCM_SETCURSEL, 0, 0);
				ShowWindow(g_hPrefTabSettings,	SW_SHOW);
				ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
				ShowWindow(g_hPrefTabEvents,	SW_HIDE);
				ShowWindow(g_hPrefTabAbout,		SW_HIDE);
			}
		}
		break;
	case WM_NOTIFY:
		{
			NMHDR *pData = (NMHDR *)lparam;

			switch(pData->code)
			{
			case TCN_SELCHANGE:
				{
					switch(SendDlgItemMessage(hWnd, IDC_TAB_CONTROL, TCM_GETCURSEL, 0, 0))
					{
					case 0:
						{
							ShowWindow(g_hPrefTabSettings,	SW_SHOW);
							ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
							ShowWindow(g_hPrefTabEvents,	SW_HIDE);
							ShowWindow(g_hPrefTabAbout,		SW_HIDE);
						}
						break;
					case 1:
						{
							ShowWindow(g_hPrefTabSettings,	SW_HIDE);
							ShowWindow(g_hPrefTabAlerts,	SW_SHOW);
							ShowWindow(g_hPrefTabEvents,	SW_HIDE);
							ShowWindow(g_hPrefTabAbout,		SW_HIDE);
						}
						break;
					case 2:
						{
							ShowWindow(g_hPrefTabSettings,	SW_HIDE);
							ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
							ShowWindow(g_hPrefTabEvents,	SW_SHOW);
							ShowWindow(g_hPrefTabAbout,		SW_HIDE);
						}
						break;
					case 3:
						{
							ShowWindow(g_hPrefTabSettings,	SW_HIDE);
							ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
							ShowWindow(g_hPrefTabEvents,	SW_HIDE);
							ShowWindow(g_hPrefTabAbout,		SW_SHOW);
						}
						break;
					}
				}
				break;
			}
		}
		break;
	case WM_CLOSE:
		{
			ShowWindow(g_hPrefTabEvents,	SW_HIDE);
			ShowWindow(g_hPrefTabSettings,	SW_HIDE);
			ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
			ShowWindow(g_hPrefTabAbout,		SW_HIDE);
			ShowWindow(hWnd,				SW_HIDE);
			return TRUE;
		}
		break;
	case WM_COMMAND:
		{
			switch(wparam)
			{
			case IDC_PREF_OK:
				{
					CheckPrefs(g_hPrefTabEvents,	IDD_EVENTS);
					CheckPrefs(g_hPrefTabSettings,	IDD_SETTINGS);
					CheckPrefs(g_hPrefTabAlerts,	IDD_ALERTS);

					SavePrefs(0);

					ShowWindow(g_hPrefTabEvents,	SW_HIDE);
					ShowWindow(g_hPrefTabSettings,	SW_HIDE);
					ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
					ShowWindow(g_hPrefTabAbout,		SW_HIDE);
					ShowWindow(hWnd,				SW_HIDE);
					return TRUE;
				}
				break;
			case IDC_PREF_CANCEL:
				{
					ShowWindow(g_hPrefTabEvents,	SW_HIDE);
					ShowWindow(g_hPrefTabSettings,	SW_HIDE);
					ShowWindow(g_hPrefTabAlerts,	SW_HIDE);
					ShowWindow(g_hPrefTabAbout,		SW_HIDE);
					ShowWindow(hWnd,				SW_HIDE);
					return TRUE;
				}
				break;
			case IDC_PREF_APPLY:
				{
					CheckPrefs(g_hPrefTabEvents,	IDD_EVENTS);
					CheckPrefs(g_hPrefTabSettings,	IDD_SETTINGS);
					CheckPrefs(g_hPrefTabAlerts,	IDD_ALERTS);

					SavePrefs(0);
					return FALSE;
				}
				break;
			}
		}
		break;
	case WM_DESTROY:
		{
			SendMessage(g_hPrefTabEvents,	WM_CLOSE, 0, 0);
			SendMessage(g_hPrefTabSettings,	WM_CLOSE, 0, 0);
			SendMessage(g_hPrefTabAbout,	WM_CLOSE, 0, 0);
			SendMessage(g_hPrefTabAlerts,	WM_CLOSE, 0, 0);
		}
		break;
	}

	return FALSE;
}

/****************************************************/
/****************************************************/
/****************************************************/
LRESULT CALLBACK AlertsProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_CLOSE:
		{
			DestroyWindow(hWnd);
			return TRUE;
			break;
		}
		break;
	case WM_COMMAND:
		{
			switch(LOWORD(wparam))
			{
			case PREF_AMAE:
				{
					SetToggle(hWnd, PREF_OSBWM,			PREF_AMAE, TRUE);
					SetToggle(hWnd, PREF_UWIOB,			PREF_AMAE, TRUE);
					SetToggle(hWnd, PREF_KAOI,			PREF_AMAE, TRUE);
					
					if(IsDlgButtonChecked(hWnd, PREF_AMAE))
					{
						SetToggle(hWnd, IDC_ALERT_HOTKEY,		PREF_UWIOB, TRUE);
						SetToggle(hWnd, IDC_ALERT_HOTKEY_TEXT,	PREF_UWIOB, TRUE);
						SetToggle(hWnd, IDC_ALERT_TIME,			PREF_KAOI, FALSE);
						SetToggle(hWnd, IDC_ALERT_TIME_TEXT,	PREF_KAOI, FALSE);
					}
					else
					{
						SetToggle(hWnd, IDC_ALERT_HOTKEY,		PREF_AMAE, TRUE);
						SetToggle(hWnd, IDC_ALERT_HOTKEY_TEXT,	PREF_AMAE, TRUE);
						SetToggle(hWnd, IDC_ALERT_TIME,			PREF_AMAE, TRUE);
						SetToggle(hWnd, IDC_ALERT_TIME_TEXT,	PREF_AMAE, TRUE);
					}
				}
				break;
			case PREF_UWIOB:
				{
					SetToggle(hWnd, IDC_ALERT_HOTKEY,		PREF_UWIOB, TRUE);
					SetToggle(hWnd, IDC_ALERT_HOTKEY_TEXT,	PREF_UWIOB, TRUE);
				}
				break;
			case PREF_KAOI:
				{
					SetToggle(hWnd, IDC_ALERT_TIME,			PREF_KAOI, FALSE);
					SetToggle(hWnd, IDC_ALERT_TIME_TEXT,	PREF_KAOI, FALSE);
				}
				break;
			}
			break;
		}
	}

	return FALSE;
}

/****************************************************/
/****************************************************/
/****************************************************/
LRESULT CALLBACK AboutProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(msg == WM_CLOSE)
	{
		DestroyWindow(hWnd);
		return true;
	}

	return FALSE;
}

/*****************************************************/
/** Process the events for our event dialog **********/
/*****************************************************/
LRESULT CALLBACK EventsProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(msg == WM_CLOSE)
	{
		DestroyWindow(hWnd);
		return true;
	}

	return FALSE;
}

/*****************************************************/
/** Process the events for our settings dialog this **/
/** is alot more complicated because options are	**/
/** enabled/disabled based on the state of others   **/
/*****************************************************/
LRESULT CALLBACK SettingsProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(msg == WM_CLOSE)
	{
		DestroyWindow(hWnd);
		return true;
	}

	return FALSE;
}

/*****************************************************/
/** this is the hotkey message  processing function **/
/** this window is always open and ready to be told **/
/** if someone has hit the hotkey, if they did, we  **/
/** need to close out all of the tray alerts, for   **/
/** this I wrote sdCloseAlerts, more info there	 **/
/*****************************************************/
LRESULT CALLBACK HotKeyProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(msg == WM_CLOSE)
	{
		DestroyWindow(hWnd);
		return true;
	}
	else if(msg == WM_HOTKEY)
	{
		sdCloseAlerts();
	}

	return FALSE;
}

