/*
 * SysInfo - sysinfo plugin for HexChat
 * Copyright (c) 2015 Patrick Griffis.
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

#include <glib.h>
#include "parse.h"
#include "match.h"
#include "sysinfo.h"
#include "format.h"
#include "df.h"

char *sysinfo_backend_get_os(void)
{
	char name[bsize];

	if (xs_parse_distro (name) != 0)
	{
		return NULL;
	}

	return g_strdup(name);
}

char *sysinfo_backend_get_disk(void)
{
	gint64 total, free;

	if (xs_parse_df (&total, &free))
	{
		return NULL;
	}

	return sysinfo_format_disk (total, free);
}

char *sysinfo_backend_get_memory(void)
{
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned long long swap_total;
	unsigned long long swap_free;
	char *swap_fmt = NULL, *mem_fmt, *ret;

	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		return NULL;
	}
	if (xs_parse_meminfo (&swap_total, &swap_free, 1) != 1 && swap_total != 0)
	{
		swap_fmt = sysinfo_format_memory (swap_total, swap_free);
	}

	mem_fmt = sysinfo_format_memory (mem_total, mem_free);

	if (swap_fmt)
	{
		ret = g_strdup_printf ("Physical: %s Swap: %s", mem_fmt, swap_fmt);
		g_free (mem_fmt);
		g_free (swap_fmt);
	}
	else
		ret = mem_fmt;

	return ret;
}

char *sysinfo_backend_get_cpu(void)
{
	char model[bsize];
	char vendor[bsize];
	char buffer[bsize];
	double freq;
	int giga = 0;

	if (xs_parse_cpu (model, vendor, &freq) != 0)
	{
		return NULL;
	}

	if (freq > 1000)
	{
		freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		g_snprintf (buffer, bsize, "%s (%.2fGHz)", model, freq);
	}
	else
	{
		g_snprintf (buffer, bsize, "%s (%.0fMHz)", model, freq);
	}
	
	return g_strdup (buffer);
}

char *sysinfo_backend_get_gpu(void)
{
	char vid_card[bsize];
	char agp_bridge[bsize];
	char buffer[bsize];
	int ret;

	if ((ret = xs_parse_video (vid_card)) != 0)
	{
		return NULL;
	}

	if (xs_parse_agpbridge (agp_bridge) != 0)
	{
		g_snprintf (buffer, bsize, "%s", vid_card);
	}
	else
	{
		g_snprintf (buffer, bsize, "%s @ %s", vid_card, agp_bridge);
	}

	return g_strdup (buffer);
}

char *sysinfo_backend_get_sound(void)
{
	char sound[bsize];

	if (xs_parse_sound (sound) != 0)
	{
		return NULL;
	}
	return g_strdup (sound);
}

char *sysinfo_backend_get_uptime(void)
{
	gint64 uptime;

	if ((uptime = xs_parse_uptime ()) == 0)
	{
		return NULL;
	}

	return sysinfo_format_uptime (uptime);
}

char *sysinfo_backend_get_network(void)
{
	char ethernet_card[bsize];

	if (xs_parse_ether (ethernet_card))
	{
		g_strlcpy (ethernet_card, "None found", bsize);
	}
	
	return g_strdup (ethernet_card);
}
