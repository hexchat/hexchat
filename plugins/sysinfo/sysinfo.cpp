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

#include <windows.h>
#include <comutil.h>
#include <comdef.h>
#include <wbemidl.h>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <sstream>
#include <chrono>
#include <mutex>
#pragma comment(lib, "comsuppw.lib")

#include "hexchat-plugin.h"



namespace{
	_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
	_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
	_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
	_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

	static hexchat_plugin *ph;   /* plugin handle */
	static const char name[] = "SysInfo";
	static const char desc[] = "Display info about your hardware and OS";
	static const char version[] = "1.1";
	static const char helptext[] = "USAGE: /sysinfo - Sends info about your hardware and OS to current channel.";
	static bool firstRun;
	static ::std::string wmiOs;
	static ::std::string wmiCpu;
	static ::std::string wmiVga;

	class coInitialize{
		const HRESULT res;
	public:
		coInitialize(COINIT init)
			:res(CoInitializeEx(nullptr, init))
		{}
		~coInitialize(){
			if (SUCCEEDED(res)){
				CoUninitialize();
			}
		}
		operator HRESULT(){
			return res;
		}
	};

	static std::string narrow(const std::wstring& in)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conversion;
		return conversion.to_bytes(in);
	}

	static int
		getCpuArch()
	{
		OSVERSIONINFOEX osvi = { 0 };
		SYSTEM_INFO si = { 0 };

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		GetVersionEx((LPOSVERSIONINFOW)::std::addressof(osvi));

		GetSystemInfo(&si);

		if (si.wProcessorArchitecture == 9)
		{
			return 64;
		}
		else
		{
			return 86;
		}
	}

	struct reg_key{
		const HKEY key;
		reg_key(HKEY key)
			:key(key){};
		~reg_key(){
			RegCloseKey(key);
		}
	};

	static ::std::string
		getCpuMhz(void)
	{
		HKEY hKey;
		::std::stringstream buffer;

		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Hardware\\Description\\System\\CentralProcessor\\0"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
		{
			int data;
			reg_key regKey(hKey);
			DWORD dataSize = sizeof(data);
			LSTATUS result = RegQueryValueEx(hKey, TEXT("~MHz"), nullptr, nullptr, (LPBYTE)&data, &dataSize);
			if (result == ERROR_SUCCESS)
			{
				double cpuspeed = static_cast<double>((data > 1000) ? data / 1000 : data);
				buffer << cpuspeed << " " << ((data > 1000) ? "GHz" : "MHz");
			}
		}

		return buffer.str();
	}

	static ::std::string
		getMemoryInfo(void)
	{
		::std::stringstream buffer;
		//static char buffer[32] = { 0 };
		MEMORYSTATUSEX meminfo = { 0 };

		meminfo.dwLength = sizeof(meminfo);
		GlobalMemoryStatusEx(&meminfo);
		buffer << meminfo.ullTotalPhys / 1024 / 1024 << " MB Total (" << meminfo.ullAvailPhys / 1024 / 1024 << " MB Free)";
		return buffer.str();
	}

	enum class wmi_info_mode
	{
		os,
		processor,
		vga
	};

	static ::std::string
		getWmiInfo(wmi_info_mode mode)
	{
		/* for more details about this wonderful API, see
		http://msdn.microsoft.com/en-us/site/aa394138
		http://msdn.microsoft.com/en-us/site/aa390423
		http://msdn.microsoft.com/en-us/library/windows/desktop/aa394138%28v=vs.85%29.aspx
		http://social.msdn.microsoft.com/forums/en-US/vcgeneral/thread/d6420012-e432-4964-8506-6f6b65e5a451
		*/
		coInitialize init(COINIT_APARTMENTTHREADED);
		HRESULT hres;
		HRESULT hr;
		IWbemLocatorPtr pLoc;
		IWbemServicesPtr pSvc;
		IEnumWbemClassObjectPtr pEnumerator;

		if (FAILED(init))
		{
			return "Error Code 0";
		}

		hres = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

		/* mysteriously failing after the first execution, but only when used as a plugin, skip it */
		/*if (FAILED (hres))
		{
		CoUninitialize ();
		strcpy (buffer, "Error Code 1");
		return buffer;
		}*/

		hres = pLoc.CreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER);

		if (FAILED(hres))
		{
			return "Error Code 2";
		}

		hres = pLoc->ConnectServer(_bstr_t(L"root\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);

		if (FAILED(hres))
		{
			return "Error Code 3";
		}

		hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

		if (FAILED(hres))
		{
			return "Error Code 4";
		}

		switch (mode)
		{
		case wmi_info_mode::os:
			hres = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM Win32_OperatingSystem"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
			break;
		case wmi_info_mode::processor:
			hres = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
			break;
		case wmi_info_mode::vga:
			hres = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM Win32_VideoController"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
			break;

		}

		if (FAILED(hres))
		{
			return "Error Code 5";
		}
		::std::wstringstream buffer;
		while (pEnumerator)
		{
			ULONG uReturn = 0;
			IWbemClassObjectPtr pclsObj;
			hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (0 == uReturn)
			{
				break;
			}

			_variant_t vtProp;
			switch (mode)
			{
			case wmi_info_mode::os:
				hr = pclsObj->Get(L"Caption", 0, &vtProp, nullptr, nullptr);
				break;
			case wmi_info_mode::processor:
				hr = pclsObj->Get(L"Name", 0, &vtProp, nullptr, nullptr);
				break;
			case wmi_info_mode::vga:
				hr = pclsObj->Get(L"Name", 0, &vtProp, nullptr, nullptr);
				break;
			}
			buffer << vtProp.bstrVal;
		}
		return narrow(buffer.str());
	}

	static std::mutex g_mutex;

	static int
		printInfo(const char * const word[], const char * const word_eol[], void *user_data)
	{
		std::chrono::milliseconds ticks(GetTickCount64());
		using fp_hours = ::std::chrono::duration < float, ::std::chrono::hours::period > ;
		/* query WMI info only at the first time SysInfo is called, then cache it to save time */
		if (firstRun)
		{
			::std::lock_guard<::std::mutex> guard(g_mutex);
			hexchat_printf(ph, "%s first execution, querying and caching WMI info...\n", name);
			wmiOs = getWmiInfo(wmi_info_mode::os);
			wmiCpu = getWmiInfo(wmi_info_mode::processor);
			wmiVga = getWmiInfo(wmi_info_mode::vga);
			firstRun = false;
		}
		if (hexchat_list_int(ph, nullptr, "type") >= 2)
		{

			/* uptime will work correctly for up to 50 days, should be enough */
			hexchat_commandf(ph, "ME ** SysInfo ** Client: HexChat %s (x%d) ** OS: %s ** CPU: %s (%s) ** RAM: %s ** VGA: %s ** Uptime: %.2f Hours **",
				hexchat_get_info(ph, "version"),
				getCpuArch(),
				wmiOs.c_str(),
				wmiCpu.c_str(),
				getCpuMhz().c_str(),
				getMemoryInfo().c_str(),
				wmiVga.c_str(), fp_hours(ticks).count());
		}
		else
		{
			hexchat_printf(ph, " * Client:  HexChat %s (x%d)\n", hexchat_get_info(ph, "version"), getCpuArch());
			hexchat_printf(ph, " * OS:      %s\n", wmiOs.c_str());
			hexchat_printf(ph, " * CPU:     %s (%s)\n", wmiCpu.c_str(), getCpuMhz().c_str());
			hexchat_printf(ph, " * RAM:     %s\n", getMemoryInfo().c_str());
			hexchat_printf(ph, " * VGA:     %s\n", wmiVga.c_str());
			hexchat_printf(ph, " * Uptime:  %.2f Hours\n", fp_hours(ticks).count());
		}

		return HEXCHAT_EAT_HEXCHAT;
	}
}

int
hexchat_plugin_init(hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = const_cast<char*>(name);
	*plugin_desc = const_cast<char*>(desc);
	*plugin_version = const_cast<char*>(version);

	firstRun = true;

	hexchat_hook_command(ph, "SYSINFO", HEXCHAT_PRI_NORM, printInfo, helptext, nullptr);
	hexchat_command(ph, "MENU -ishare\\system.png ADD \"Window/Send System Info\" \"SYSINFO\"");

	hexchat_printf(ph, "%s plugin loaded\n", name);

	return 1;       /* return 1 for success */
}


int
hexchat_plugin_deinit(void)
{
	hexchat_command(ph, "MENU DEL \"Window/Display System Info\"");
	hexchat_printf(ph, "%s plugin unloaded\n", name);
	return 1;
}
