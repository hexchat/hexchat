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
sts_find (const char *host)
{
	time_t now;
	GSList *next;
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
	g_autoptr (GFile) file = NULL;
	g_autoptr (GFileInputStream) input = NULL;
	g_autoptr (GDataInputStream) stream = NULL;
	char *path;
	char *line;
	struct sts_profile *profile;

	path = g_build_filename (get_xdir (), STS_CONFIG_FILE, NULL);
	file = g_file_new_for_path (path);
	g_free (path);

	input = g_file_read (file, NULL, NULL);
	if (!input)
		return; /* Filesystem not readable. */

	stream = g_data_input_stream_new (G_INPUT_STREAM (input));
	while ((line = g_data_input_stream_read_line (stream, NULL, NULL, NULL)))
	{
		profile = sts_new ();
		if (sscanf (line, "%s %u %ld", profile->host, &profile->port, &profile->expiry) != 3)
		{
			/* Malformed profile; drop it. */
			g_debug ("Malformed STS profile: %s", line);
			g_free (line);
			g_free (profile);
			continue;
		}

		g_free (line);
		sts_store (profile);
	}
}

struct sts_profile *
sts_new (void)
{
	struct sts_profile *profile;

	profile = g_new0 (struct sts_profile, 1);
	return profile;
}

GHashTable *
sts_parse_cap (const char *cap)
{
	char **entries, **currentry;
	char *value;
	GHashTable *table;

	table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	entries = g_strsplit (cap, ",", 0);
	for (currentry = entries; *currentry; ++currentry)
	{
		value = strchr (*currentry, '=');
		if (value)
			g_hash_table_insert (table, g_strndup (*currentry, value - *currentry), g_strdup (value + 1));
	}

	g_strfreev (entries);
	return table;
}

void
sts_save (void)
{
	g_autoptr (GFile) file = NULL;
	g_autoptr (GFileOutputStream) output = NULL;
	g_autoptr (GDataOutputStream) stream = NULL;
	char buf[512];
	const GSList *next;
	char *path;
	struct sts_profile *nextprofile;
	time_t now;

	path = g_build_filename (get_xdir (), STS_CONFIG_FILE, NULL);
	file = g_file_new_for_path (path);
	g_free (path);

	output = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
	if (!output)
		return; /* Filesystem not readable. */

	now = time (NULL);
	stream = g_data_output_stream_new (G_OUTPUT_STREAM (output));
	for (next = profiles; next; next = next->next)
	{
		nextprofile = (struct sts_profile *)next->data;
		if (now >= nextprofile->expiry)
			continue; /* Profile has expired. */

		g_snprintf (buf, sizeof buf, "%s %u %ld\n", nextprofile->host, nextprofile->port,
			(unsigned long)nextprofile->expiry);

		if (!g_data_output_stream_put_string (stream, buf, NULL, NULL))
			g_debug ("Failed to save STS profile: %s", buf);
	}
}

void
sts_store (struct sts_profile *profile)
{
	profiles = g_slist_append (profiles, profile);
}

void sts_update_expiry (const char *host, time_t newexpiry, struct sts_profile *incomplete_profile)
{
	struct sts_profile *profile;

	profile = incomplete_profile ? incomplete_profile : sts_find (host);
	if (profile)
		profile->expiry = time (NULL) + newexpiry;
}
