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
	} else
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
		if (!strcmp (argv[1], "-r"))			/* xchat.rc/FILEVERSION, PRODUCTVERSION */
		{
			printf ("#define COMMA_VERSION %s\n", comma ());
		}
		else if (!strcmp (argv[1], "-a"))	/* xchat-wdk.iss/AppVerName */
		{
#ifdef _WIN64
			printf ("AppVerName=XChat-WDK %s (x64)\n", PACKAGE_VERSION);
#else
			printf ("AppVerName=XChat-WDK %s (x86)\n", PACKAGE_VERSION);
#endif
		}
		else if (!strcmp (argv[1], "-v"))	/* xchat-wdk.iss/AppVersion */
		{
			printf ("AppVersion=%s\n", point ());
		}
		else if (!strcmp (argv[1], "-i"))	/* xchat-wdk.iss/VersionInfoVersion */
		{
			printf ("VersionInfoVersion=%s\n", point ());
		}
		else if (!strcmp (argv[1], "-o"))	/* xchat-wdk.iss/OutputBaseFilename */
		{
#ifdef _WIN64
			printf ("OutputBaseFilename=XChat-WDK %s x64\n", PACKAGE_VERSION);
#else
			printf ("OutputBaseFilename=XChat-WDK %s x86\n", PACKAGE_VERSION);
#endif
		}
		else if (!strcmp (argv[1], "-v"))	/* version.txt */
		{
			printf ("%s", PACKAGE_VERSION);
		} else
		{
			printf ("usage:\n\t-a\txchat-wdk.iss/AppVerName\n\t-i\txchat-wdk.iss/VersionInfoVersion\n\t-o\txchat-wdk.iss/OutputBaseFilename\n\t-r\txchat.rc/FILEVERSION, PRODUCTVERSION\n\t-v\txchat-wdk.iss/AppVersion\n");
		}
	} else
	{
		printf ("usage:\n\t-a\txchat-wdk.iss/AppVerName\n\t-i\txchat-wdk.iss/VersionInfoVersion\n\t-o\txchat-wdk.iss/OutputBaseFilename\n\t-r\txchat.rc/FILEVERSION, PRODUCTVERSION\n\t-v\txchat-wdk.iss/AppVersion\n");
	}

#if 0 /* ugly hack */
	switch ((int) argv[1][0])
	{
		case 'r':	/* xchat.rc/FILEVERSION, PRODUCTVERSION*/
			printf ("#define COMMA_VERSION \"%s\"\n", comma ());
			break;
		case 'a':	/* xchat-wdk.iss/AppVerName */
			printf ("AppVerName=XChat-WDK %s\n", PACKAGE_VERSION);
			break;
		case 'v':	/* xchat-wdk.iss/AppVersion */
			printf ("AppVersion=%s\n", point ());
			break;
		case 'i':	/* xchat-wdk.iss/VersionInfoVersion */
			printf ("VersionInfoVersion=%s\n", point ());
			break;
		case 'o':	/* xchat-wdk.iss/OutputBaseFilename */
			printf ("OutputBaseFilename=XChat-WDK %s\n", PACKAGE_VERSION);
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
