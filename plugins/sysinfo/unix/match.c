/*
 * match.c - matching functions for X-Sys
 * Copyright (C) 2005, 2006, 2007 Tony Vroon
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "sysinfo.h"

#define delims ":="

void find_match_char(char *buffer, char *match, char *result)
{
	char *position;
	g_strchug(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
	{
		position = strpbrk(buffer, delims);
		if (position != NULL)
		{
			position += 1;
			strcpy(result, position);
			position = strstr(result, "\n");
			*(position) = '\0';
			g_strchug(result);
		}
		else
			strcpy(result, "\0");
	}
}

void find_match_double(char *buffer, char *match, double *result)
{
	char *position;
	g_strchug(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
	{
		position = strpbrk(buffer, delims);
		if (position != NULL)
		{
        	position += 1;
        	*result = strtod(position, NULL);
		}
		else
			*result = 0;
	}
}

void find_match_double_hex(char *buffer, char *match, double *result)
{
	char *position;
	g_strchug(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
	{
		position = strpbrk(buffer, delims);
		if (position != NULL)
		{
			memcpy(position,"0x",2);
			*result = strtod(position,NULL);
		}
		else
			*result = 0;
	}
}

void find_match_ll(char *buffer, char *match, unsigned long long *result)
{
	char *position;
	g_strchug(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
	{
		position = strpbrk(buffer, delims);
		if (position != NULL)
		{
			position += 1;
			*result = strtoll(position, NULL, 10);
		}
		else
			*result = 0;
	}
}

