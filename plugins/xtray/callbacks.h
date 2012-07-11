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

#ifndef _H_CALLBACKS_H
#define _H_CALLBACKS_H

int					event_cb		(char *word[], void *userdata);
int					command_cb		(char *word[], char *word_eol[], void *userdata);

LRESULT CALLBACK	WindowProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL	CALLBACK	EnumWindowsProc	(HWND hWnd, LPARAM lParam);
LRESULT CALLBACK	sdTrayProc		(HWND hwnd, int msg);

LRESULT CALLBACK	AlertProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK	HotKeyProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK	EventsProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK	AboutProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK	AlertsProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK	SettingsProc	(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int		CALLBACK	PrefProc		(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#endif