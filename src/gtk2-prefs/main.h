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
                          main.cpp  -  description
                             -------------------
    begin                : Wed Jan  1 2003
    copyright            : (C) 2003 - 2005 by Alex Shaduri
    email                : ashaduri '@' gmail.com
 ***************************************************************************/

#ifndef _MAIN_H_
#define _MAIN_H_


#include <string>


std::string& get_orig_theme();
std::string& get_orig_font();

std::string get_current_theme();
std::string get_current_font();

std::string get_selected_theme();
std::string get_selected_font();

void set_theme(const std::string& theme_name, const std::string& font);
void apply_theme(const std::string& theme_name, const std::string& font);
bool save_current_theme();

void program_shutdown();


#endif
