/*
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "sysinfo.h"

int xs_parse_df(gint64 *out_total, gint64 *out_free)
{
	FILE *pipe;
	char buffer[bsize];
	
	pipe = popen("df -k -l -P --exclude-type=squashfs --exclude-type=devtmpfs --exclude-type=tmpfs", "r");
	if(pipe==NULL)
		return 1;

	*out_total = *out_free = 0;

	while(fgets(buffer, bsize, pipe) != NULL)
	{
		long long int avail, total;

		/* Filesystem 1024-blocks Used Available Capacity Mounted-on */
		if (sscanf (buffer, "%*s %lld %*s %lld %*s %*s", &total, &avail) == 2)
		{
			*out_total += total;
			*out_free += avail;
		}
	}

	/* Convert to bytes */
	*out_total *= 1000;
	*out_free *= 1000;

	pclose(pipe);
	return 0;
}
