/* simple identd server for xchat under win32 */


static int identd_is_running = FALSE;


static int
identd (void *unused)
{
	int sok, read_sok, len;
	char *p;
	char buf[256];
	char outbuf[256];
	struct sockaddr_in addr;

	sok = socket (AF_INET, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
		return 0;

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons (113);

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		closesocket (sok);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);
	if (read_sok == INVALID_SOCKET)
		return 0;

	snprintf (outbuf, sizeof (outbuf), "%%\tServicing ident request from %s\n",
				 inet_ntoa (addr.sin_addr));
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n",
					 atoi (buf), atoi (p + 1), prefs.username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	identd_is_running = FALSE;

	return 0;
}

static void
identd_start (void)
{
	DWORD tid;

	if (identd_is_running == FALSE)
	{
		identd_is_running = TRUE;
		CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) identd,
						 NULL, 0, &tid));
	}
}
