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


#ifndef FORMAT_H
#define FORMAT_H

char *sysinfo_format_uptime(gint64 uptime);
char *sysinfo_format_memory(guint64 totalmem, guint64 freemem);
char *sysinfo_format_disk(guint64 total, guint64 free);

#endif
