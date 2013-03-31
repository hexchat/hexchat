/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

#ifndef HEXCHAT_TYPEDEF_H
#define HEXCHAT_TYPEDEF_H

#ifdef WIN32

#ifndef SSIZE_T_DEFINED
#ifdef ssize_t
#undef ssize_t
#endif
#ifdef _WIN64
typedef __int64          ssize_t;
#else
typedef _W64 int         ssize_t;
#endif
#define SSIZE_T_DEFINED
#endif

#ifndef fstat
#define fstat _fstat
#define stat _stat
#endif

#endif

#endif
