/*
 * parse.c - parsing functions for X-Sys
 * by mike9010
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
#include <ctype.h>
#ifdef HAVE_LIBPCI
	#include <pci/header.h>
	#include "pci.h"
#endif
#include <glib.h>

#ifdef __sparc__
#include <dirent.h>
#endif

#include "match.h"
#include "parse.h"
#include "sysinfo.h"

int xs_parse_cpu(char *model, char *vendor, double *freq)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__) || defined(__alpha__) || defined(__ia64__) || defined(__parisc__) || defined(__sparc__)
	char buffer[bsize];
#endif
	FILE *fp;

	fp = fopen("/proc/cpuinfo", "r");
	if(fp == NULL)
		return 1;

#if defined(__i386__) || defined(__x86_64__)

	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "model name", model);
		find_match_char(buffer, "vendor_id", vendor);
		find_match_double(buffer, "cpu MHz", freq);
	}

#elif defined(__powerpc__)
	{
		char *pos;

		while(fgets(buffer, bsize, fp) != NULL)
		{
			find_match_char(buffer, "cpu", model);
			find_match_char(buffer, "machine", vendor);
			find_match_double(buffer, "clock", freq);
		}
		pos = strstr(model, ",");
		if (pos != NULL)
		    *pos = '\0';
	}
#elif defined( __alpha__)

	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "cpu model", model);
		find_match_char(buffer, "system type", vendor);
		find_match_double(buffer, "cycle frequency [Hz]", freq);
	}
	*freq = *freq / 1000000;

#elif defined(__ia64__)

	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "model", model);
		find_match_char(buffer, "vendor", vendor);
		find_match_double(buffer, "cpu MHz", freq);
	}

#elif defined(__parisc__)

	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "cpu  ", model);
		find_match_char(buffer, "cpu family", vendor);
		find_match_double(buffer, "cpu MHz", freq);
	}

#elif defined(__sparc__)
    {
	    DIR *dir;
	    struct dirent *entry;
	    FILE *fp2;

	    while(fgets(buffer, bsize, fp) != NULL)
	    {
		    find_match_char(buffer, "cpu	", model);
		    find_match_char(buffer, "type	", vendor);
		    find_match_double_hex(buffer, "Cpu0ClkTck", freq);
	    }
	    *freq = *freq / 1000000;
    }
#else

    fclose(fp);
    return 1; /* Unsupported */

#endif

	fclose(fp);
	return 0;
}

gint64 xs_parse_uptime(void)
{
	char buffer[bsize];
	gint64 uptime = 0;
	FILE *fp = fopen("/proc/uptime", "r");
	if(fp == NULL)
		return 0;

	if(fgets(buffer, bsize, fp) != NULL)
		uptime = g_ascii_strtoll(buffer, NULL, 0);

	fclose(fp);

	return uptime;
}

int xs_parse_sound(char *snd_card)
{
#ifndef HAVE_LIBPCI
	return 1;
#else
	char buffer[bsize], cards[bsize] = "\0", vendor[7] = "\0", device[7] = "\0", *pos;
	u16 class = PCI_CLASS_MULTIMEDIA_AUDIO;

	FILE *fp = NULL;
	if((fp = fopen("/proc/asound/cards", "r"))== NULL)
	{
		if (pci_find_by_class(&class, vendor, device) == 0)
		{
			pci_find_fullname(snd_card, vendor, device);
			return 0;
		}
		else
			return 1;
	}


	while(fgets(buffer, bsize, fp) != NULL)
	{
		if(isdigit(buffer[0]) || isdigit(buffer[1]))
		{
			char card_buf[bsize];
			gint64 card_id = 0;
			pos = strstr(buffer, ":");
			card_id = g_ascii_strtoll(buffer, NULL, 0);
			if (card_id == 0)
				g_snprintf(card_buf, bsize, "%s", pos+2);
			else
				g_snprintf(card_buf, bsize, "%"G_GINT64_FORMAT": %s", card_id, pos+2);
			pos = strstr(card_buf, "\n");
			*pos = '\0';
			strcat(cards, card_buf);
		}
	}

	strcpy(snd_card, cards);

	fclose(fp);
	return 0;
#endif
}

int xs_parse_video(char *vid_card)
{
#ifndef HAVE_LIBPCI
	return 1;
#else
	char vendor[7] = "\0", device[7] = "\0";
	u16 class = PCI_CLASS_DISPLAY_VGA;
	if (pci_find_by_class(&class, vendor, device))
		return 1;
	else
		pci_find_fullname(vid_card, vendor, device);
	return 0;
#endif
}

int xs_parse_ether(char *ethernet_card)
{
#ifndef HAVE_LIBPCI
	return 1;
#else
	char vendor[7] = "\0", device[7] = "\0";
	u16 class = PCI_CLASS_NETWORK_ETHERNET;
	if (pci_find_by_class(&class, vendor, device))
		return 1;
	else
		pci_find_fullname(ethernet_card, vendor, device);
	return 0;
#endif
}

int xs_parse_agpbridge(char *agp_bridge)
{
#ifndef HAVE_LIBPCI
	return 1;
#else
	char vendor[7] = "\0", device[7] = "\0";
	u16 class = PCI_CLASS_BRIDGE_HOST;
	if (pci_find_by_class(&class, vendor, device))
		return 1;
	else
		pci_find_fullname(agp_bridge, vendor, device);
	return 0;
#endif
}

int xs_parse_meminfo(unsigned long long *mem_tot, unsigned long long *mem_free, int swap)
{
	FILE *fp;
	char buffer[bsize];
	unsigned long long freemem = 0, buffers = 0, cache = 0;
	*mem_tot = 0;
	*mem_free = 0;

	if((fp = fopen("/proc/meminfo", "r")) == NULL)
		return 1;

	while(fgets(buffer, bsize, fp) != NULL)
	{
		if(!swap)
		{
			find_match_ll(buffer, "MemTotal:", mem_tot);
			find_match_ll(buffer, "MemFree:", &freemem);
			find_match_ll(buffer, "Buffers:", &buffers);
			find_match_ll(buffer, "Cached:", &cache);
		}
		else
		{
			find_match_ll(buffer, "SwapTotal:", mem_tot);
			find_match_ll(buffer, "SwapFree:", mem_free);
		}
	}
	if (!swap)
	{
		*mem_free = freemem + buffers + cache;
	}
	fclose(fp);

	/* Convert to bytes */
	*mem_free *= 1000;
	*mem_tot *= 1000;
	return 0;
}

static void strip_quotes(char *string)
{
	size_t len = strlen(string);
	if (string[len - 1] == '"')
		string[--len] = '\0';

	if (string[0] == '"')
		memmove(string, string + 1, len);
}

int xs_parse_distro(char *name)
{
	FILE *fp = NULL;
	char buffer[bsize], *pos = NULL;

	if((fp = fopen("/etc/redhat-release", "r")) != NULL)
		fgets(buffer, bsize, fp);
	else if((fp = fopen("/etc/mageia-release", "r")) != NULL)
		fgets(buffer, bsize, fp);
	else if((fp = fopen("/etc/slackware-version", "r")) != NULL)
		fgets(buffer, bsize, fp);
	else if((fp = fopen("/etc/mandrake-release", "r")) != NULL)
		fgets(buffer, bsize, fp);
	else if((fp = fopen("/etc/SuSE-release", "r")) != NULL)
		fgets(buffer, bsize, fp);
	else if((fp = fopen("/etc/turbolinux-release", "r")) != NULL)
		fgets(buffer, bsize, fp);
	else if((fp = fopen("/etc/arch-release", "r")) != NULL)
		g_snprintf(buffer, bsize, "ArchLinux");
	else if((fp = fopen("/etc/lsb-release", "r")) != NULL)
	{
		char id[bsize], codename[bsize], release[bsize];
		strcpy(id, "?");
		strcpy(codename, "?");
		strcpy(release, "?");
		while(fgets(buffer, bsize, fp) != NULL)
		{
			find_match_char(buffer, "DISTRIB_ID", id);
			find_match_char(buffer, "DISTRIB_CODENAME", codename);
			find_match_char(buffer, "DISTRIB_RELEASE", release);
		}
		g_snprintf(buffer, bsize, "%s \"%s\" %s", id, codename, release);
	}
	else if((fp = fopen("/etc/debian_version", "r")) != NULL)
	{
		char release[bsize];
		fgets(release, bsize, fp);
		g_snprintf(buffer, bsize, "Debian %s", release);
	}
	else if((fp = fopen("/etc/portage/make.conf", "r")) != NULL ||
			(fp = fopen("/etc/make.conf", "r")) != NULL)
	{
		char keywords[bsize];
		while(fgets(buffer, bsize, fp) != NULL)
			find_match_char(buffer, "ACCEPT_KEYWORDS", keywords);
		/* cppcheck-suppress uninitvar */
		if (strstr(keywords, "\"") == NULL)
			g_snprintf(buffer, bsize, "Gentoo Linux (stable)");
		else
			g_snprintf(buffer, bsize, "Gentoo Linux %s", keywords);
	}
	else if((fp = fopen("/etc/os-release", "r")) != NULL)
	{
		char name[bsize], version[bsize];
		strcpy(name, "?");
		strcpy(version, "?");
		while(fgets(buffer, bsize, fp) != NULL)
		{
			find_match_char(buffer, "NAME=", name);
			find_match_char(buffer, "VERSION=", version);
		}
		strip_quotes(name);
		strip_quotes(version);
		g_snprintf(buffer, bsize, "%s %s", name, version);
	}
	else
		g_snprintf(buffer, bsize, "Unknown Distro");
	if(fp != NULL)
		fclose(fp);

	pos=strchr(buffer, '\n');
	if(pos != NULL)
		*pos = '\0';
	strcpy(name, buffer);
	return 0;
}
