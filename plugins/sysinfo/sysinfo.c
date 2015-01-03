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

#include "hexchat-plugin.h"

static hexchat_plugin *ph;   /* plugin handle */

static char name[] = "SysInfo";
static char desc[] = "Display info about your hardware and OS";
static char version[] = "1.1";
static char helptext[] = "USAGE: /SYSINFO - Sends info about your hardware and OS to current channel.";

/* Cache the info for subsequent invocations of /SYSINFO */
static int cpu_arch = 0;
static char *os_name = NULL;
static char *cpu_info = NULL;
static char *vga_name = NULL;

static int printInfo (char *word[], char *word_eol[], void *user_data);

typedef enum
{
	QUERY_WMI_OS,
	QUERY_WMI_CPU,
	QUERY_WMI_VGA,
} QueryWmiType;

static int get_cpu_arch (void);
char *query_wmi (QueryWmiType mode);
char *get_memory_info (void);

int hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	hexchat_hook_command (ph, "SYSINFO", HEXCHAT_PRI_NORM, printInfo, helptext, NULL);
	hexchat_command (ph, "MENU -ishare\\system.png ADD \"Window/Send System Info\" \"SYSINFO\"");

	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;
}

int hexchat_plugin_deinit (void)
{
	g_free (os_name);
	g_free (cpu_info);
	g_free (vga_name);

	hexchat_command (ph, "MENU DEL \"Window/Display System Info\"");
	hexchat_printf (ph, "%s plugin unloaded\n", name);

	return 1;
}

static int printInfo (char *word[], char *word_eol[], void *user_data)
{
	char *memory_info;
	int channel_type;

	/* Load information if not already loaded */

	if (cpu_arch == 0)
	{
		cpu_arch = get_cpu_arch ();
	}

	if (os_name == NULL)
	{
		hexchat_printf (ph, "%s - Querying and caching OS info...\n", name);

		os_name = query_wmi (QUERY_WMI_OS);
		if (os_name == NULL)
		{
			hexchat_printf (ph, "%s - Error while getting OS info.\n", name);
			os_name = g_strdup ("Unknown");
		}
	}

	if (cpu_info == NULL)
	{
		hexchat_printf (ph, "%s - Querying and caching CPU info...\n", name);

		cpu_info = query_wmi (QUERY_WMI_CPU);
		if (cpu_info == NULL)
		{
			hexchat_printf (ph, "%s - Error while getting CPU info.\n", name);
			cpu_info = g_strdup ("Unknown");
		}
	}

	if (vga_name == NULL)
	{
		hexchat_printf (ph, "%s - Querying and caching VGA info...\n", name);

		vga_name = query_wmi (QUERY_WMI_VGA);
		if (vga_name == NULL)
		{
			hexchat_printf (ph, "%s - Error while getting VGA info.\n", name);
			vga_name = g_strdup ("Unknown");
		}
	}

	/* Memory information is always loaded dynamically since it includes the current amount of free memory */
	memory_info = get_memory_info ();
	if (memory_info == NULL)
	{
		hexchat_printf (ph, "%s - Error while getting memory info.\n", name);
		memory_info = g_strdup ("Unknown");
	}

	channel_type = hexchat_list_int (ph, NULL, "type");
	if (channel_type == 2 /* SESS_CHANNEL */ || channel_type == 3 /* SESS_DIALOG */)
	{
		hexchat_commandf (
			ph,
			"ME ** SysInfo ** Client: HexChat %s (x%d) ** OS: %s ** CPU: %s ** RAM: %s ** VGA: %s ** Uptime: %.2f Hours **",
			hexchat_get_info (ph, "version"),
			cpu_arch,
			os_name,
			cpu_info,
			memory_info,
			vga_name,
			(float) GetTickCount64 () / 1000 / 60 / 60);
	}
	else
	{
		hexchat_printf (ph, " * Client:  HexChat %s (x%d)\n", hexchat_get_info (ph, "version"), cpu_arch);
		hexchat_printf (ph, " * OS:      %s\n", os_name);
		hexchat_printf (ph, " * CPU:     %s\n", cpu_info);
		hexchat_printf (ph, " * RAM:     %s\n", memory_info);
		hexchat_printf (ph, " * VGA:     %s\n", vga_name);
		hexchat_printf (ph, " * Uptime:  %.2f Hours\n", (float) GetTickCount64 () / 1000 / 60 / 60);
	}

	g_free (memory_info);

	return HEXCHAT_EAT_HEXCHAT;
}

/* http://msdn.microsoft.com/en-us/site/aa390423 */
static char *query_wmi (QueryWmiType type)
{
	GString *result = NULL;
	HRESULT hr;

	IWbemLocator *locator = NULL;
	BSTR namespaceName = NULL;
	BSTR queryLanguageName = NULL;
	BSTR query = NULL;
	IWbemServices *namespace = NULL;
	IUnknown *namespaceUnknown = NULL;
	IEnumWbemClassObject *enumerator = NULL;
	int i;

	hr = CoInitializeEx (0, COINIT_APARTMENTTHREADED);
	if (FAILED (hr))
	{
		goto exit;
	}

	/* If this is called after some other call to CoCreateInstance somewhere else in the process, this will fail with RPC_E_TOO_LATE.
	 * However if not, it *is* required to be called, so call it here but ignore any error returned.
	 */
	CoInitializeSecurity (NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	hr = CoCreateInstance (&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &locator);
	if (FAILED (hr))
	{
		goto couninitialize;
	}

	namespaceName = SysAllocString (L"root\\CIMV2");

	hr = locator->lpVtbl->ConnectServer (locator, namespaceName, NULL, NULL, NULL, 0, NULL, NULL, &namespace);
	if (FAILED (hr))
	{
		goto release_locator;
	}

	hr = namespace->lpVtbl->QueryInterface (namespace, &IID_IUnknown, &namespaceUnknown);
	if (FAILED (hr))
	{
		goto release_namespace;
	}

	hr = CoSetProxyBlanket (namespaceUnknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED (hr))
	{
		goto release_namespaceUnknown;
	}

	queryLanguageName = SysAllocString (L"WQL");

	switch (type)
	{
	case QUERY_WMI_OS:
		query = SysAllocString (L"SELECT * FROM Win32_OperatingSystem");
		break;
	case QUERY_WMI_CPU:
		query = SysAllocString (L"SELECT * FROM Win32_Processor");
		break;
	case QUERY_WMI_VGA:
		query = SysAllocString (L"SELECT * FROM Win32_VideoController");
		break;
	default:
		goto release_queryLanguageName;
	}

	hr = namespace->lpVtbl->ExecQuery (namespace, queryLanguageName, query, WBEM_FLAG_FORWARD_ONLY, NULL, &enumerator);
	if (FAILED (hr))
	{
		goto release_query;
	}

	result = g_string_new ("");

	for (i = 0;; i++)
	{
		ULONG numReturned = 0;
		IWbemClassObject *object;
		VARIANT property;
		char *property_utf8;

		hr = enumerator->lpVtbl->Next (enumerator, WBEM_INFINITE, 1, &object, &numReturned);
		if (FAILED (hr) || numReturned == 0)
		{
			break;
		}

		switch (type)
		{
			case QUERY_WMI_OS:
				hr = object->lpVtbl->Get (object, L"Caption", 0, &property, 0, 0);
				break;

			case QUERY_WMI_CPU:
				hr = object->lpVtbl->Get (object, L"Name", 0, &property, 0, 0);
				break;

			case QUERY_WMI_VGA:
				hr = object->lpVtbl->Get (object, L"Name", 0, &property, 0, 0);
				break;

			default:
				break;
		}

		if (FAILED (hr))
		{
			break;
		}

		if (i > 0)
		{
			g_string_append (result, ", ");
		}

		property_utf8 = g_utf16_to_utf8 (property.bstrVal, SysStringLen (property.bstrVal), NULL, NULL, NULL);
		g_string_append (result, property_utf8);

		VariantClear (&property);

		if (type == QUERY_WMI_CPU)
		{
			guint cpu_freq_mhz;

			hr = object->lpVtbl->Get (object, L"MaxClockSpeed", 0, &property, 0, 0);
			if (FAILED (hr))
			{
				break;
			}

			cpu_freq_mhz = property.uintVal;

			if (cpu_freq_mhz > 1000)
			{
				g_string_append_printf (result, " (%.2f GHz)", property.uintVal / 1000.0f);
			}
			else
			{
				g_string_append_printf (result, " (%" G_GUINT32_FORMAT " MHz)", property.uintVal);
			}
		}

		object->lpVtbl->Release (object);
	}

	enumerator->lpVtbl->Release (enumerator);

release_query:
	SysFreeString(query);

release_queryLanguageName:
	SysFreeString(queryLanguageName);

release_namespaceUnknown:
	namespaceUnknown->lpVtbl->Release (namespaceUnknown);

release_namespace:
	namespace->lpVtbl->Release (namespace);

release_locator:
	locator->lpVtbl->Release(locator);
	SysFreeString (namespaceName);

couninitialize:
	CoUninitialize ();

exit:
	if (result == NULL)
	{
		return NULL;
	}

	return g_string_free (result, FALSE);
}

static int get_cpu_arch (void)
{
	SYSTEM_INFO si;

	GetNativeSystemInfo (&si);

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
	{
		return 64;
	}
	else
	{
		return 86;
	}
}

static char *get_memory_info (void)
{
	MEMORYSTATUSEX meminfo = { 0 };
	meminfo.dwLength = sizeof (meminfo);

	if (!GlobalMemoryStatusEx (&meminfo))
	{
		return NULL;
	}

	return g_strdup_printf ("%" G_GUINT64_FORMAT " MB Total (%" G_GUINT64_FORMAT " MB Free)", meminfo.ullTotalPhys / 1024 / 1024, meminfo.ullAvailPhys / 1024 / 1024);
}
