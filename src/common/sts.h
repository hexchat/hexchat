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

#ifndef HEXCHAT_STS_H
#define HEXCHAT_STS_H

#include <time.h>

#include <glib.h>

/* Encapsulates a STS profile.
 *
 * See https://ircv3.net/specs/extensions/sts
 */
struct sts_profile
{
	/* The hostname the profile applies to. */
	char host[256];

	/* The port all secure connections should happen on. */
	unsigned int port;

	/* The time at which this STS profile expires. */
	time_t expiry;
};


/* Searches for a STS profile that matches the specified hostname */
struct sts_profile *sts_find (const char *host);

/* Loads STS profiles from sts.conf */
void sts_load (void);

/* Creates a new empty STS profile. */
struct sts_profile * sts_new (void);

/* Saves STS profiles to sts.conf */
void sts_save (void);

#endif
