/*
 * SysInfo - sysinfo plugin for HexChat
 * Copyright (c) 2015 Patrick Griffis.
 *
 * This program is free software you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <glib.h>

char *
sysinfo_format_uptime (gint64 uptime)
{
	char buffer[128];

	gint64 weeks = uptime / 604800;
	int days     = (uptime / 86400) % 7;
	int hours    = (uptime / 3600) % 24;
	int minutes  = (uptime / 60) % 60;
	int seconds  = uptime % 60;

	if (weeks != 0)
	{
		g_snprintf (buffer, sizeof(buffer), "%" G_GINT64_FORMAT "w %dd %dh %dm %ds", weeks, days, hours, minutes, seconds);
	}
	else if (days != 0)
	{
		g_snprintf (buffer, sizeof(buffer), "%dd %dh %dm %ds", days, hours, minutes, seconds);
	}
	else if (hours != 0)
	{
		g_snprintf (buffer, sizeof(buffer), "%dh %dm %ds", hours, minutes, seconds);
	}
	else if (minutes != 0)
	{
		g_snprintf (buffer, sizeof(buffer), "%dm %ds", minutes, seconds);
	}
	else
	{
		g_snprintf (buffer, sizeof(buffer), "%ds", seconds);
	}

	return g_strdup (buffer);
}

char *
sysinfo_format_memory (guint64 totalmem, guint64 freemem)
{
	char *total_fmt, *free_fmt, *ret;

	total_fmt = g_format_size_full (totalmem, G_FORMAT_SIZE_IEC_UNITS);
	free_fmt = g_format_size_full (freemem, G_FORMAT_SIZE_IEC_UNITS);
	ret = g_strdup_printf ("%s Total (%s Free)", total_fmt, free_fmt);

	g_free (total_fmt);
	g_free (free_fmt);
	return ret;
}

char *
sysinfo_format_disk (guint64 total, guint64 free)
{
	char *total_fmt, *free_fmt, *used_fmt, *ret;
	GFormatSizeFlags format_flags = G_FORMAT_SIZE_DEFAULT;

#ifdef WIN32 /* Windows uses IEC size (with SI format) */
	format_flags = G_FORMAT_SIZE_IEC_UNITS;
#endif

	total_fmt = g_format_size_full (total, format_flags);
	free_fmt = g_format_size_full (free, format_flags);
	used_fmt = g_format_size_full (total - free, format_flags);
	ret = g_strdup_printf ("%s / %s (%s Free)", used_fmt, total_fmt, free_fmt);

	g_free (total_fmt);
	g_free (free_fmt);
	g_free (used_fmt);
	return ret;
}
