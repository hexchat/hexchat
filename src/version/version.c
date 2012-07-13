/* XChat-WDK
 * Copyright (c) 2011 Berke Viktor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "../../config.h"

char *
comma ()
{
	int major, minor;
	char *version_string; /* nnnn,n,n,n format */

	version_string = (char*) malloc (11);

	if (sscanf (PACKAGE_VERSION, "%d-%d", &major, &minor) > 1)
	{
		sprintf (version_string, "%d,%d,0,0", major, minor);
	} else
	{
		sprintf (version_string, "%d,0,0,0", major);
	}

	return version_string;
}

char *
point ()
{
	int major1, major2, major3, major4, minor;
	char *version_string; /* nn.nn.nn.nn format */

	version_string = (char*) malloc (12);

	if (sscanf (PACKAGE_VERSION, "%c%c%c%c-%d", &major1, &major2, &major3, &major4, &minor) > 4)
	{
		sprintf (version_string, "%c%c.%c%c.%d.0", major1, major2, major3, major4, minor);
	}
	else
	{
		sprintf (version_string, "%c%c.%c%c.0.0", major1, major2, major3, major4);
	}

	return version_string;
}

int
main (int argc, char *argv[])
{
	if (argc > 1)
	{
		if (!strcmp (argv[1], "-r"))			/* hexchat.rc/FILEVERSION, PRODUCTVERSION */
		{
			printf ("#define COMMA_VERSION %s\n", comma ());
		}
		else if (!strcmp (argv[1], "-a"))	/* hexchat.iss/AppVerName */
		{
#ifdef _WIN64
			printf ("AppVerName=HexChat %s (x64)\n", PACKAGE_VERSION);
#else
			printf ("AppVerName=HexChat %s (x86)\n", PACKAGE_VERSION);
#endif
		}
		else if (!strcmp (argv[1], "-v"))	/* hexchat.iss/AppVersion */
		{
			/* printf ("AppVersion=%s\n", point ()); this was required only for nnnn[-n] version numbers */
			printf ("AppVersion=%s\n", PACKAGE_VERSION);
		}
		else if (!strcmp (argv[1], "-i"))	/* hexchat.iss/VersionInfoVersion */
		{
			/* printf ("VersionInfoVersion=%s\n", point ()); this was required only for nnnn[-n] version numbers */
			printf ("VersionInfoVersion=%s\n", PACKAGE_VERSION);
		}
		else if (!strcmp (argv[1], "-o"))	/* hexchat.iss/OutputBaseFilename */
		{
#ifdef _WIN64
			printf ("OutputBaseFilename=HexChat %s x64\n", PACKAGE_VERSION);
#else
			printf ("OutputBaseFilename=HexChat %s x86\n", PACKAGE_VERSION);
#endif
		}
		else if (!strcmp (argv[1], "-v"))	/* version.txt */
		{
			printf ("%s", PACKAGE_VERSION);
		} else
		{
			printf ("usage:\n\t-a\thexchat.iss/AppVerName\n\t-i\thexchat.iss/VersionInfoVersion\n\t-o\thexchat.iss/OutputBaseFilename\n\t-r\thexchat.rc/FILEVERSION, PRODUCTVERSION\n\t-v\thexchat.iss/AppVersion\n");
		}
	} else
	{
		printf ("usage:\n\t-a\thexchat.iss/AppVerName\n\t-i\thexchat.iss/VersionInfoVersion\n\t-o\thexchat.iss/OutputBaseFilename\n\t-r\thexchat.rc/FILEVERSION, PRODUCTVERSION\n\t-v\thexchat.iss/AppVersion\n");
	}

#if 0 /* ugly hack */
	switch ((int) argv[1][0])
	{
		case 'r':	/* hexchat.rc/FILEVERSION, PRODUCTVERSION*/
			printf ("#define COMMA_VERSION \"%s\"\n", comma ());
			break;
		case 'a':	/* hexchat.iss/AppVerName */
			printf ("AppVerName=HexChat %s\n", PACKAGE_VERSION);
			break;
		case 'v':	/* hexchat.iss/AppVersion */
			printf ("AppVersion=%s\n", point ());
			break;
		case 'i':	/* hexchat.iss/VersionInfoVersion */
			printf ("VersionInfoVersion=%s\n", point ());
			break;
		case 'o':	/* hexchat.iss/OutputBaseFilename */
			printf ("OutputBaseFilename=HexChat %s\n", PACKAGE_VERSION);
			break;
		case 'u':	/* version.txt */
			printf ("%s", PACKAGE_VERSION);
			break;
		default:
			printf ("use a, i, o, r or v.\n");
			break;
	}
#endif

	return 0;
}
