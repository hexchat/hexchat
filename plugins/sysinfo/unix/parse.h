/*
 * parse.h - parsing header for X-Sys
 * by mikeshoup
 * Copyright (C) 2003, 2004, 2005 Michael Shoup
 * Copyright (C) 2005, 2006 Tony Vroon
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


#ifndef _PARSE_H_
#define _PARSE_H_

int xs_parse_cpu(char *model, char *vendor, double *freq);
gint64 xs_parse_uptime(void);
int xs_parse_sound(char *snd_card);
int xs_parse_meminfo(unsigned long long *mem_tot, unsigned long long *mem_free, int swap);
int xs_parse_video(char *vid_card);
int xs_parse_agpbridge(char *agp_bridge);
int xs_parse_ether(char *ethernet_card);
int xs_parse_distro(char *name);

#endif
