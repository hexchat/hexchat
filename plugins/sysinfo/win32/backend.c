/* HexChat
 * Copyright (c) 2011-2012 Berke Viktor.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <windows.h>
#include <wbemidl.h>

#include <glib.h>

#include "../../../src/common/sysinfo/sysinfo.h"

#include "../format.h"

static int command_callback (char *word[], char *word_eol[], void *user_data);

void print_info (void);

guint64 hdd_capacity;
guint64 hdd_free_space;
char *read_hdd_info (IWbemClassObject *object);
char *get_memory_info (void);

char *
sysinfo_backend_get_sound (void)
{
	return NULL;
}

char *
sysinfo_backend_get_network (void)
{
	return NULL;
}

char *
sysinfo_backend_get_uptime (void)
{
	return sysinfo_format_uptime (GetTickCount64 () / 1000);
}

char *
sysinfo_backend_get_disk (void)
{
	guint64 hdd_capacity;
	guint64 hdd_free_space;

	sysinfo_get_hdd_info (&hdd_capacity, &hdd_free_space);

	if (hdd_capacity != 0)
	{
		return sysinfo_format_disk(hdd_capacity, hdd_free_space);
	}

	return NULL;
}

char *
sysinfo_backend_get_cpu (void)
{
	return sysinfo_get_cpu ();
}

char *
sysinfo_backend_get_memory (void)
{
	/* Memory information is always loaded dynamically since it includes the current amount of free memory */
	return get_memory_info ();
}

char *
sysinfo_backend_get_gpu (void)
{
	return sysinfo_get_gpu ();
}

char *
sysinfo_backend_get_os (void)
{
	return sysinfo_get_os ();
}

static int get_cpu_arch (void)
{
	return sysinfo_get_cpu_arch ();
}

static char *get_memory_info (void)
{
	MEMORYSTATUSEX meminfo = { 0 };
	meminfo.dwLength = sizeof (meminfo);

	if (!GlobalMemoryStatusEx (&meminfo))
	{
		return NULL;
	}

	return sysinfo_format_memory (meminfo.ullTotalPhys, meminfo.ullAvailPhys);
}
