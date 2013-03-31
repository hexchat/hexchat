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
                          win32util.cpp  -  description
                             -------------------
    begin                : Tue Jan 14 2003
    copyright            : (C) 2003 by Alex Shaduri
    email                : alex_sh@land.ru
 ***************************************************************************/

#ifdef _WIN32

#include <string>
#include "sys_win32.h"
#include "win32util.h"





std::string win32_get_registry_value_string(HKEY base, const std::string& keydir, const std::string& key)
{

	HKEY reg_key = NULL;
	DWORD type;
	DWORD nbytes;
	char* result = NULL;
//HKEY_CURRENT_USER
	nbytes = 0;
	if ( RegOpenKeyEx ( base, keydir.c_str(), 0, KEY_QUERY_VALUE, &reg_key) == ERROR_SUCCESS
			&& RegQueryValueEx (reg_key, key.c_str(), 0, &type, NULL, &nbytes) == ERROR_SUCCESS ) {
		result = (char*)malloc(nbytes + 1);
		RegQueryValueEx (reg_key,  key.c_str(), 0, &type, (BYTE*)result, &nbytes);
		result[nbytes] = '\0';
	}

	if (reg_key != NULL)
		RegCloseKey (reg_key);

	std::string ret = "";

	if (result) {
		ret = result;
	}

	return ret;

}




void win32_set_registry_value_string(HKEY base, const std::string& keydir, const std::string& key, const std::string& value)
{

	HKEY reg_key = NULL;
	DWORD nbytes;

	nbytes = value.length() + 1;

	if ( RegOpenKeyEx ( base, keydir.c_str(), 0, KEY_QUERY_VALUE, &reg_key) == ERROR_SUCCESS) {
		RegSetValueEx (reg_key,  key.c_str(), 0, REG_SZ, (const BYTE*)(value.c_str()), nbytes);
	}

	if (reg_key != NULL)
		RegCloseKey (reg_key);

}







#endif






