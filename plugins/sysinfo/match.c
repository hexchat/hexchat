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
#include <ctype.h>
#include <unistd.h>
#include "xsys.h"

float percentage(unsigned long long *free, unsigned long long *total)
{
        unsigned long long result = (*free) * (unsigned long long)1000 / (*total);
        return result / 10.0;
}

char *pretty_freespace(const char *desc, unsigned long long *free_k, unsigned long long *total_k)
{
	char *quantities[] = { "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB", 0 };
	char *result, **quantity;
	double free_space, total_space;
	free_space = *free_k;
	total_space = *total_k;
        result = malloc(bsize * sizeof(char));
	if (total_space == 0)
	{
		snprintf(result, bsize, "%s: none", desc);
		return result;
	}
        quantity = quantities;
	while (total_space > 1023 && *(quantity + 1))
	{
		quantity++;
		free_space = free_space / 1024;
		total_space = total_space / 1024;
	}
	if (sysinfo_get_percent () != 0)
		snprintf(result, bsize, "%s: %.1f%s, %.1f%% free",
		desc, total_space, *quantity,
		percentage(free_k, total_k));
	else
		snprintf(result, bsize, "%s: %.1f%s/%.1f%s free",
		desc, free_space, *quantity, total_space, *quantity);
        return result;
}


void remove_leading_whitespace(char *buffer)
{
	char *p;

	if (buffer == NULL)
		return;

	for (p = buffer; *p && isspace (*p); p++)
	;

	memmove (buffer, p, strlen (p) + 1);
}

char *decruft_filename(char *buffer)
{
	char *match, *match_end;

	while ((match = strstr(buffer, "%20")))
	{
		match_end = match + 3;
		*match++ = ' ';
		while (*match_end)
			*match++ = *match_end++;
		*match = 0;
	}
	return buffer;
}

void find_match_char(char *buffer, char *match, char *result)
{
	char *position;
	remove_leading_whitespace(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
		{
			position = strpbrk(buffer, delims);
			if (position != NULL) {
				position += 1;
				strcpy(result, position);
				position = strstr(result, "\n");
				*(position) = '\0';
				remove_leading_whitespace(result);
				}
			else
				strcpy(result, "\0");
		}
}

void find_match_double(char *buffer, char *match, double *result)
{
	char *position;
	remove_leading_whitespace(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
		{
			position = strpbrk(buffer, delims);
			if (position != NULL) {
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
	remove_leading_whitespace(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
		{
			position = strpbrk(buffer, delims);
			if (position != NULL) {
				memcpy(position,"0x",2);
				*result = strtod(position,NULL);
				}
			else
				*result = 0;
		}
}

void find_match_int(char *buffer, char *match, unsigned int *result)
{
	char *position;
	remove_leading_whitespace(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
		{
			position = strpbrk(buffer, delims);
			if (position != NULL) {
                        	position += 1;
                        	*result = atoi(position);
				}
			else
				*result = 0;
		}
}

void find_match_ll(char *buffer, char *match, unsigned long long *result)
{
	char *position;
	remove_leading_whitespace(buffer);
	if(strstr(buffer, match) == strstr(buffer, buffer))
		{
			position = strpbrk(buffer, delims);
			if (position != NULL) {
		               	position += 1;
		               	*result = strtoll(position, NULL, 10);
				}
			else
				*result = 0;
		}
}

void format_output(const char *arg, char *string, char *format)
{
        char *pos1, *pos2, buffer[bsize];
        pos1 = &format[0];
        strncpy(buffer, string, bsize);
        string[0] = '\0';

        while((pos2 = strstr(pos1, "%")) != NULL)
        {
                strncat(string, pos1, (size_t)(pos2-pos1));
                if(*(pos2+1) == '1')
                        strcat(string, arg);
                else if(*(pos2+1) == '2')
                        strcat(string, buffer);
                else if(*(pos2+1) == 'C' || *(pos2+1) == 'c')
                        strcat(string, "\003");
                else if(*(pos2+1) == 'B' || *(pos2+1) == 'b')
                        strcat(string, "\002");
                else if(*(pos2+1) == 'R' || *(pos2+1) == 'r')
                        strcat(string, "\026");
                else if(*(pos2+1) == 'O' || *(pos2+1) == 'o')
                        strcat(string, "\017");
                else if(*(pos2+1) == 'U' || *(pos2+1) == 'u')
                        strcat(string, "\037");
                else if(*(pos2+1) == '%')
                        strcat(string, "%");
                pos1=pos2+2;
        }

        strcat(string, pos1);
}

void flat_format_output(const char *arg, char *string, char *format)
{
        char *pos1, *pos2, buffer[bsize];
        pos1 = &format[0];
        strncpy(buffer, string, bsize);
        string[0] = '\0';

        while((pos2 = strstr(pos1, "%")) != NULL)
        {
                strncat(string, pos1, (size_t)(pos2-pos1));
                if(*(pos2+1) == '1')
                        strcat(string, arg);
                else if(*(pos2+1) == '2')
                        strcat(string, buffer);
                else if(*(pos2+1) == '%')
                        strcat(string, "%");
                pos1=pos2+2;
        }

        strcat(string, pos1);
}
