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
