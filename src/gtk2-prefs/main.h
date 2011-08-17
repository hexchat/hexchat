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
