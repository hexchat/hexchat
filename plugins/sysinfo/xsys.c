/*
 * SysInfo - sysinfo plugin for HexChat
 * Copyright (c) 2012 Berke Viktor.
 *
 * xsys.c - main functions for X-Sys 2
 * by mikeshoup
 * Copyright (C) 2003, 2004, 2005 Michael Shoup
 * Copyright (C) 2005, 2006, 2007 Tony Vroon
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "hexchat-plugin.h"
#include "parse.h"
#include "match.h"
#include "xsys.h"

#define DEFAULT_FORMAT "%B%1:%B %2 **"
#define DEFAULT_PERCENT 1
#define DEFAULT_ANNOUNCE 1
#define DEFAULT_PCIIDS "/usr/share/hwdata/pci.ids"

static hexchat_plugin *ph;	/* plugin handle */
static int error_printed = 0;	/* semaphore, make sure not to print the same error more than once during one execution */

static char name[] = "SysInfo";
static char desc[] = "Display info about your hardware and OS";
static char version[] = "3.0";
static char sysinfo_help[] = "SysInfo Usage:\n  /SYSINFO [-e|-o] [OS|DISTRO|CPU|RAM|DISK|VGA|SOUND|ETHERNET|UPTIME], print various details about your system or print a summary without arguments\n  /SYSINFO LIST, print current settings\n  /SYSINFO SET <variable>, update given setting\n  /SYSINFO RESET, reset settings to defaults\n  /NETDATA <iface>, show transmitted data on given interface\n  /NETSTREAM <iface>, show current bandwidth on given interface\n";

void
sysinfo_get_pciids (char* dest)
{
	hexchat_pluginpref_get_str (ph, "pciids", dest);
}

int
sysinfo_get_percent ()
{
	return hexchat_pluginpref_get_int (ph, "percent");
}

int
sysinfo_get_announce ()
{
	return hexchat_pluginpref_get_int (ph, "announce");
}

void
sysinfo_print_error (const char* msg)
{
	if (!error_printed)
	{
		hexchat_printf (ph, "%s\t%s", name, msg);
	}
	error_printed++;
	
}

static int
print_summary (int announce, char* format)
{
	char sysinfo[bsize];
	char buffer[bsize];
	char cpu_model[bsize];
	char cpu_cache[bsize];
	char cpu_vendor[bsize];
	char os_host[bsize];
	char os_user[bsize];
	char os_kernel[bsize];
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned int count;
	double cpu_freq;
	int giga = 0;
	int weeks;
	int days;
	int hours;
	int minutes;
	int seconds;
	sysinfo[0] = '\0';

	snprintf (buffer, bsize, "%s", hexchat_get_info (ph, "version"));
	format_output ("HexChat", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN OS PARSING */
	if (xs_parse_os (os_user, os_host, os_kernel) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_os()", name);
		return HEXCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s", os_kernel);
	format_output ("OS", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN DISTRO PARSING */
        if (xs_parse_distro (buffer) != 0)
        {
		strncpy (buffer, "Unknown", bsize);
	}

	format_output ("Distro", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));	

	/* BEGIN CPU PARSING */
	if (xs_parse_cpu (cpu_model, cpu_vendor, &cpu_freq, cpu_cache, &count) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_cpu()", name);
		return HEXCHAT_EAT_ALL;
	}

	if (cpu_freq > 1000)
	{
		cpu_freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		snprintf (buffer, bsize, "%u x %s (%s) @ %.2fGHz", count, cpu_model, cpu_vendor, cpu_freq);
	}
	else
	{
		snprintf (buffer, bsize, "%u x %s (%s) @ %.0fMHz", count, cpu_model, cpu_vendor, cpu_freq);
	}

	format_output ("CPU", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN MEMORY PARSING */
	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		hexchat_printf (ph, "%s\tERROR in parse_meminfo!", name);
		return HEXCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s", pretty_freespace ("Physical", &mem_free, &mem_total));
	format_output ("RAM", buffer, format);	
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN DISK PARSING */
	if (xs_parse_df (NULL, buffer))
	{
		hexchat_printf (ph, "%s\tERROR in parse_df", name);
		return HEXCHAT_EAT_ALL;
	}

	format_output ("Disk", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN VIDEO PARSING */
	if (xs_parse_video (buffer))
	{
		hexchat_printf (ph, "%s\tERROR in parse_video", name);
		return HEXCHAT_EAT_ALL;
	}

	format_output ("VGA", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN SOUND PARSING */
	if (xs_parse_sound (buffer))
	{
		strncpy (buffer, "Not present", bsize);
	}

	format_output ("Sound", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN ETHERNET PARSING */
	if (xs_parse_ether (buffer))
	{
		strncpy (buffer, "None found", bsize);
	}

	format_output ("Ethernet", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN UPTIME PARSING */
	if (xs_parse_uptime (&weeks, &days, &hours, &minutes, &seconds))
	{
		hexchat_printf (ph, "%s\tERROR in parse_uptime()", name);
		return HEXCHAT_EAT_ALL;
	}

	if (minutes != 0 || hours != 0 || days != 0 || weeks != 0)
	{
		if (hours != 0 || days != 0 || weeks != 0)
		{
			if (days  !=0 || weeks != 0)
			{
				if (weeks != 0)
				{
					snprintf (buffer, bsize, "%dw %dd %dh %dm %ds", weeks, days, hours, minutes, seconds);
				}
				else
				{
					snprintf (buffer, bsize, "%dd %dh %dm %ds", days, hours, minutes, seconds);
				}
			}
			else
			{
				snprintf (buffer, bsize, "%dh %dm %ds", hours, minutes, seconds);
			}
		}
		else
		{
			snprintf (buffer, bsize, "%dm %ds", minutes, seconds);
		}
	}

        format_output ("Uptime", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", sysinfo);
	}
	else
	{
		hexchat_printf (ph, "%s", sysinfo);
	}

	return HEXCHAT_EAT_ALL;
}

static int
print_os (int announce, char* format)
{
	char buffer[bsize];
	char user[bsize];
	char host[bsize];
	char kernel[bsize];

	if (xs_parse_os (user, host, kernel) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_os()", name);
		return HEXCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s@%s, %s", user, host, kernel);
	format_output ("OS", buffer, format);
	
	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", buffer);
	}
	else
	{
		hexchat_printf (ph, "%s", buffer);
	}

	return HEXCHAT_EAT_ALL;
}

static int
print_distro (int announce, char* format)
{
	char name[bsize];

	if (xs_parse_distro (name) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_distro()!", name);
		return HEXCHAT_EAT_ALL;
	}

	format_output("Distro", name, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", name);
	}
	else
	{
		hexchat_printf (ph, "%s", name);
	}
	return HEXCHAT_EAT_ALL;
}

static int
print_cpu (int announce, char* format)
{
	char model[bsize];
	char vendor[bsize];
	char cache[bsize];
	char buffer[bsize];
	unsigned int count;
	double freq;
	int giga = 0;

	if (xs_parse_cpu (model, vendor, &freq, cache, &count) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_cpu()", name);
		return HEXCHAT_EAT_ALL;
	}

	if (freq > 1000)
	{
		freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		snprintf (buffer, bsize, "%u x %s (%s) @ %.2fGHz w/ %s L2 Cache", count, model, vendor, freq, cache);
	}
	else
	{
		snprintf (buffer, bsize, "%u x %s (%s) @ %.0fMHz w/ %s L2 Cache", count, model, vendor, freq, cache);
	}

	format_output ("CPU", buffer, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", buffer);
	}
	else
	{
		hexchat_printf (ph, "%s", buffer);
	}

	return HEXCHAT_EAT_ALL;
}

static int
print_ram (int announce, char* format)
{
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned long long swap_total;
	unsigned long long swap_free;
	char string[bsize];

	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		hexchat_printf (ph, "%s\tERROR in parse_meminfo!", name);
		return HEXCHAT_EAT_ALL;
	}
	if (xs_parse_meminfo (&swap_total, &swap_free, 1) == 1)
	{
		hexchat_printf (ph, "%s\tERROR in parse_meminfo!", name);
		return HEXCHAT_EAT_ALL;
	}

	snprintf (string, bsize, "%s - %s", pretty_freespace ("Physical", &mem_free, &mem_total), pretty_freespace ("Swap", &swap_free, &swap_total));
	format_output ("RAM", string, format);
	
	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", string);
	}
	else
	{
		hexchat_printf (ph, "%s", string);
	}
	
	return HEXCHAT_EAT_ALL;
}

static int
print_disk (int announce, char* format)
{
	char string[bsize] = {0,};

#if 0
	if (*word == '\0')
	{
		if (xs_parse_df (NULL, string))
		{
			hexchat_printf (ph, "ERROR in parse_df");
			return HEXCHAT_EAT_ALL;
		}
	}
	else
	{
		if (xs_parse_df (*word, string))
		{
			hexchat_printf (ph, "ERROR in parse_df");
			return HEXCHAT_EAT_ALL;
		}
	}
#endif

	if (xs_parse_df (NULL, string))
	{
		hexchat_printf (ph, "%s\tERROR in parse_df", name);
		return HEXCHAT_EAT_ALL;
	}

	format_output ("Disk", string, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", string);
	}
	else
	{
		hexchat_printf (ph, "%s", string);
	}

	return HEXCHAT_EAT_ALL;
}

static int
print_vga (int announce, char* format)
{
	char vid_card[bsize];
	char agp_bridge[bsize];
	char buffer[bsize];
	int ret;

	if ((ret = xs_parse_video (vid_card)) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_video! %d", name, ret);
		return HEXCHAT_EAT_ALL;
	}

	if (xs_parse_agpbridge (agp_bridge) != 0)
	{
		snprintf (buffer, bsize, "%s", vid_card);
	}
	else
	{
		snprintf (buffer, bsize, "%s @ %s", vid_card, agp_bridge);
	}

	format_output ("VGA", buffer, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", buffer);
	}
	else
	{
		hexchat_printf (ph, "%s", buffer);
	}

	return HEXCHAT_EAT_ALL;
}

static int
print_sound (int announce, char* format)
{
	char sound[bsize];

	if (xs_parse_sound (sound) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_asound()!", name);
		return HEXCHAT_EAT_ALL;
	}

	format_output ("Sound", sound, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", sound);
	}
	else
	{
		hexchat_printf (ph, "%s", sound);
	}

	return HEXCHAT_EAT_ALL;
}


static int
print_ethernet (int announce, char* format)
{
	char ethernet_card[bsize];

	if (xs_parse_ether (ethernet_card))
	{
		strncpy (ethernet_card, "None found", bsize);
	}

	format_output ("Ethernet", ethernet_card, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", ethernet_card);
	}
	else
	{
		hexchat_printf (ph, "%s", ethernet_card);
	}

	return HEXCHAT_EAT_ALL;
}

static int
print_uptime (int announce, char* format)
{
	char buffer[bsize];
	int weeks;
	int days;
	int hours;
	int minutes;
	int seconds;

	if (xs_parse_uptime (&weeks, &days, &hours, &minutes, &seconds))
	{
		hexchat_printf (ph, "%s\tERROR in parse_uptime()", name);
		return HEXCHAT_EAT_ALL;
	}

	if (minutes != 0 || hours != 0 || days != 0 || weeks != 0)
	{
		if (hours != 0 || days != 0 || weeks != 0)
		{
			if (days  !=0 || weeks != 0)
			{
				if (weeks != 0)
				{
					snprintf (buffer, bsize, "%dw %dd %dh %dm %ds", weeks, days, hours, minutes, seconds);
				}
				else
				{
					snprintf (buffer, bsize, "%dd %dh %dm %ds", days, hours, minutes, seconds);
				}
			}
			else
			{
				snprintf (buffer, bsize, "%dh %dm %ds", hours, minutes, seconds);
			}
		}
		else
		{
			snprintf (buffer, bsize, "%dm %ds", minutes, seconds);
		}
	}

	format_output ("Uptime", buffer, format);

	if (announce)
	{
		hexchat_commandf (ph, "SAY %s", buffer);
	}
	else
	{
		hexchat_printf (ph, "%s", buffer);
	}

	return HEXCHAT_EAT_ALL;
}

static int
netdata_cb (char *word[], char *word_eol[], void *userdata)
{
	char netdata[bsize];
	char format[bsize];
	unsigned long long bytes_recv;
	unsigned long long bytes_sent;
	
	if (*word[2] == '\0')
	{
		hexchat_printf (ph, "%s\tYou must specify a network device (e.g. /NETDATA eth0)!", name);
		return HEXCHAT_EAT_ALL;
	}

	if (xs_parse_netdev (word[2], &bytes_recv, &bytes_sent) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_netdev", name);
		return HEXCHAT_EAT_ALL;
	}

	bytes_recv /= 1024;
	bytes_sent /= 1024;
	
	snprintf (netdata, bsize, "%s: %.1f MB Received, %.1f MB Sent", word[2], (double)bytes_recv/1024.0, (double)bytes_sent/1024.0);
	hexchat_pluginpref_get_str (ph, "format", format);
	format_output ("Netdata", netdata, format);

	if (hexchat_list_int (ph, NULL, "type") >= 2)
	{
		hexchat_commandf (ph, "SAY %s", netdata);
	}
	else
	{
		hexchat_printf (ph, "%s", netdata);
	}
	
	return HEXCHAT_EAT_ALL;
}

static int
netstream_cb (char *word[], char *word_eol[], void *userdata)
{
	char netstream[bsize];
	char mag_r[5];
	char mag_s[5];
	char format[bsize];
	unsigned long long bytes_recv;
	unsigned long long bytes_sent;
	unsigned long long bytes_recv_p;
	unsigned long long bytes_sent_p;

	struct timespec ts = {1, 0};

	if (*word[2] == '\0')
	{
		hexchat_printf (ph, "%s\tYou must specify a network device (e.g. /NETSTREAM eth0)!", name);
		return HEXCHAT_EAT_ALL;
	}

	if (xs_parse_netdev(word[2], &bytes_recv, &bytes_sent) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_netdev", name);
		return HEXCHAT_EAT_ALL;
	}

	while (nanosleep (&ts, &ts) < 0);

	if (xs_parse_netdev(word[2], &bytes_recv_p, &bytes_sent_p) != 0)
	{
		hexchat_printf (ph, "%s\tERROR in parse_netdev", name);
		return HEXCHAT_EAT_ALL;
	}

	bytes_recv = (bytes_recv_p - bytes_recv);
	bytes_sent = (bytes_sent_p - bytes_sent);

	if (bytes_recv > 1024)
	{
		bytes_recv /= 1024;
		snprintf (mag_r, 5, "KB/s");
	}
	else
	{
		snprintf (mag_r, 5, "B/s");
	}

	if (bytes_sent > 1024)
	{
		bytes_sent /= 1024;
		snprintf (mag_s, 5, "KB/s");
	}
	else
	{
		snprintf (mag_s, 5, "B/s");
	}

	snprintf (netstream, bsize, "%s: Receiving %llu %s, Sending %llu %s", word[2], bytes_recv, mag_r, bytes_sent, mag_s);
	hexchat_pluginpref_get_str (ph, "format", format);
	format_output ("Netstream", netstream, format);

	if (hexchat_list_int (ph, NULL, "type") >= 2)
	{
		hexchat_commandf (ph, "SAY %s", netstream);
	}
	else
	{
		hexchat_printf (ph, "%s", netstream);
	}

	return HEXCHAT_EAT_ALL;
}

static void
list_settings ()
{
	char list[512];
	char buffer[512];
	char* token;

	hexchat_pluginpref_list (ph, list);
	hexchat_printf (ph, "%s\tCurrent Settings:", name);
	token = strtok (list, ",");

	while (token != NULL)
	{
		hexchat_pluginpref_get_str (ph, token, buffer);
		hexchat_printf (ph, "%s\t%s: %s\n", name, token, buffer);
		token = strtok (NULL, ",");
	}
}

static void
reset_settings ()
{
	hexchat_pluginpref_set_str (ph, "pciids", DEFAULT_PCIIDS);
	hexchat_pluginpref_set_str (ph, "format", DEFAULT_FORMAT);
	hexchat_pluginpref_set_int (ph, "percent", DEFAULT_PERCENT);
	hexchat_pluginpref_set_int (ph, "announce", DEFAULT_ANNOUNCE);
}

static int
sysinfo_cb (char *word[], char *word_eol[], void *userdata)
{
	error_printed = 0;
	int announce = sysinfo_get_announce ();
	int offset = 0;
	int buffer;
	char format[bsize];

	if (!hexchat_pluginpref_get_str (ph, "format", format))
	{
		hexchat_printf (ph, "%s\tError reading config file!", name);
		return HEXCHAT_EAT_ALL;
	}

	/* Cannot send to server tab */
	if (hexchat_list_int (ph, NULL, "type") == 1)
	{
		announce = 0;
	}

	/* Allow overriding global announce setting */
	if (!strcmp ("-e", word[2]))
	{
		announce = 0;
		offset++;
	}
	else if (!strcmp ("-o", word[2]))
	{
		announce = 1;
		offset++;
	}

	if (!g_ascii_strcasecmp ("HELP", word[2+offset]))
	{
		hexchat_printf (ph, sysinfo_help);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("LIST", word[2+offset]))
	{
		list_settings ();
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("SET", word[2+offset]))
	{
		if (!g_ascii_strcasecmp ("", word_eol[4+offset]))
		{
			hexchat_printf (ph, "%s\tEnter a value!\n", name);
			return HEXCHAT_EAT_ALL;
		}
		if (!g_ascii_strcasecmp ("format", word[3+offset]))
		{
			hexchat_pluginpref_set_str (ph, "format", word_eol[4+offset]);
			hexchat_printf (ph, "%s\tformat is set to: %s\n", name, word_eol[4+offset]);
		}
		else if (!g_ascii_strcasecmp ("percent", word[3+offset]))
		{
			buffer = atoi (word[4+offset]);	/* don't use word_eol, numbers must not contain spaces */

			if (buffer > 0 && buffer < INT_MAX)
			{
				hexchat_pluginpref_set_int (ph, "percent", buffer);
				hexchat_printf (ph, "%s\tpercent is set to: %d\n", name, buffer);
			}
			else
			{
				hexchat_printf (ph, "%s\tInvalid input!\n", name);
			}
		}
		else if (!g_ascii_strcasecmp ("announce", word[3+offset]))
		{
			buffer = atoi (word[4+offset]);	/* don't use word_eol, numbers must not contain spaces */

			if (buffer > 0)
			{
				hexchat_pluginpref_set_int (ph, "announce", 1);
				hexchat_printf (ph, "%s\tannounce is set to: On\n", name);
			}
			else
			{
				hexchat_pluginpref_set_int (ph, "announce", 0);
				hexchat_printf (ph, "%s\tannounce is set to: Off\n", name);
			}
		}
		else if (!g_ascii_strcasecmp ("pciids", word[3+offset]))
		{
			hexchat_pluginpref_set_str (ph, "pciids", word_eol[4+offset]);
			hexchat_printf (ph, "%s\tpciids is set to: %s\n", name, word_eol[4+offset]);
		}
		else
		{
			hexchat_printf (ph, "%s\tInvalid variable name! Use 'pciids', 'format' or 'percent'!\n", name);
			return HEXCHAT_EAT_ALL;
		}

		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("RESET", word[2+offset]))
	{
		reset_settings ();
		hexchat_printf (ph, "%s\tSettings have been restored to defaults.\n", name);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("OS", word[2+offset]))
	{
		print_os (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("DISTRO", word[2+offset]))
	{
		print_distro (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("CPU", word[2+offset]))
	{
		print_cpu (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("RAM", word[2+offset]))
	{
		print_ram (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("DISK", word[2+offset]))
	{
		print_disk (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("VGA", word[2+offset]))
	{
		print_vga (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("SOUND", word[2+offset]))
	{
		print_sound (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("ETHERNET", word[2+offset]))
	{
		print_ethernet (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("UPTIME", word[2+offset]))
	{
		print_uptime (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("", word[2+offset]))
	{
		print_summary (announce, format);
		return HEXCHAT_EAT_ALL;
	}
	else
	{
		hexchat_printf (ph, sysinfo_help);
		return HEXCHAT_EAT_ALL;
	}
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;
	*plugin_name    = name;
	*plugin_desc    = desc;
	*plugin_version = version;
	char buffer[bsize];

	hexchat_hook_command (ph, "SYSINFO",	HEXCHAT_PRI_NORM,	sysinfo_cb,	sysinfo_help, NULL);
	hexchat_hook_command (ph, "NETDATA",	HEXCHAT_PRI_NORM,	netdata_cb,	NULL, NULL);
	hexchat_hook_command (ph, "NETSTREAM",	HEXCHAT_PRI_NORM,	netstream_cb,	NULL, NULL);

	/* this is required for the very first run */
	if (hexchat_pluginpref_get_str (ph, "pciids", buffer) == 0)
	{
		hexchat_pluginpref_set_str (ph, "pciids", DEFAULT_PCIIDS);
	}

	if (hexchat_pluginpref_get_str (ph, "format", buffer) == 0)
	{
		hexchat_pluginpref_set_str (ph, "format", DEFAULT_FORMAT);
	}

	if (hexchat_pluginpref_get_int (ph, "percent") == -1)
	{
		hexchat_pluginpref_set_int (ph, "percent", DEFAULT_PERCENT);
	}

	if (hexchat_pluginpref_get_int (ph, "announce") == -1)
	{
		hexchat_pluginpref_set_int (ph, "announce", DEFAULT_ANNOUNCE);
	}

	hexchat_command (ph, "MENU ADD \"Window/Send System Info\" \"SYSINFO\"");
	hexchat_printf (ph, "%s plugin loaded\n", name);
	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_command (ph, "MENU DEL \"Window/Display System Info\"");
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
