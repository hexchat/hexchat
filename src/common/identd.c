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
	struct sockaddr_in addr;

	sok = socket (AF_INET, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
	{
		free (username);
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
		free (username);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		free (username);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);
	if (read_sok == INVALID_SOCKET)
	{
		free (username);
		return 0;
	}

	identd_is_running = FALSE;

#if 0	/* causes random crashes, probably due to CreateThread */
	EMIT_SIGNAL (XP_TE_IDENTD, current_sess, inet_ntoa (addr.sin_addr), username, NULL, NULL, 0);
#endif
	snprintf (outbuf, sizeof (outbuf), "*\tServicing ident request from %s as %s\n", inet_ntoa (addr.sin_addr), username);
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n",
					 atoi (buf), atoi (p + 1), username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	free (username);

	return 0;
}

#ifdef USE_IPV6
#define IPV6BUFLEN 60
static int
identd_ipv6 (char *username)
{
	int sok, read_sok, len;
	char *p;
	char buf[256];
	char outbuf[256];	
	LPSTR ipv6buf = (LPSTR) malloc (IPV6BUFLEN);
	struct sockaddr_in6 addr;

	sok = socket (AF_INET6, SOCK_STREAM, 0);

	if (sok == INVALID_SOCKET)
	{
		free (username);
		free (ipv6buf);
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
		free (username);
		free (ipv6buf);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		free (username);
		free (ipv6buf);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);

	if (read_sok == INVALID_SOCKET)
	{
		free (username);
		free (ipv6buf);
		return 0;
	}

	identd_ipv6_is_running = FALSE;

	if (WSAAddressToString ((struct sockaddr *) &addr, sizeof (addr), NULL, ipv6buf, (LPDWORD) IPV6BUFLEN) == SOCKET_ERROR)
	{
		snprintf (ipv6buf, sizeof (ipv6buf) - 1, "[SOCKET ERROR: 0x%X]", WSAGetLastError ());
	}

#if 0	/* causes random crashes, probably due to CreateThread */
	EMIT_SIGNAL (XP_TE_IDENTD, current_sess, ipv6buf, username, NULL, NULL, 0);
#endif
	snprintf (outbuf, sizeof (outbuf), "*\tServicing ident request from %s as %s\n", ipv6buf, username);
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');

	if (p)
	{
		snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n", atoi (buf), atoi (p + 1), username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	free (username);
	free (ipv6buf);

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
						 strdup (username), 0, &tidv6));
	}
#endif

	if (identd_is_running == FALSE)
	{
		identd_is_running = TRUE;
		CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) identd,
						 strdup (username), 0, &tid));
	}
}
