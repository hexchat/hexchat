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

/* include stuff for internet */

#ifndef HEXCHAT_INET_H
#define HEXCHAT_INET_H

#ifndef WIN32

#ifdef WANTSOCKET
#include <sys/types.h>
#include <sys/socket.h>
#endif
#ifdef WANTARPA
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef WANTDNS
#include <netdb.h>
/* OpenBSD's netdb.h does not define AI_ADDRCONFIG */
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif
#endif
#define closesocket close
#define set_blocking(sok) fcntl(sok, F_SETFL, 0)
#define set_nonblocking(sok) fcntl(sok, F_SETFL, O_NONBLOCK)
#define would_block() (errno == EAGAIN || errno == EWOULDBLOCK)
#define sock_error() (errno)

#else

#include "config.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#define set_blocking(sok)	{ \
									unsigned long zero = 0; \
									ioctlsocket (sok, FIONBIO, &zero); \
									}
#define set_nonblocking(sok)	{ \
										unsigned long one = 1; \
										ioctlsocket (sok, FIONBIO, &one); \
										}
#define would_block() (WSAGetLastError() == WSAEWOULDBLOCK)
#define sock_error WSAGetLastError

#endif

#endif
