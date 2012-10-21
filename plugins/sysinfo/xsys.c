/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xchat-plugin.h"
#include "parse.h"
#include "match.h"
#include "xsys.h"

#define DEFAULT_FORMAT "%B%1:%B %2 **"
#define DEFAULT_PERCENTAGES 1
#define DEFAULT_PCIIDS "/usr/share/hwdata/pci.ids"

static xchat_plugin *ph;

static char name[] = "SysInfo";
static char desc[] = "Display info about your hardware and OS";
static char version[] = "2.2";

static int
cpuinfo_cb (char *word[], char *word_eol[], void *userdata)
{
	char model[bsize];
	char vendor[bsize];
	char cache[bsize];
	char buffer[bsize];
	char format[bsize];
	unsigned int count;
	double freq;
	int giga = 0;

	if (xs_parse_cpu (model, vendor, &freq, cache, &count) != 0)
	{
		xchat_printf (ph, "ERROR in parse_cpu()");
		return XCHAT_EAT_ALL;
	}

	if (freq > 1000)
	{
		freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.2fGHz w/ %s L2 Cache", count, model, vendor, freq, cache);
	}
	else
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.0fMHz w/ %s L2 Cache", count, model, vendor, freq, cache);
	}

	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("CPU", buffer, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", buffer);
	}
	else
	{
		xchat_commandf (ph, "say %s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
uptime_cb (char *word[], char *word_eol[], void *userdata)
{
	char buffer[bsize];
	char format[bsize];
	int weeks;
	int days;
	int hours;
	int minutes;
	int seconds;

	if (xs_parse_uptime (&weeks, &days, &hours, &minutes, &seconds))
	{
		xchat_printf (ph, "ERROR in parse_uptime()");
		return XCHAT_EAT_ALL;
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

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", buffer);
	}
	else
	{
		xchat_commandf (ph, "say %s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
osinfo_cb (char *word[], char *word_eol[], void *userdata)
{
	char buffer[bsize];
	char user[bsize];
	char host[bsize];
	char kernel[bsize];
	char format[bsize];

	if (xs_parse_os (user, host, kernel) != 0)
	{
		xchat_printf (ph, "ERROR in parse_os()");
		return XCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s@%s, %s", user, host, kernel);
	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("OS", buffer, format);
	
	if ((long)userdata)
	{
		xchat_printf (ph, "%s", buffer);
	}
	else
	{
		xchat_commandf (ph, "say %s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
sound_cb (char *word[], char *world_eol[], void *userdata)
{
	char sound[bsize];
	char format[bsize];

	if (xs_parse_sound (sound) != 0)
	{
		xchat_printf (ph, "ERROR in parse_asound()!");
		return XCHAT_EAT_ALL;
	}

	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Sound", sound, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", sound);
	}
	else
	{
		xchat_commandf (ph, "say %s", sound);
	}

	return XCHAT_EAT_ALL;
}

static int
distro_cb (char *word[], char *word_eol[], void *userdata)
{
	char name[bsize];
	char format[bsize];

	if (xs_parse_distro (name) != 0)
	{
		xchat_printf (ph, "ERROR in parse_distro()!");
		return XCHAT_EAT_ALL;
	}

	xchat_pluginpref_get_str (ph, "format", format);
	format_output("Distro", name, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", name);
	}
	else
	{
		xchat_commandf (ph, "say %s", name);
	}
	return XCHAT_EAT_ALL;
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
		xchat_printf (ph, "You must specify a network device! (eg.: /netdata eth0)");
		return XCHAT_EAT_ALL;
	}

	if (xs_parse_netdev (word[2], &bytes_recv, &bytes_sent) != 0)
	{
		xchat_printf (ph, "ERROR in parse_netdev");
		return XCHAT_EAT_ALL;
	}

	bytes_recv /= 1024;
	bytes_sent /= 1024;
	
	snprintf (netdata, bsize, "%s: %.1f MB Recieved, %.1f MB Sent", word[2], (double)bytes_recv/1024.0, (double)bytes_sent/1024.0);
	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Netdata", netdata, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", netdata);
	}
	else
	{
		xchat_commandf (ph, "say %s", netdata);
	}
	
	return XCHAT_EAT_ALL;
}

static int
netstream_cb (char *word[], char *word_eol[], void *userdata)
{
	char netstream[bsize];
	char mag_r[3];
	char mag_s[5];
	char format[bsize];
	unsigned long long bytes_recv;
	unsigned long long bytes_sent;
	unsigned long long bytes_recv_p;
	unsigned long long bytes_sent_p;

	struct timespec ts = {1, 0};

	if (*word[2] == '\0')
	{
		xchat_printf (ph, "You must specify a network device! (eg.: /netstream eth0)");
		return XCHAT_EAT_ALL;
	}

	if (xs_parse_netdev(word[2], &bytes_recv, &bytes_sent) != 0)
	{
		xchat_printf (ph, "ERROR in parse_netdev");
		return XCHAT_EAT_ALL;
	}

	while (nanosleep (&ts, &ts) < 0);

	if (xs_parse_netdev(word[2], &bytes_recv_p, &bytes_sent_p) != 0)
	{
		xchat_printf (ph, "ERROR in parse_netdev");
		return XCHAT_EAT_ALL;
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
	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Netstream", netstream, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", netstream);
	}
	else
	{
		xchat_commandf (ph, "say %s", netstream);
	}

	return XCHAT_EAT_ALL;
}

static int
disk_cb (char *word[], char *word_eol[], void *userdata)
{
	char string[bsize] = {0,};
	char format[bsize];

	if (*word[2] == '\0')
	{
		if (xs_parse_df(NULL, string))
		{
			xchat_printf (ph, "ERROR in parse_df");
			return XCHAT_EAT_ALL;
		}
	}
	else
	{
		if (xs_parse_df(word[2], string))
		{
			xchat_printf (ph, "ERROR in parse_df");
			return XCHAT_EAT_ALL;
		}
	}

	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Disk", string, format);
	
	if ((long)userdata)
	{
		xchat_printf(ph, "%s", string);
	}
	else
	{
		xchat_commandf(ph, "say %s", string);
	}

	return XCHAT_EAT_ALL;
}

static int
mem_cb (char *word[], char *word_eol[], void *userdata)
{
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned long long swap_total;
	unsigned long long swap_free;
	char string[bsize];
	char format[bsize];

	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		xchat_printf (ph, "ERROR in parse_meminfo!");
		return XCHAT_EAT_ALL;
	}
	if (xs_parse_meminfo (&swap_total, &swap_free, 1) == 1)
	{
		xchat_printf (ph, "ERROR in parse_meminfo!");
		return XCHAT_EAT_ALL;
	}

	snprintf (string, bsize, "%s - %s", pretty_freespace ("Physical", &mem_free, &mem_total), pretty_freespace ("Swap", &swap_free, &swap_total));
 	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("RAM", string, format);
	
	if ((long)userdata)
	{
		xchat_printf (ph, "%s", string);
	}
	else
	{
		xchat_commandf (ph, "say %s", string);
	}
	
	return XCHAT_EAT_ALL;
}

static int
video_cb (char *word[], char *word_eol[], void *userdata)
{
	char vid_card[bsize];
	char agp_bridge[bsize];
	char buffer[bsize];
	char format[bsize];
	int ret;

	if ((ret = xs_parse_video (vid_card)) != 0)
	{
		xchat_printf (ph, "ERROR in parse_video! %d", ret);
		return XCHAT_EAT_ALL;
	}

	if (xs_parse_agpbridge (agp_bridge) != 0)
	{
		snprintf (buffer, bsize, "%s", vid_card);
	}
	else
	{
		snprintf (buffer, bsize, "%s @ %s", vid_card, agp_bridge);
	}

	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("VGA", buffer, format);
	if ((long)userdata)
	{
		xchat_printf (ph, "%s", buffer);
	}
	else
	{
		xchat_commandf (ph, "say %s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
ether_cb (char *word[], char *word_eol[], void *userdata)
{
	char ethernet_card[bsize];
	char format[bsize];

	if (xs_parse_ether (ethernet_card))
	{
		strncpy (ethernet_card, "None found", bsize);
	}

	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Ethernet", ethernet_card, format);

	if ((long)userdata)
	{
		xchat_printf(ph, "%s", ethernet_card);
	}
	else
	{
		xchat_commandf(ph, "say %s", ethernet_card);
	}

	return XCHAT_EAT_ALL;
}

int
sysinfo_get_percentages ()
{
	return xchat_pluginpref_get_int (ph, "percentages");
}

void
sysinfo_get_pciids (char* dest)
{
	xchat_pluginpref_get_str (ph, "pciids", dest);
}

static int
sysinfo_cb (char *word[], char *word_eol[], void *userdata)
{
	char sysinfo[bsize];
	char buffer[bsize];
	char cpu_model[bsize];
	char cpu_cache[bsize];
	char cpu_vendor[bsize];
	char os_host[bsize];
	char os_user[bsize];
	char os_kernel[bsize];
	char format[bsize];
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned int count;
	double cpu_freq;
	int giga = 0;
	sysinfo[0] = '\0';

	xchat_pluginpref_get_str (ph, "format", format);

	/* BEGIN OS PARSING */
	if (xs_parse_os (os_user, os_host, os_kernel) != 0)
	{
		xchat_printf (ph, "ERROR in parse_os()");
		return XCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s", os_kernel);
	format_output ("OS", buffer, format);
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
		xchat_printf (ph, "ERROR in parse_cpu()");
		return XCHAT_EAT_ALL;
	}

	if (cpu_freq > 1000)
	{
		cpu_freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.2fGHz", count, cpu_model, cpu_vendor, cpu_freq);
	}
	else
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.0fMHz", count, cpu_model, cpu_vendor, cpu_freq);
	}

	format_output ("CPU", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN MEMORY PARSING */
	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		xchat_printf (ph, "ERROR in parse_meminfo!");
		return XCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s", pretty_freespace ("Physical", &mem_free, &mem_total));
	format_output ("RAM", buffer, format);	
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN DISK PARSING */
	if (xs_parse_df (NULL, buffer))
	{
		xchat_printf (ph, "ERROR in parse_df");
		return XCHAT_EAT_ALL;
	}

	format_output ("Disk", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN VIDEO PARSING */
	if (xs_parse_video (buffer))
	{
		xchat_printf (ph, "ERROR in parse_video");
		return XCHAT_EAT_ALL;
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

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", sysinfo);
	}
	else
	{
		xchat_commandf (ph, "say %s", sysinfo);	
	}

	return XCHAT_EAT_ALL;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;
	*plugin_name    = name;
	*plugin_desc    = desc;
	*plugin_version = version;
	char buffer[bsize];

	xchat_hook_command (ph, "SYSINFO",    XCHAT_PRI_NORM, sysinfo_cb,   NULL, (void *) 0);
	xchat_hook_command (ph, "ESYSINFO",   XCHAT_PRI_NORM, sysinfo_cb,   NULL, (void *) 1);
	xchat_hook_command (ph, "CPUINFO",    XCHAT_PRI_NORM, cpuinfo_cb,   NULL, (void *) 0);
	xchat_hook_command (ph, "ECPUINFO",   XCHAT_PRI_NORM, cpuinfo_cb,   NULL, (void *) 1);
	xchat_hook_command (ph, "SYSUPTIME",  XCHAT_PRI_NORM, uptime_cb,    NULL, (void *) 0);
	xchat_hook_command (ph, "ESYSUPTIME", XCHAT_PRI_NORM, uptime_cb,    NULL, (void *) 1);
	xchat_hook_command (ph, "OSINFO",     XCHAT_PRI_NORM, osinfo_cb,    NULL, (void *) 0);
	xchat_hook_command (ph, "EOSINFO",    XCHAT_PRI_NORM, osinfo_cb,    NULL, (void *) 1);
	xchat_hook_command (ph, "SOUND",      XCHAT_PRI_NORM, sound_cb,     NULL, (void *) 0);
	xchat_hook_command (ph, "ESOUND",     XCHAT_PRI_NORM, sound_cb,     NULL, (void *) 1);
	xchat_hook_command (ph, "NETDATA",    XCHAT_PRI_NORM, netdata_cb,   NULL, (void *) 0);
	xchat_hook_command (ph, "ENETDATA",   XCHAT_PRI_NORM, netdata_cb,   NULL, (void *) 1);
	xchat_hook_command (ph, "NETSTREAM",  XCHAT_PRI_NORM, netstream_cb, NULL, (void *) 0);
	xchat_hook_command (ph, "ENETSTREAM", XCHAT_PRI_NORM, netstream_cb, NULL, (void *) 1);
	xchat_hook_command (ph, "DISKINFO",   XCHAT_PRI_NORM, disk_cb,      NULL, (void *) 0);
	xchat_hook_command (ph, "EDISKINFO",  XCHAT_PRI_NORM, disk_cb,      NULL, (void *) 1);
	xchat_hook_command (ph, "MEMINFO",    XCHAT_PRI_NORM, mem_cb,       NULL, (void *) 0);
	xchat_hook_command (ph, "EMEMINFO",   XCHAT_PRI_NORM, mem_cb,       NULL, (void *) 1);
	xchat_hook_command (ph, "VIDEO",      XCHAT_PRI_NORM, video_cb,     NULL, (void *) 0);
	xchat_hook_command (ph, "EVIDEO",     XCHAT_PRI_NORM, video_cb,     NULL, (void *) 1);
	xchat_hook_command (ph, "ETHER",      XCHAT_PRI_NORM, ether_cb,     NULL, (void *) 0);
	xchat_hook_command (ph, "EETHER",     XCHAT_PRI_NORM, ether_cb,     NULL, (void *) 1);
	xchat_hook_command (ph, "DISTRO",     XCHAT_PRI_NORM, distro_cb,    NULL, (void *) 0);
	xchat_hook_command (ph, "EDISTRO",    XCHAT_PRI_NORM, distro_cb,    NULL, (void *) 1);

	/* this is required for the very first run */
	if (xchat_pluginpref_get_str (ph, "pciids", buffer) == 0)
	{
		xchat_pluginpref_set_str (ph, "pciids", DEFAULT_PCIIDS);
	}

	if (xchat_pluginpref_get_str (ph, "format", buffer) == 0)
	{
		xchat_pluginpref_set_str (ph, "format", DEFAULT_FORMAT);
	}

	if (xchat_pluginpref_get_int (ph, "percentages") == -1)
	{
		xchat_pluginpref_set_int (ph, "percentages", DEFAULT_PERCENTAGES);
	}

	xchat_printf (ph, "%s plugin loaded\n", name);
	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
