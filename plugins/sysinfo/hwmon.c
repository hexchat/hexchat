/*
 * hwmon.c - Hardware monitoring functions for X-Sys
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "xsys.h"

int hwmon_chip_present()
{
	FILE *fp = fopen("/sys/class/hwmon/hwmon0/device/name", "r");
	if(fp != NULL) {
		fclose(fp);
		return 1;
	}
	return 0;
}

#if 0
void get_hwmon_chip_name(char *name)
{
	char *position, buffer[bsize];
	FILE *fp = fopen("/sys/class/hwmon/hwmon0/device/name", "r");
	if(fp != NULL) {
		if(fgets(buffer, bsize, fp) != NULL) {
			position = strstr(buffer, "\n");
                        *(position) = '\0';
			snprintf(name, sizeof(name), "%s", buffer);
		}
		fclose(fp);
	}
}
#endif

void get_hwmon_temp(unsigned int *value, unsigned int *sensor)
{
	char buffer[bsize];
	snprintf(buffer, bsize, "/sys/class/hwmon/hwmon0/device/temp%i_input", *sensor);
	FILE *fp = fopen(buffer, "r");
	if(fp != NULL) {
		if(fgets(buffer, bsize, fp) != NULL)
			*value = atoi(buffer);
		fclose(fp);
	}
}
