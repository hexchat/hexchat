/* XChat Win32 DNS Plugin
 * Copyright (C) 2003-2004 Peter Zelezny.
 * Copyright (C) 2012 Berke Viktor.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
/*
 * Requires MS Visual Studio and IPV6 headers to compile (run nmake).
 * Compiling with gcc (mingw) will fail due to missing gai_strerror.
 */

#define DNS_VERSION "2.4"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define USE_IPV6

#ifdef WIN32
#ifdef USE_IPV6
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock2.h>
#endif
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "xchat-plugin.h"
#include "thread.h"

#define HELP "Usage: DNS <nickname|hostname|numerical address>\n"
#define HEAD "\0034[DNS]\017\t"

#define PIPE_READ 0
#define PIPE_WRITE 1
#define MAX_HOSTNAME 128

static xchat_plugin *ph;
static thread *active_thread = NULL;


static int
waitline (void *source, char *buf, int bufsize)
{
	int i = 0;
	int len;

	while(1)
	{
		len = 1;
		/* we can't read() here, due to glib's giowin32 */
		if(ph->xchat_read_fd(ph, source, buf + i, &len) != 0)
			return -1;
		if(buf[i] == '\n' || bufsize == i + 1)
		{
			buf[i] = 0;
			return i;
		}
		i++;
	}
}

static void *
thread_function (void *ud)
{
#ifdef USE_IPV6
	struct addrinfo *ent;
	struct addrinfo *cur;
	struct addrinfo hints;
#else
	struct hostent *ent;
#endif
	thread *th = ud;
	int fd = th->pipe_fd[PIPE_WRITE];
	int ret;
	char ipstring[MAX_HOSTNAME];
	char reverse[MAX_HOSTNAME];
//	int i;

	active_thread = th;

#ifdef USE_IPV6
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC; /* support ipv6 and ipv4 */
	hints.ai_flags = AI_CANONNAME;
//	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo (th->userdata, NULL, &hints, &ent);
	if (ret != 0)
	{
		sprintf (ipstring, "1%d\n", ret);	/* failed */
		write (fd, ipstring, strlen (ipstring));
//		Sleep (3000);
		active_thread = NULL;
		return 0;
	}

//	i = 0;
	cur = ent;
	while (cur)
	{
		/* find the numeric IP number */
		ipstring[0] = 0;
		getnameinfo (cur->ai_addr, cur->ai_addrlen,
						 ipstring, sizeof (ipstring), NULL, 0, NI_NUMERICHOST);

		if (cur->ai_canonname)
		{
			/* force reverse lookup if canonname & ipstring are the same */
			if (/*i == 0 &&*/ strcmp (cur->ai_canonname, ipstring) == 0)
				goto lamecode;
		}

		if (cur->ai_canonname)
		{
			write (fd, "0", 1);
			write (fd, ipstring, strlen (ipstring));
			write (fd, "\n", 1);
			write (fd, cur->ai_canonname, strlen (cur->ai_canonname));
		} else
		{
lamecode:
	//		ret = 1;
	//		if (i == 0)
			{
				/* reverse lookup */
				reverse[0] = 0;
				ret = getnameinfo (cur->ai_addr, cur->ai_addrlen,
							 reverse, sizeof (reverse), NULL, 0, NI_NAMEREQD);
			}

			write (fd, "0", 1);
			write (fd, ipstring, strlen (ipstring));

			write (fd, "\n", 1);
			if (ret == 0)
				write (fd, reverse, strlen (reverse));
		}
		write (fd, "\n", 1);

//		i++;
		cur = cur->ai_next;
	}

	/* tell the parent we're done */
	write (fd, "2\n", 2);
	freeaddrinfo (ent);

#else
	ent = gethostbyname (th->userdata);
	if (ent)
	{
		write (fd, "0", 1);
		write (fd, ent->h_name, strlen (ent->h_name));
		write (fd, "\n", 1);
		write (fd, ent->h_name, strlen (ent->h_name));
		write (fd, "\n", 1);
		write (fd, "2\n", 2);
	} else
	{
		write (fd, "10\n", 1);
	}
#endif

//	Sleep (3000);
	active_thread = NULL;	/* race condition, better than nothing */

	return 0;
}

static int
dns_close_pipe (int fd)
{
	close (fd);
	return 0;
}

/* read messages comming from the child (through the pipe) */

static int
dns_read_cb (int fd, int flags, thread *th, void *source)
{
	char buf[512];
	char buf2[512];

	while (waitline (source, buf, sizeof (buf)))
	{
		switch (buf[0])
		{
		case '0':		/* got data to show */
			waitline (source, buf2, sizeof (buf2));
			if (buf2[0] == 0)
				xchat_printf(ph, HEAD"\002Numerical\002: %s\n", buf + 1);
			else
				xchat_printf(ph, HEAD"\002Canonical\002: %s \002Numerical\002: %s\n", buf2, buf + 1);
			return 1;

		case '1':		/* failed */
			xchat_printf(ph, HEAD"Lookup failed. %s\n", gai_strerrorA (atoi (buf + 1)));

		case '2':		/* done */
		//	close (th->pipe_fd[PIPE_WRITE]);
		//	close (th->pipe_fd[PIPE_READ]);
			xchat_hook_timer(ph, 3000, dns_close_pipe, (void *)th->pipe_fd[PIPE_WRITE]);
			xchat_hook_timer(ph, 4000, dns_close_pipe, (void *)th->pipe_fd[PIPE_READ]);
			free (th->userdata); 	/* hostname strdup'ed */
			free (th);
			return 0;
		}
	}

	return 1;
}

/* find hostname from nickname (search the userlist, current chan only) */

static char *
find_nick_host (char *nick)
{
	xchat_list *list;
	char *at;
	const char *host;

	list = xchat_list_get (ph, "users");
	if (!list)
		return NULL;

	while (xchat_list_next (ph, list))
	{
		if (stricmp (nick, xchat_list_str (ph, list, "nick")) == 0)
		{
			host = xchat_list_str (ph, list, "host");
			if (host)
			{
				at = strrchr (host, '@');
				if (at)
					return at + 1;
			}
			break;
		}
	}

	return NULL;
}

static int
dns_cmd_cb (char *word[], char *word_eol[], void *ud)
{
	thread *th;
	char *nickhost;

	if (!word[2][0])
	{
		xchat_print (ph, HELP);
		return XCHAT_EAT_ALL;
	}

	th = thread_new ();
	if (th)
	{
		nickhost = find_nick_host (word[2]);
		if (nickhost)
		{
			xchat_printf (ph, HEAD"Looking up %s (%s)...\n", nickhost, word[2]);
			th->userdata = strdup (nickhost);
		} else
		{
			xchat_printf (ph, HEAD"Looking up %s...\n", word[2]);
			th->userdata = strdup (word[2]);
		}

		if (thread_start (th, thread_function, th))
		{
			xchat_hook_fd(ph, th->pipe_fd[PIPE_READ],
							XCHAT_FD_READ | XCHAT_FD_EXCEPTION | XCHAT_FD_NOTSOCKET,
							(void *)dns_read_cb, th);

		}
	}

	return XCHAT_EAT_ALL;
}

int
xchat_plugin_deinit (xchat_plugin *plugin_handle)
{
	while (active_thread)	/* children will set this var to NULL soon... */
	{
		Sleep (1000);
	}
	xchat_printf (ph, "DNS plugin unloaded\n");
	return 1;
}

int
xchat_plugin_init
				(xchat_plugin *plugin_handle, char **plugin_name,
				char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any xchat_* functions */
	ph = plugin_handle;

	*plugin_name = "DNS";
	*plugin_desc = "Threaded IPv4/6 DNS Command";
	*plugin_version = DNS_VERSION;

	xchat_hook_command(ph, "DNS", XCHAT_PRI_LOW, dns_cmd_cb, HELP, 0);
	xchat_printf (ph, "DNS plugin loaded\n");

	return 1;       /* return 1 for success */
}
