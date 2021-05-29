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

#include <fcntl.h>
#include <time.h>

#include "cfgfiles.h"
#include "sts.h"
#include "util.h"

#define STS_CONFIG_FILE "sts.conf"

GSList *profiles = NULL;

struct sts_profile *
sts_find (const char* host)
{
	time_t now;
	GList *next;
	struct sts_profile *nextprofile;

	now = time (NULL);
	for (next = profiles; next; next = next->next)
	{
		nextprofile = (struct sts_profile *)next->data;
		if (now >= nextprofile->expiry)
			continue; /* Profile has expired. */

		if (!g_strcmp0 (host, nextprofile->host))
			return nextprofile; /* We found the right profile! */
	}

	/* No profile for this host. */
	return NULL;
}

void
sts_load (void)
{
	char buf[256];
	int fh;
	struct sts_profile *profile;

	fh = hexchat_open_file (STS_CONFIG_FILE, O_RDONLY, 0, 0);
	if (fh < 0)
		return; /* Filesystem not readable. */

	while (waitline (fh, buf, sizeof buf, FALSE) != -1)
	{
		profile = sts_new ();
		if (sscanf (buf, "%s %u %ld", profile->host, &profile->port, &profile->expiry) != 3)
		{
			/* Malformed profile; drop it. */
			g_free (profile);
			continue;
		}

		profiles = g_slist_append (profiles, profile);
	}
}

struct sts_profile *
sts_new (void)
{
	struct sts_profile *profile;

	profile = malloc (sizeof (struct sts_profile));
	profile->host[0] = 0;
	profile->port = 0;
	profile->expiry = 0;
	return profile;
}

void
sts_save (void)
{
	char buf[512];
	int fh;
	const GSList *next;
	struct sts_profile *nextprofile;
	time_t now;

	fh = hexchat_open_file (STS_CONFIG_FILE, O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh < 0)
		return; /* Filesystem not writable. */

	now = time (NULL);
	for (next = profiles; next; next = profiles->next)
	{
		nextprofile = (struct sts_profile *)next->data;
		if (now >= nextprofile->expiry)
			continue; /* Profile has expired. */

		g_snprintf (buf, sizeof buf, "%s %u %ld\n", nextprofile->host, nextprofile->port,
			(unsigned long)nextprofile->expiry);
		write (fh, buf, strlen (buf));
	}

	close (fh);
}
