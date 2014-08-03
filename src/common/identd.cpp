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

/* simple identd server for HexChat under Win32 */
#include <thread>
#include <atomic>
#include <string>
#include "inet.h"
extern "C"{
#include "hexchat.h"
#include "hexchatc.h"
#include "text.h"
}

namespace{
	static ::std::atomic_bool identd_is_running = { false };
#ifdef USE_IPV6
	static ::std::atomic_bool identd_ipv6_is_running = { false };
#endif

	struct sock{
		SOCKET s;
		sock(SOCKET sock)
			:s(sock){}
		~sock(){
			release();
		}
		void release(){
			if (s != INVALID_SOCKET)
				closesocket(s);
			s = INVALID_SOCKET;
		}
		operator SOCKET(){
			return s;
		}
	};

static int
identd(std::string username)
{
	int len;
	char *p;
	char buf[256];
	char outbuf[256];
	char ipbuf[INET_ADDRSTRLEN];
	sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(113);

	sock sok = socket(AF_INET, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
	{
		return 0;
	}

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		return 0;
	}

	len = sizeof (addr);
	sock read_sok = accept(sok, (struct sockaddr *) &addr, &len);
	sok.release();
	if (read_sok == INVALID_SOCKET)
	{
		return 0;
	}

	identd_is_running = false;

#if 0	/* causes random crashes, probably due to CreateThread */
	EMIT_SIGNAL (XP_TE_IDENTD, current_sess, inet_ntoa (addr.sin_addr), username, NULL, NULL, 0);
#endif
	inet_ntop (AF_INET, &addr.sin_addr, ipbuf, sizeof (ipbuf));
	snprintf(outbuf, sizeof(outbuf), "*\tServicing ident request from %s as %s\n", ipbuf, username.c_str());
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n",
		atoi(buf), atoi(p + 1), username.c_str());
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);

	return 0;
}

#ifdef USE_IPV6
static int
identd_ipv6(::std::string username)
{
		int len;
	char *p;
	char buf[256];
	char outbuf[256];
	char ipbuf[INET6_ADDRSTRLEN];
	struct sockaddr_in6 addr = { 0 };
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(113);

	sock sok = socket(AF_INET6, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
	{
		return 0;
	}

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		return 0;
	}

	len = sizeof (addr);
	sock read_sok = accept(sok, (struct sockaddr *) &addr, &len);
	sok.release();
	if (read_sok == INVALID_SOCKET)
	{
		return 0;
	}

	identd_ipv6_is_running = false;

	inet_ntop (AF_INET6, &addr.sin6_addr, ipbuf, sizeof (ipbuf));
	snprintf(outbuf, sizeof(outbuf), "*\tServicing ident request from %s as %s\n", ipbuf, username.c_str());
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		snprintf(outbuf, sizeof(outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n", atoi(buf), atoi(p + 1), username.c_str());
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);

	return 0;
}
#endif
}

void
identd_start(const ::std::string& username)
{
#ifdef USE_IPV6
	if (identd_ipv6_is_running == false)
	{
		identd_ipv6_is_running = true;
		::std::thread ipv6(identd_ipv6, username);
		ipv6.detach();
	}
#endif

	if (identd_is_running == false)
	{
		identd_is_running = true;
		::std::thread ipv4(identd, username);
		ipv4.detach();
	}
}
