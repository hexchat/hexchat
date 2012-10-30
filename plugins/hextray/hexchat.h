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

#ifndef _H_XCHAT_H
#define _H_XCHAT_H

void					hexchat_exec			(char *);
char					*hexchat_strip_color	(char *);
void					hexchat_parse			(char *);
struct _hexchat_context	*hexchat_find_server	(int);
void					hexchat_globally_away	(TCHAR *);
void					hexchat_away			(TCHAR *);
void					hexchat_globally_back	();
void					hexchat_back			();
HMENU					setServerMenu		();

#endif