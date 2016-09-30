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

 /*
	* Some snippets based upon Textual's System Profiler plugin.
	* https://github.com/Codeux-Software/Textual
	*/

#import <Cocoa/Cocoa.h>

#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <mach/mach_vm.h>

#include <glib.h>

#include "format.h"
#include "df.h"

static char *
get_os (void)
{
	NSDictionary *systemversion = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
	NSString *build = [systemversion objectForKey:@"ProductBuildVersion"];
	if (!build)
		return NULL;
	NSString *version = [systemversion objectForKey:@"ProductUserVisibleVersion"];
	if (!version)
	{
		[build release];
		return NULL;
	}

	NSDictionary *profiler = [NSDictionary dictionaryWithContentsOfFile:[@"~/Library/Preferences/com.apple.SystemProfiler.plist" stringByExpandingTildeInPath]];
	NSDictionary *names = [profiler objectForKey:@"OS Names"];
	NSString *os_name = nil;

	for (NSString *name in names)
	{
		if ([name hasPrefix:build])
		{
			os_name = [names objectForKey:name];
			break;
		}
	}
	[build release];

	if (!os_name)
	{
		[version release];
		return NULL;
	}

	char *ret = g_strdup_printf ("%s %s", [os_name UTF8String], [version UTF8String]);
	[version release];

	return ret;
}

static char *
get_os_fallback (void)
{
#if !defined (MAC_OS_X_VERSION_10_9) || MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_9
	SInt32 ver_major = 0,
	       ver_minor = 0,
	       ver_patch = 0;

	Gestalt (gestaltSystemVersionMajor, &ver_major);
	Gestalt (gestaltSystemVersionMinor, &ver_minor);
	Gestalt (gestaltSystemVersionBugFix, &ver_patch);

	return g_strdup_printf ("OS X %d.%d.%d", ver_major, ver_minor, ver_patch);
#else
	NSProcessInfo *info = [NSProcessInfo processInfo];
	NSOperatingSystemVersion version = [info operatingSystemVersion];

	return g_strdup_printf ("OS X %ld.%ld.%ld", version.majorVersion, version.minorVersion, version.patchVersion);
#endif
}
char *
sysinfo_backend_get_os(void)
{
	static char *os_str = NULL;
	if (!os_str)
	{
		os_str = get_os();
		if (!os_str)
			os_str = get_os_fallback();
	}
	return g_strdup (os_str);
}

char *
sysinfo_backend_get_disk(void)
{
	gint64 total, free_space;

	if (xs_parse_df (&total, &free_space))
	{
		return NULL;
	}

	return sysinfo_format_disk (total, free_space);
}

static guint64
get_free_memory (void)
{
	mach_msg_type_number_t infoCount = (sizeof(vm_statistics_data_t) / sizeof(natural_t));

	vm_size_t pagesize;
	vm_statistics_data_t vm_stat;

	host_page_size(mach_host_self(), &pagesize);

	if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vm_stat, &infoCount) == KERN_SUCCESS)
		return ((vm_stat.inactive_count + vm_stat.free_count) * pagesize);

	return 0;
}

char *
sysinfo_backend_get_memory(void)
{
	NSProcessInfo *info = [NSProcessInfo processInfo];
	guint64 totalmem, freemem;

	totalmem = [info physicalMemory];

	if ((freemem = get_free_memory()) == 0)
		return NULL;

	return sysinfo_format_memory (totalmem, freemem);
}

char *
sysinfo_backend_get_cpu(void)
{
	guint64 cpu_clock_uint = 0;
	double cpu_clock;
	char cpu_string[256];
	gsize len;
	gboolean giga = FALSE;

	len = sizeof(cpu_string);
	if (sysctlbyname ("machdep.cpu.brand_string", cpu_string, &len, NULL, 0) != 0)
		return NULL;
	cpu_string[sizeof(cpu_string) - 1] = '\0';

	len = sizeof(cpu_clock_uint);
	if (sysctlbyname("hw.cpufrequency", &cpu_clock_uint, &len, NULL, 0) < 0)
		return NULL;

	cpu_clock = cpu_clock_uint / 1000000;
	if (cpu_clock > 1000)
	{
		cpu_clock /= 1000;
		giga = TRUE;
	}

	if (giga)
		return g_strdup_printf ("%s (%.2fGHz)", cpu_string, cpu_clock);
	else
		return g_strdup_printf ("%s (%.0fMHz)", cpu_string, cpu_clock);
}

static char *
get_gpu(void)
{
	CFMutableDictionaryRef pciDevices = IOServiceMatching("IOPCIDevice");
	io_iterator_t entry_iterator, serviceObject;

	if (IOServiceGetMatchingServices(kIOMasterPortDefault, pciDevices, &entry_iterator) != kIOReturnSuccess)
		return NULL;

	GString *gpu_list = g_string_new(NULL);
	while ((serviceObject = IOIteratorNext(entry_iterator)))
	{
		CFMutableDictionaryRef serviceDictionary;

		kern_return_t status = IORegistryEntryCreateCFProperties(serviceObject, &serviceDictionary,
														 kCFAllocatorDefault, kNilOptions);

		if (status != kIOReturnSuccess)
		{
			IOObjectRelease(serviceObject);
			continue;
		}

		const void *class = CFDictionaryGetValue(serviceDictionary, @"class-code");
		if (!class || *(guint32*)CFDataGetBytePtr(class) != 0x30000) /* DISPLAY_VGA */
		{
			CFRelease(serviceDictionary);
			continue;
		}

		const void *model = CFDictionaryGetValue(serviceDictionary, @"model");
		if (model)
		{
			if (CFGetTypeID(model) == CFDataGetTypeID() && CFDataGetLength(model) > 1)
			{
				if (gpu_list->len != 0)
						g_string_append (gpu_list, ", ");
				g_string_append_len (gpu_list, (const char*)CFDataGetBytePtr(model), CFDataGetLength(model) - 1);
			}
		}

		CFRelease(serviceDictionary);
	}

	if (gpu_list->len == 0)
	{
		g_string_free (gpu_list, TRUE);
		return NULL;
	}

	/* The string may contain nul-chars we must replace */
	int i;
	for (i = 0; i < gpu_list->len; i++)
	{
		if (gpu_list->str[i] == '\0')
			gpu_list->str[i] = ' ';
	}

	return g_string_free (gpu_list, FALSE);
}

char *
sysinfo_backend_get_gpu(void)
{
	static char *gpu_str = NULL;
	if (!gpu_str)
		gpu_str = get_gpu();

	return g_strdup (gpu_str);
}

char *
sysinfo_backend_get_sound(void)
{
	return NULL;
}

char *
sysinfo_backend_get_uptime(void)
{
	NSProcessInfo *info = [NSProcessInfo processInfo];
	double uptime = [info systemUptime];

	return sysinfo_format_uptime ((gint64)uptime);
}

char *
sysinfo_backend_get_network(void)
{
	return NULL;
}
