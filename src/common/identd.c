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

#include "inet.h"
#include "hexchat.h"
#include "hexchatc.h"
#include "text.h"

static int identd_is_running = FALSE;
#ifdef USE_IPV6
static int identd_ipv6_is_running = FALSE;
#endif

static int
identd (char *username)
{
	int sok, read_sok, len;
	char *p;
	char buf[256];
	char outbuf[256];
	char ipbuf[INET_ADDRSTRLEN];
	struct sockaddr_in addr;

	sok = socket (AF_INET, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
	{
		g_free (username);
		return 0;
	}

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons (113);

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		closesocket (sok);
		g_free (username);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		g_free (username);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);
	if (read_sok == INVALID_SOCKET)
	{
		g_free (username);
		return 0;
	}

	identd_is_running = FALSE;

#if 0	/* causes random crashes, probably due to CreateThread */
	EMIT_SIGNAL (XP_TE_IDENTD, current_sess, inet_ntoa (addr.sin_addr), username, NULL, NULL, 0);
#endif
	inet_ntop (AF_INET, &addr.sin_addr, ipbuf, sizeof (ipbuf));
	g_snprintf (outbuf, sizeof (outbuf), "*\tServicing ident request from %s as %s\n", ipbuf, username);
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		g_snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n",
					 atoi (buf), atoi (p + 1), username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	g_free (username);

	return 0;
}

#ifdef USE_IPV6
static int
identd_ipv6 (char *username)
{
	int sok, read_sok, len;
	char *p;
	char buf[256];
	char outbuf[256];
	char ipbuf[INET6_ADDRSTRLEN];
	struct sockaddr_in6 addr;

	sok = socket (AF_INET6, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
	{
		g_free (username);
		return 0;
	}

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	memset (&addr, 0, sizeof (addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons (113);

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		closesocket (sok);
		g_free (username);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		g_free (username);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);
	if (read_sok == INVALID_SOCKET)
	{
		g_free (username);
		return 0;
	}

	identd_ipv6_is_running = FALSE;

	inet_ntop (AF_INET6, &addr.sin6_addr, ipbuf, sizeof (ipbuf));
	g_snprintf (outbuf, sizeof (outbuf), "*\tServicing ident request from %s as %s\n", ipbuf, username);
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		g_snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n", atoi (buf), atoi (p + 1), username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	g_free (username);

	return 0;
}
#endif

void
identd_start (char *username)
{
	DWORD tid;

#ifdef USE_IPV6
	DWORD tidv6;
	if (identd_ipv6_is_running == FALSE)
	{
		identd_ipv6_is_running = TRUE;
		CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) identd_ipv6,
						 g_strdup (username), 0, &tidv6));
	}
#endif

	if (identd_is_running == FALSE)
	{
		identd_is_running = TRUE;
		CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) identd,
						 g_strdup (username), 0, &tid));
	}
}
