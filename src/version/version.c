/* HexChat
 * Copyright (c) 2011-2012 Berke Viktor.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "../../config.h"

char *
comma ()
{
# if 0	/* for WDK version numbers */
	int major, minor;
	char *version_string; /* nnnn,n,n,n format */

	version_string = (char*) malloc (11);

	if (sscanf (PACKAGE_VERSION, "%d-%d", &major, &minor) > 1)
	{
		sprintf (version_string, "%d,%d,0,0", major, minor);
	}
	else
	{
		sprintf (version_string, "%d,0,0,0", major);
	}
#endif
	int major, minor, build;
	char *version_string; /* n,n,n,n format */

	version_string = (char*) malloc (8);

	sscanf (PACKAGE_VERSION, "%d.%d.%d", &major, &minor, &build);
	sprintf (version_string, "%d,%d,%d,0", major, minor, build);

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
