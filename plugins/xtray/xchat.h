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

void					xchat_exec			(char *);
char					*xchat_strip_color	(char *);
void					xchat_parse			(char *);
struct _xchat_context	*xchat_find_server	(int);
void					xchat_globally_away	(TCHAR *);
void					xchat_away			(TCHAR *);
void					xchat_globally_back	();
void					xchat_back			();
HMENU					setServerMenu		();

#endif