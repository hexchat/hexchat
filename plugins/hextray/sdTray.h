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

#ifndef _H_SDTRAY_H
#define _H_SDTRAY_H

int				AddIcon		(HWND, UINT, HICON, char *, unsigned short, UINT);
int				ShowBalloon	(HWND, UINT, char *, char *, UINT, UINT);
int				BlinkIcon	(HWND, UINT, HICON, HICON, UINT, UINT);
int				SetTooltip	(HWND, UINT, char *);
int				SetIcon		(HWND, UINT, HICON);
void			StopBlink	(HWND, UINT, HICON);
int				RemoveIcon	(HWND, UINT);

typedef struct IBLINK
{
	HICON hBase;
	HICON hBlink;
	HWND hwnd;
	UINT id;
	UINT time;
	UINT num;
}iBlink;
#endif