#include <windows.h>
#include <wininet.h>

char* check_version ()
{
	HINTERNET hINet, hFile;
	hINet = InternetOpen("XChat-WDK Update Checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	
	if (!hINet)
	{
		return "unavailable";
	}

	hFile = InternetOpenUrl (hINet, "http://xchat-wdk.googlecode.com/hg/version.txt", NULL, 0, 0, 0);
	
	if (hFile)
	{
		static char buffer[1024];
		DWORD dwRead;
		while (InternetReadFile(hFile, buffer, 1023, &dwRead))
		{
			if (dwRead == 0)
			{
				break;
			}
			buffer[dwRead] = 0;
		}
		
		return buffer;
		InternetCloseHandle (hFile);
	}
	
	InternetCloseHandle (hINet);

	return "unavailable";
}
