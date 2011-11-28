/* XChat-WDK
 * Copyright (c) 2011 Berke Viktor.
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
#include <comutil.h>
#include <wbemidl.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */

static char *
getOsName (void)
{
	static char winver[32];
	double mhz;
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;

	osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
	GetVersionEx ((LPOSVERSIONINFOW) &osvi);

	GetSystemInfo (&si);

	strcpy (winver, "Windows ");

	switch (osvi.dwMajorVersion)
	{
		case 5:
			switch (osvi.dwMinorVersion)
			{
				case 1:
					strcat (winver, "XP");
					break;
				case 2:
					if (osvi.wProductType == VER_NT_WORKSTATION)
					{
						strcat (winver, "XP x64 Edition");
					}
					else
					{
						if (GetSystemMetrics(SM_SERVERR2) == 0)
						{
							strcat (winver, "Server 2003");
						}
						else
						{
							strcat (winver, "Server 2003 R2");
						}
					}
					break;
			}
			break;
		case 6:
			switch (osvi.dwMinorVersion)
			{
				case 0:
					if (osvi.wProductType == VER_NT_WORKSTATION)
					{
						strcat (winver, "Vista");
					}
					else
					{
						strcat (winver, "Server 2008");
					}
					break;
				case 1:
					if (osvi.wProductType == VER_NT_WORKSTATION)
					{
						strcat (winver, "7");
					}
					else
					{
						strcat (winver, "Server 2008 R2");
					}
					break;
				case 2:
					if (osvi.wProductType == VER_NT_WORKSTATION)
					{
						strcat (winver, "8");
					}
					else
					{
						strcat (winver, "8 Server");
					}
					break;
			}
			break;
	}

	if (si.wProcessorArchitecture == 9)
	{
		strcat (winver, " (x64)");
	}
	else
	{
		strcat (winver, " (x86)");
	}

	return winver;
}

#if 0 /* x86-only, SDK-only, use WMI instead */
static char *
getCpuName (void)
{
	// Get extended ids.
	unsigned int nExIds;
	unsigned int i;
	int CPUInfo[4] = {-1};
	static char CPUBrandString[128];

	__cpuid (CPUInfo, 0x80000000);
	nExIds = CPUInfo[0];

	/* Get the information associated with each extended ID. */
	for (i=0x80000000; i <= nExIds; ++i)
	{
		__cpuid (CPUInfo, i);

		if (i == 0x80000002)
		{
			memcpy (CPUBrandString, CPUInfo, sizeof (CPUInfo));
		}
		else if (i == 0x80000003)
		{
			memcpy( CPUBrandString + 16, CPUInfo, sizeof (CPUInfo));
		}
		else if (i == 0x80000004)
		{
			memcpy (CPUBrandString + 32, CPUInfo, sizeof (CPUInfo));
		}
	}

	return CPUBrandString;
}
#endif

static char *
getCpuMhz (void)
{
	HKEY hKey;
	int result;
	int data;
	int dataSize;
	double cpuspeed;
	static char buffer[16];
	const char *cpuspeedstr;

	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Hardware\\Description\\System\\CentralProcessor\\0"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dataSize = sizeof (data);
		result = RegQueryValueEx (hKey, TEXT("~MHz"), 0, 0, (LPBYTE)&data, (LPDWORD)&dataSize);
		RegCloseKey (hKey);
		if (result == ERROR_SUCCESS)
		{
			cpuspeed = ( data > 1000 ) ? data / 1000 : data;
			cpuspeedstr = ( data > 1000 ) ? "GHz" : "MHz";
			sprintf (buffer, "%.2f %s", cpuspeed, cpuspeedstr);
		}
	}

	return buffer;
}

static char *
getMemoryInfo (void)
{
	static char buffer[32];
	MEMORYSTATUSEX meminfo;

	meminfo.dwLength = sizeof (meminfo);
	GlobalMemoryStatusEx (&meminfo);

	sprintf (buffer, "%I64d MB Total (%I64d MB Free)", meminfo.ullTotalPhys / 1024 / 1024, meminfo.ullAvailPhys / 1024 / 1024);

	return buffer;
}

static char *
getWmiInfo (int mode)
{
	/* for more details about this wonderful API, see 
	http://msdn.microsoft.com/en-us/site/aa394138
	http://msdn.microsoft.com/en-us/site/aa390423
	http://msdn.microsoft.com/en-us/library/windows/desktop/aa394138%28v=vs.85%29.aspx
	http://social.msdn.microsoft.com/forums/en-US/vcgeneral/thread/d6420012-e432-4964-8506-6f6b65e5a451
	*/

	static char buffer[128];
	HRESULT hres;
	HRESULT hr;
	IWbemLocator *pLoc = NULL;
	IWbemServices *pSvc = NULL;
	IEnumWbemClassObject *pEnumerator = NULL;
	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;

	hres =  CoInitializeEx (0, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);

	if (FAILED (hres))
	{
		strcpy (buffer, "Error Code 0");
		return buffer;
	}

	hres =  CoInitializeSecurity (NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	/* mysteriously failing after the first execution, but only when used as a plugin, skip it */
	/*if (FAILED (hres))
	{
		CoUninitialize ();
		strcpy (buffer, "Error Code 1");
		return buffer;
	}*/

	hres = CoCreateInstance (CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);

	if (FAILED (hres))
	{
		CoUninitialize ();
		strcpy (buffer, "Error Code 2");
		return buffer;
	}

	hres = pLoc->ConnectServer (_bstr_t (L"root\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

	if (FAILED (hres))
	{
		pLoc->Release ();
		CoUninitialize ();
		strcpy (buffer, "Error Code 3");
		return buffer;
	}

	hres = CoSetProxyBlanket (pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

	if (FAILED (hres))
	{
		pSvc->Release ();
		pLoc->Release ();
		CoUninitialize ();
		strcpy (buffer, "Error Code 4");
		return buffer;
	}

	if (mode)
	{
		hres = pSvc->ExecQuery (_bstr_t ("WQL"), _bstr_t ("SELECT * FROM Win32_VideoController"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	}
	else
	{
		hres = pSvc->ExecQuery (_bstr_t ("WQL"), _bstr_t ("SELECT * FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	}

	if (FAILED (hres))
	{
		pSvc->Release ();
		pLoc->Release ();
		CoUninitialize ();
		strcpy (buffer, "Error Code 5");
		return buffer;
	}

	while (pEnumerator)
	{
		hr = pEnumerator->Next (WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn)
		{
			break;
		}
		VARIANT vtProp;
		hr = pclsObj->Get (L"Name", 0, &vtProp, 0, 0);
		WideCharToMultiByte (CP_ACP, 0, vtProp.bstrVal, -1, buffer, SysStringLen (vtProp.bstrVal)+1, NULL, NULL);
		VariantClear (&vtProp);
    }

	pSvc->Release ();
	pLoc->Release ();
	pEnumerator->Release ();
	pclsObj->Release ();
	CoUninitialize ();
	return buffer;
}

static int
printInfo (char *word[], char *word_eol[], void *user_data)
{
	xchat_printf (ph, "OS:\t%s\n", getOsName ());
	xchat_printf (ph, "CPU:\t%s (%s)\n", getWmiInfo (0), getCpuMhz ());
	xchat_printf (ph, "RAM:\t%s\n", getMemoryInfo ());
	xchat_printf (ph, "VGA:\t%s\n", getWmiInfo (1));
	/* will work correctly for up to 50 days, should be enough */
	xchat_printf (ph, "Uptime:\t%.2f Hours\n", (float) GetTickCount() / 1000 / 60 / 60);
	return XCHAT_EAT_XCHAT;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = "WinSys";
	*plugin_desc = "Display info about your hardware and OS";
	*plugin_version = "1.0";

	xchat_hook_command (ph, "WINSYS", XCHAT_PRI_NORM, printInfo, NULL, NULL);
	xchat_command (ph, "MENU -ietc\\download.png ADD \"Window/Display System Info\" \"WINSYS\"");

	xchat_printf (ph, "%s plugin loaded\n", *plugin_name);

	return 1;       /* return 1 for success */
}


int
xchat_plugin_deinit (void)
{
	xchat_command (ph, "MENU DEL \"Window/Display System Info\"");
	xchat_print (ph, "WinSys plugin unloaded\n");
	return 1;
}
