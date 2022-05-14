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
#include <wchar.h>
#include <windows.h>
#include <wbemidl.h>

#include "../sysinfo.h"

/* Cache */
static int cpu_arch = 0;
static int build_arch = 0;
static char *os_name = NULL;
static char *cpu_info = NULL;
static char *vga_name = NULL;

typedef enum
{
	QUERY_WMI_OS,
	QUERY_WMI_CPU,
	QUERY_WMI_VGA,
	QUERY_WMI_HDD,
} QueryWmiType;

static char *query_wmi (QueryWmiType mode);
static char *read_os_name (IWbemClassObject *object);
static char *read_cpu_info (IWbemClassObject *object);
static char *read_vga_name (IWbemClassObject *object);

static guint64 hdd_capacity;
static guint64 hdd_free_space;
static char *read_hdd_info (IWbemClassObject *object);

static char *bstr_to_utf8 (BSTR bstr);
static guint64 variant_to_uint64 (VARIANT *variant);

char *
sysinfo_get_cpu (void)
{
	if (cpu_info == NULL)
		cpu_info = query_wmi (QUERY_WMI_CPU);

	return g_strdup (cpu_info);
}

char *
sysinfo_get_os (void)
{
	if (os_name == NULL)
		os_name = query_wmi (QUERY_WMI_OS);

	return g_strdup_printf ("%s (x%d)", os_name, sysinfo_get_cpu_arch ());
}

int
sysinfo_get_cpu_arch (void)
{
	SYSTEM_INFO si;

	if (cpu_arch != 0)
	{
		return cpu_arch;
	}

	GetNativeSystemInfo (&si);

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
	{
		return cpu_arch = 64;
	}
	else
	{
		return cpu_arch = 86;
	}
}

int
sysinfo_get_build_arch (void)
{
	SYSTEM_INFO si;

	if (build_arch != 0)
	{
		return build_arch;
	}

	GetSystemInfo (&si);

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
	{
		return build_arch = 64;
	}
	else
	{
		return build_arch = 86;
	}
}

char *
sysinfo_get_gpu (void)
{
	if (vga_name == NULL)
		vga_name = query_wmi (QUERY_WMI_VGA);

	return g_strdup (vga_name);
}

void
sysinfo_get_hdd_info (guint64 *hdd_capacity_out, guint64 *hdd_free_space_out)
{
	char *hdd_info;

	/* HDD information is always loaded dynamically since it includes the current amount of free space */
	*hdd_capacity_out = hdd_capacity = 0;
	*hdd_free_space_out = hdd_free_space = 0;

	hdd_info = query_wmi (QUERY_WMI_HDD);
	if (hdd_info == NULL)
	{
		return;
	}

	*hdd_capacity_out = hdd_capacity;
	*hdd_free_space_out = hdd_free_space;
}

/* https://msdn.microsoft.com/en-us/library/aa390423 */
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
	gboolean atleast_one_appended = FALSE;

	hr = CoCreateInstance (&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &locator);
	if (FAILED (hr))
	{
		goto exit;
	}

	namespaceName = SysAllocString (L"root\\CIMV2");

	hr = locator->lpVtbl->ConnectServer (locator, namespaceName, NULL, NULL, NULL, 0, NULL, NULL, &namespace);
	if (FAILED (hr))
	{
		goto release_locator;
	}

	hr = namespace->lpVtbl->QueryInterface (namespace, &IID_IUnknown, (void**)&namespaceUnknown);
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
		query = SysAllocString (L"SELECT Caption FROM Win32_OperatingSystem");
		break;
	case QUERY_WMI_CPU:
		query = SysAllocString (L"SELECT Name, MaxClockSpeed FROM Win32_Processor");
		break;
	case QUERY_WMI_VGA:
		query = SysAllocString (L"SELECT Name FROM Win32_VideoController");
		break;
	case QUERY_WMI_HDD:
		query = SysAllocString (L"SELECT Name, Capacity, FreeSpace FROM Win32_Volume");
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
		char *line;

		hr = enumerator->lpVtbl->Next (enumerator, WBEM_INFINITE, 1, &object, &numReturned);
		if (FAILED (hr) || numReturned == 0)
		{
			break;
		}

		switch (type)
		{
			case QUERY_WMI_OS:
				line = read_os_name (object);
				break;

			case QUERY_WMI_CPU:
				line = read_cpu_info (object);
				break;

			case QUERY_WMI_VGA:
				line = read_vga_name (object);
				break;

			case QUERY_WMI_HDD:
				line = read_hdd_info (object);
				break;

			default:
				break;
		}

		object->lpVtbl->Release (object);

		if (line != NULL)
		{
			if (atleast_one_appended)
			{
				g_string_append (result, ", ");
			}

			g_string_append (result, line);

			g_free (line);

			atleast_one_appended = TRUE;
		}
	}

	enumerator->lpVtbl->Release (enumerator);

release_query:
	SysFreeString (query);

release_queryLanguageName:
	SysFreeString (queryLanguageName);

release_namespaceUnknown:
	namespaceUnknown->lpVtbl->Release (namespaceUnknown);

release_namespace:
	namespace->lpVtbl->Release (namespace);

release_locator:
	locator->lpVtbl->Release (locator);
	SysFreeString (namespaceName);

exit:
	if (result == NULL)
	{
		return NULL;
	}

	return g_string_free (result, FALSE);
}

static char *read_os_name (IWbemClassObject *object)
{
	HRESULT hr;
	VARIANT caption_variant;
	char *caption_utf8;

	hr = object->lpVtbl->Get (object, L"Caption", 0, &caption_variant, NULL, NULL);
	if (FAILED (hr))
	{
		return NULL;
	}

	caption_utf8 = bstr_to_utf8 (caption_variant.bstrVal);

	VariantClear(&caption_variant);

	if (caption_utf8 == NULL)
	{
		return NULL;
	}

	g_strchomp (caption_utf8);

	return caption_utf8;
}

static char *read_cpu_info (IWbemClassObject *object)
{
	HRESULT hr;
	VARIANT name_variant;
	char *name_utf8;
	VARIANT max_clock_speed_variant;
	guint cpu_freq_mhz;
	char *result;

	hr = object->lpVtbl->Get (object, L"Name", 0, &name_variant, NULL, NULL);
	if (FAILED (hr))
	{
		return NULL;
	}

	name_utf8 = bstr_to_utf8 (name_variant.bstrVal);

	VariantClear (&name_variant);

	if (name_utf8 == NULL)
	{
		return NULL;
	}

	hr = object->lpVtbl->Get (object, L"MaxClockSpeed", 0, &max_clock_speed_variant, NULL, NULL);
	if (FAILED (hr))
	{
		g_free (name_utf8);

		return NULL;
	}

	cpu_freq_mhz = max_clock_speed_variant.uintVal;

	VariantClear (&max_clock_speed_variant);

	g_strchomp (name_utf8);

	if (cpu_freq_mhz > 1000)
	{
		result = g_strdup_printf ("%s (%.2fGHz)", name_utf8, cpu_freq_mhz / 1000.f);
	}
	else
	{
		result = g_strdup_printf ("%s (%" G_GUINT32_FORMAT "MHz)", name_utf8, cpu_freq_mhz);
	}

	g_free (name_utf8);

	return result;
}

static char *read_vga_name (IWbemClassObject *object)
{
	HRESULT hr;
	VARIANT name_variant;
	char *name_utf8;

	hr = object->lpVtbl->Get (object, L"Name", 0, &name_variant, NULL, NULL);
	if (FAILED (hr))
	{
		return NULL;
	}

	name_utf8 = bstr_to_utf8 (name_variant.bstrVal);

	VariantClear (&name_variant);

	if (name_utf8 == NULL)
	{
		return NULL;
	}

	return g_strchomp (name_utf8);
}

static char *read_hdd_info (IWbemClassObject *object)
{
	HRESULT hr;
	VARIANT name_variant;
	BSTR name_bstr;
	gsize name_len;
	VARIANT capacity_variant;
	guint64 capacity;
	VARIANT free_space_variant;
	guint64 free_space;

	hr = object->lpVtbl->Get (object, L"Name", 0, &name_variant, NULL, NULL);
	if (FAILED (hr))
	{
		return NULL;
	}

	name_bstr = name_variant.bstrVal;
	name_len = SysStringLen (name_variant.bstrVal);

	if (name_len >= 4 && name_bstr[0] == L'\\' && name_bstr[1] == L'\\' && name_bstr[2] == L'?' && name_bstr[3] == L'\\')
	{
		// This is not a named volume. Skip it.
		VariantClear (&name_variant);

		return NULL;
	}

	VariantClear (&name_variant);

	hr = object->lpVtbl->Get (object, L"Capacity", 0, &capacity_variant, NULL, NULL);
	if (FAILED (hr))
	{
		return NULL;
	}

	capacity = variant_to_uint64 (&capacity_variant);

	VariantClear (&capacity_variant);

	if (capacity == 0)
	{
		return NULL;
	}

	hr = object->lpVtbl->Get (object, L"FreeSpace", 0, &free_space_variant, NULL, NULL);
	if (FAILED (hr))
	{
		return NULL;
	}

	free_space = variant_to_uint64 (&free_space_variant);

	VariantClear (&free_space_variant);

	if (free_space == 0)
	{
		return NULL;
	}

	hdd_capacity += capacity;
	hdd_free_space += free_space;

	return NULL;
}

static char *bstr_to_utf8 (BSTR bstr)
{
	return g_utf16_to_utf8 (bstr, SysStringLen (bstr), NULL, NULL, NULL);
}

static guint64 variant_to_uint64 (VARIANT *variant)
{
	switch (V_VT (variant))
	{
	case VT_UI8:
		return variant->ullVal;

	case VT_BSTR:
		return wcstoull (variant->bstrVal, NULL, 10);

	default:
		return 0;
	}
}
