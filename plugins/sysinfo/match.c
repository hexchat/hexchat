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
        char *result, *bytesize;
	double free_space, total_space;
	free_space = *free_k;
	total_space = *total_k;
        result = malloc(bsize * sizeof(char));
	const char *quantities = "KB\0MB\0GB\0TB\0PB\0EB\0ZB\0YB\0";
	int i=0;
	if (total_space == 0)
	{
		snprintf(result, bsize, "%s: none", desc);
		return result;
	}
        bytesize = malloc(3 * sizeof(char));
	while (total_space > 1023 && i <= 14)
	{
		i=i+3;
		*bytesize=*(quantities+i);
		*(bytesize+1)=*(quantities+i+1);
		*(bytesize+2)=*(quantities+i+2);
		free_space = free_space / 1024;
		total_space = total_space / 1024;
	}
	if (sysinfo_get_percent () != 0)
		snprintf(result, bsize, "%s: %.1f%s, %.1f%% free",
		desc, total_space, bytesize,
		percentage(free_k, total_k));
	else
		snprintf(result, bsize, "%s: %.1f%s/%.1f%s free",
		desc, free_space, bytesize, total_space, bytesize);
	free (bytesize);
        return result;
}

void remove_leading_whitespace(char *buffer)
{
    char * buffer2 = NULL;
    int i = 0, j = 0,ews = 0;

    buffer2 = (char*) malloc(strlen(buffer)*(sizeof(char)));
    if (buffer2 == NULL) 
        return;
    
    memset(buffer2,(char) 0,strlen(buffer));
    while (i < strlen(buffer))
    {
        /* count tabs and spaces until we hit a non-whitespace character. */
        if (!(buffer[i] == (char) 32 || buffer[i] == (char) 9) || ews == 1)
        {
            ews = 1;
            buffer2[j] = buffer[i];
            j++;
        } 
        i++;
    }
    memset(buffer, (char)0,strlen(buffer));
    strcpy(buffer,buffer2);
    free(buffer2);
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
