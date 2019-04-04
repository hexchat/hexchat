/*
 * SysInfo - sysinfo plugin for HexChat
 * Copyright (c) 2015 Patrick Griffis.
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


#ifndef SYSINFO_BACKEND_H
#define SYSINFO_BACKEND_H

char *sysinfo_backend_get_os(void);
char *sysinfo_backend_get_disk(void);
char *sysinfo_backend_get_memory(void);
char *sysinfo_backend_get_cpu(void);
char *sysinfo_backend_get_gpu(void);
char *sysinfo_backend_get_sound(void);
char *sysinfo_backend_get_uptime(void);
char *sysinfo_backend_get_network(void);

#endif
