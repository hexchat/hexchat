/*
 * pci.c - PCI functions for X-Sys
 * Copyright (C) 1997-1999 Martin Mares <mj@atrey.karlin.mff.cuni.cz> [PCI routines from lspci]
 * Copyright (C) 2000 Tom Rini <trini@kernel.crashing.org> [XorgAutoConfig pci.c, based on lspci]
 * Copyright (C) 2005, 2006 Tony Vroon
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
#include <pci/pci.h>
#include <glib.h>

#include "sysinfo.h"

static struct pci_filter filter;       /* Device filter */
static struct pci_access *pacc;
int bus, dev, func; /* Location of the card */

struct device {
	struct device *next;
	struct pci_dev *dev;
	unsigned int config_cnt;
	u8 config[256];
};

static struct device *first_dev;

static struct device *scan_device(struct pci_dev *p)
{
	int how_much = 64;
	struct device *d;

	if (!pci_filter_match(&filter, p))
		return NULL;
	d = g_new0 (struct device, 1);
	d->dev = p;
	if (!pci_read_block(p, 0, d->config, how_much))
		exit(1);
	if (how_much < 128 && (d->config[PCI_HEADER_TYPE] & 0x7f) == PCI_HEADER_TYPE_CARDBUS)
	{
		/* For cardbus bridges, we need to fetch 64 bytes more to get the full standard header... */
		if (!pci_read_block(p, 64, d->config+64, 64))
			exit(1);
		how_much = 128;
	}
	d->config_cnt = how_much;
	pci_setup_cache(p, d->config, d->config_cnt);
	pci_fill_info(p, PCI_FILL_IDENT);
	return d;
}

static void scan_devices(void)
{
	struct device *d;
	struct pci_dev *p;

	pci_scan_bus(pacc);
	for(p=pacc->devices; p; p=p->next)
	{
		if ((d = scan_device(p)))
		{
			d->next = first_dev;
			first_dev = d;
		}
	}
}

static u16 get_conf_word(struct device *d, unsigned int pos)
{
	  return d->config[pos] | (d->config[pos+1] << 8);
}

int pci_find_by_class(u16 *class, char *vendor, char *device)
{
	struct device *d;
	struct pci_dev *p;
	int nomatch = 1;

	/* libpci has no way to report errors it calls exit()
	 * so we need to manually avoid potential failures like this one */
	if (!g_file_test ("/proc/bus/pci", G_FILE_TEST_EXISTS))
		return 1;

	pacc = pci_alloc();
	pci_filter_init(pacc, &filter);
	pci_init(pacc);
	scan_devices();

	for(d=first_dev; d; d=d->next)
	{
		p = d->dev;
		/* Acquire vendor & device ID if the class matches */
		if(get_conf_word(d, PCI_CLASS_DEVICE) == *class)
		{
			nomatch = 0;
			g_snprintf(vendor,7,"%04x",p->vendor_id);
			g_snprintf(device,7,"%04x",p->device_id);
			break;
		}
	  }

	  pci_cleanup(pacc);
	  return nomatch;
}

void pci_find_fullname(char *fullname, char *vendor, char *device)
{
	char buffer[bsize];
	char vendorname[bsize/2] = "";
	char devicename[bsize/2] = "";
	char *position;
	int cardfound = 0;
	FILE *fp;

	fp = fopen (PCIIDS_FILE, "r");
	if(fp == NULL)
	{
		g_snprintf(fullname, bsize, "%s:%s", vendor, device);
		//sysinfo_print_error ("pci.ids file not found! You might want to adjust your pciids setting with /SYSINFO SET pciids (you can query its current value with /SYSINFO LIST).\n");
		return;
	}

	while(fgets(buffer, bsize, fp) != NULL)
	{
		if (!isspace(buffer[0]) && strstr(buffer, vendor) != NULL)
		{
			position = strstr(buffer, vendor);
			position += 6;
			g_strlcpy(vendorname, position, sizeof (vendorname));
			position = strstr(vendorname, "\n");
			*(position) = '\0';
			break;
		}
	}
	while(fgets(buffer, bsize, fp) != NULL)
	{
		if(strstr(buffer, device) != NULL)
		{
			position = strstr(buffer, device);
			position += 6;
			g_strlcpy(devicename, position, sizeof (devicename));
			position = strstr(devicename, " (");
			if (position == NULL)
				position = strstr(devicename, "\n");
			*(position) = '\0';
			cardfound = 1;
			break;
		 }
	}
	if (cardfound == 1)
		g_snprintf(fullname, bsize, "%s %s", vendorname, devicename);
	else
		g_snprintf(fullname, bsize, "%s:%s", vendor, device);
	fclose(fp);
}
