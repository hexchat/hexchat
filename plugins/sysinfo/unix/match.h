/*
 * match.h - matching header for X-Sys
 * Copyright (C) 2005 Tony Vroon
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

#ifndef _MATCH_H_
#define _MATCH_H_

void find_match_char(char *buffer, char *match, char *result);
void find_match_double(char *buffer, char *match, double *result);
void find_match_double_hex(char *buffer, char *match, double *result);
void find_match_ll(char *buffer, char *match, unsigned long long *result);
void remove_leading_whitespace(char *buffer);

#endif
