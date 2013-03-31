/* GTK+ Preference Tool
 * Copyright (C) 2003-2005 Alex Shaduri.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/***************************************************************************
                          win32util.h  -  description
                             -------------------
    begin                : Tue Jan 14 2003
    copyright            : (C) 2003 by Alex Shaduri
    email                : alex_sh@land.ru
 ***************************************************************************/
#ifndef WIN32UTIL_H
#define WIN32UTIL_H

#ifdef _WIN32

#include <string>
#include "sys_win32.h"



std::string win32_get_registry_value_string(HKEY base, const std::string& keydir, const std::string& key);
void win32_set_registry_value_string(HKEY base, const std::string& keydir, const std::string& key, const std::string& value);




#endif //_WIN32

#endif // WIN32UTIL_H
