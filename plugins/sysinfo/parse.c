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
#include <sys/utsname.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <pci/header.h>

#include "pci.h"
#include "match.h"
#include "xsys.h"
#include "parse.h"

int xs_parse_cpu(char *model, char *vendor, double *freq, char *cache, unsigned int *count)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__) || defined(__alpha__) || defined(__ia64__) || defined(__parisc__) || defined(__sparc__)
	char buffer[bsize];
#endif
#if defined(__powerpc__)
	char *pos = NULL;
#endif
	FILE *fp = fopen("/proc/cpuinfo", "r");
	if(fp == NULL)
		return 1;

	*count = 0;
	strcpy(cache,"unknown\0");
	
	#if defined(__i386__) || defined(__x86_64__)
	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "model name", model);
		find_match_char(buffer, "vendor_id", vendor);
		find_match_double(buffer, "cpu MHz", freq);
		find_match_char(buffer, "cache size", cache);
		find_match_int(buffer, "processor", count);
	}
	*count = *count + 1;
	#endif
	#ifdef __powerpc__
	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "cpu", model);
		find_match_char(buffer, "machine", vendor);
		find_match_double(buffer, "clock", freq);
		find_match_char(buffer, "L2 cache", cache);
		find_match_int(buffer, "processor", count);
	}
	*count = *count + 1;
	pos = strstr(model, ",");
	if (pos != NULL) *pos = '\0';
	#endif
	#ifdef __alpha__
	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "cpu model", model);
		find_match_char(buffer, "system type", vendor);
		find_match_double(buffer, "cycle frequency [Hz]", freq);
		find_match_char(buffer, "L2 cache", cache);
		find_match_int(buffer, "cpus detected", count);
	}
	*freq = *freq / 1000000;
	#endif
	#ifdef __ia64__
	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "model", model);
		find_match_char(buffer, "vendor", vendor);
		find_match_double(buffer, "cpu MHz", freq);
		find_match_int(buffer, "processor", count);
	}
	*count = *count + 1;
	#endif
	#ifdef __parisc__
	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "cpu  ", model);
		find_match_char(buffer, "cpu family", vendor);
		find_match_char(buffer, "D-cache", cache);
		find_match_double(buffer, "cpu MHz", freq);
		find_match_int(buffer, "processor", count);
	}
	*count = *count + 1;
	#endif
	#ifdef __sparc__
	DIR *dir;
	struct dirent *entry;
	FILE *fp2;
	while(fgets(buffer, bsize, fp) != NULL)
	{
		find_match_char(buffer, "cpu	", model);
		find_match_char(buffer, "type	", vendor);
		find_match_int(buffer, "ncpus active", count);
		find_match_double_hex(buffer, "Cpu0ClkTck", freq);
	}
	/* Cache is tricky, only implemented for sparc64 */
	if ((dir = opendir("/proc/openprom")) != NULL)
		while ((entry = readdir(dir)) != NULL)
			if (strncmp(entry->d_name,"SUNW,UltraSPARC",15) == 0)
			{
				snprintf(buffer,bsize,"/proc/openprom/%s/ecache-size",entry->d_name);
				fp2 = fopen(buffer, "r");
				if (fp2 == NULL) break;
				fscanf(fp2,"%16s",cache);
				fclose(fp2);
				sprintf(buffer,"0x%s",cache);
				sprintf(cache,"%0.0f KB",strtod(buffer,NULL)/1024);
			}
	*freq = *freq / 1000000;
	#endif
	fclose(fp);
	
	return 0;
}

int xs_parse_uptime(int *weeks, int *days, int *hours, int *minutes, int *seconds)
{
	char buffer[bsize];
	long long uptime = 0;
	FILE *fp = fopen("/proc/uptime", "r");
	if(fp == NULL)
		return 1;

	if(fgets(buffer, bsize, fp) != NULL)
		uptime = strtol(buffer, NULL, 0);
	
	*seconds = uptime%60;
	*minutes = (uptime/60)%60;
	*hours   = (uptime/3600)%24;
	*days    = (uptime/86400)%7;
	*weeks   = uptime/604800;
	
	fclose(fp);
	
	return 0;
}

int xs_parse_os(char *user, char *host, char *kernel)
{
	struct utsname osinfo;
	char hostn[bsize], *usern = getenv("USER");
	
	if(uname(&osinfo)<0)
		return 1;	
	if(gethostname(hostn, bsize)<0)
		return 1;
	
	strncpy(user, usern, bsize);
	strcpy(host, hostn);
	snprintf(kernel, bsize, "%s %s %s", osinfo.sysname, osinfo.release, osinfo.machine);
	
	return 0;
}

int xs_parse_sound(char *snd_card)
{
	char buffer[bsize], cards[bsize] = "\0", vendor[7] = "\0", device[7] = "\0", *pos;
	u16 class = PCI_CLASS_MULTIMEDIA_AUDIO;

	FILE *fp = NULL;
	if((fp = fopen("/proc/asound/cards", "r"))== NULL) {
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
			long card_id = 0;
			pos = strstr(buffer, ":");
			card_id = strtoll(buffer, NULL, 0);
			if (card_id == 0)
				snprintf(card_buf, bsize, "%s", pos+2);
			else
				snprintf(card_buf, bsize, "%ld: %s", card_id, pos+2);
			pos = strstr(card_buf, "\n");
			*pos = '\0';
			strcat(cards, card_buf);
		}
	}

	strcpy(snd_card, cards);
	
	fclose(fp);
	return 0;
}

int xs_parse_video(char *vid_card)
{
	char vendor[7] = "\0", device[7] = "\0";
	u16 class = PCI_CLASS_DISPLAY_VGA;
	if (pci_find_by_class(&class, vendor, device))
		return 1;
	else
		pci_find_fullname(vid_card, vendor, device);
	return 0;
}

int xs_parse_ether(char *ethernet_card)
{
	char vendor[7] = "\0", device[7] = "\0";
	u16 class = PCI_CLASS_NETWORK_ETHERNET;
	if (pci_find_by_class(&class, vendor, device))
		return 1;
	else
		pci_find_fullname(ethernet_card, vendor, device);
	return 0;
}

int xs_parse_agpbridge(char *agp_bridge)
{
	char vendor[7] = "\0", device[7] = "\0";
	u16 class = PCI_CLASS_BRIDGE_HOST;
	if (pci_find_by_class(&class, vendor, device))
		return 1;
	else
		pci_find_fullname(agp_bridge, vendor, device);
	return 0;
}

int xs_parse_netdev(const char *device, unsigned long long *bytes_recv, unsigned long long *bytes_sent)
{
	FILE *fp;
	char buffer[bsize], *pos;
	int i;
	
	fp=fopen("/proc/net/dev", "r");
	if(fp==NULL)
		return 1;
	
	while(fgets(buffer, bsize, fp) != NULL)
	{
		for(i=0; isspace(buffer[i]); i++);
		if(strncmp(device, &buffer[i], strlen(device)) == 0) break;
	}
	fclose(fp);
	pos = strstr(buffer, ":");
	pos++;
	*bytes_recv = strtoull(pos, &pos, 0);

	for(i=0;i<7;i++) strtoull(pos, &pos, 0);
	
	*bytes_sent = strtoull(pos, NULL, 0);

	return 0;
}

int xs_parse_df(const char *mount_point, char *result)
{
	FILE *pipe;
	char buffer[bsize], *pos;
	unsigned long long total_k=0, free_k=0;
	int i=0;
	
	pipe = popen("df -k -l -P", "r");
	if(pipe==NULL)
		return 1;
	
	while(fgets(buffer, bsize, pipe) != NULL)
	{
		/* Skip over pseudo-filesystems and description line */
		if(isalpha(buffer[0]))
			continue;
		
		for(pos=buffer; !isspace(*pos); pos++);
		for(;isspace(*pos);pos++);
		if(mount_point == NULL)
		{
			total_k += strtoull(pos, &pos, 0);
			strtoull(pos, &pos, 0);
			free_k += strtoull(pos, &pos, 0);
			continue;
		}
		total_k = strtoull(pos, &pos, 0);
		strtoull(pos, &pos, 0);
		free_k = strtoull(pos, &pos, 0);
		strtoull(pos, &pos, 0);
		for(;isspace(*pos);pos++);
		for(;*pos!='/';pos++);
		for(i=0;*(buffer+i)!='\n';i++);
		*(buffer+i)='\0';
		
		if(strncasecmp(mount_point, "ALL", 3)==0)
		{
			char *tmp_buf = pretty_freespace(pos, &free_k, &total_k);
			strcat(tmp_buf, " | ");
			strcat(result, tmp_buf);
			free(tmp_buf);
		}
		else if(strncmp(mount_point, pos, strlen(mount_point)) == 0)
		{
			char *tmp_buf = pretty_freespace(mount_point, &free_k, &total_k);
			strncpy(result, tmp_buf, bsize);
			free(tmp_buf);
			break;
		}
		else snprintf(result, bsize, "Mount point %s not found!", mount_point);
	}
	
	if(mount_point != NULL && strncasecmp(mount_point, "ALL", 3)==0)
		*(result+strlen(result)-3) = '\0';
	
	if(mount_point == NULL)
	{
		char *tmp_buf = pretty_freespace("Total", &free_k, &total_k);
		strncpy(result, tmp_buf, bsize);
		free(tmp_buf);
	}
	pclose(pipe);
	return 0;
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
	if (!swap) {
		*mem_free = freemem + buffers + cache;
	}
        fclose(fp);
        return 0;
}

int xs_parse_distro(char *name)
{
	FILE *fp = NULL;
	char buffer[bsize], *pos = NULL;

	if((fp = fopen("/etc/portage/make.conf", "r")) != NULL ||
			(fp = fopen("/etc/make.conf", "r")) != NULL)
	{
		char keywords[bsize];
		while(fgets(buffer, bsize, fp) != NULL)
			find_match_char(buffer, "ACCEPT_KEYWORDS", keywords);
		/* cppcheck-suppress uninitvar */
		if (strstr(keywords, "\"") == NULL)
			snprintf(buffer, bsize, "Gentoo Linux (stable)");
		else
			snprintf(buffer, bsize, "Gentoo Linux %s", keywords);
	}
	else if((fp = fopen("/etc/redhat-release", "r")) != NULL)
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
		snprintf(buffer, bsize, "ArchLinux");
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
		snprintf(buffer, bsize, "%s \"%s\" %s", id, codename, release);
	}
	else if((fp = fopen("/etc/debian_version", "r")) != NULL)
	{
		char release[bsize];
		fgets(release, bsize, fp);
		snprintf(buffer, bsize, "Debian %s", release);
	}
	else
		snprintf(buffer, bsize, "Unknown Distro");
	if(fp != NULL) fclose(fp);
	
	pos=strchr(buffer, '\n');
	if(pos != NULL) *pos = '\0';
	strcpy(name, buffer);
	return 0;
}
