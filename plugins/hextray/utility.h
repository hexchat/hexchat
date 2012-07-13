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

#ifndef _H_UTILITY_H
#define _H_UTILITY_H

WORD					HotkeyfToMod	(WORD);
WORD					ModToHotkeyf	(WORD);

int						SetOption		(HWND, unsigned int, unsigned int);
int						SetCheck		(HWND, unsigned int, unsigned int);
int						SetToggle		(HWND, unsigned int, unsigned int, bool);
void					ErrorDebug		(LPTSTR lpszFunction);
void					SetDialog		(HWND, int);
void					CheckPrefs		(HWND, int);
bool					FileExists		(TCHAR *);
DWORD WINAPI			HoldCloseThread	(LPVOID);
void					SavePrefs		(int);
void					LoadPrefs		();
void					HoldClose		();

void ConvertString(const char *in,		wchar_t *out,	int size);
void ConvertString(const wchar_t *in,	char *out,		int size);
void ConvertString(const char *in,		char *out,		int size);

int WritePrivateProfileIntA(char *, char *, int, char *);
int WritePrivateProfileIntW(wchar_t *, wchar_t *, int, wchar_t *);

#ifdef UNICODE
#define WritePrivateProfileInt WritePrivateProfileIntW
#else
#define WritePrivateProfileInt WritePrivateProfileIntA
#endif

struct HOTKEY
{
	WORD key;
	WORD mod;
};
#endif