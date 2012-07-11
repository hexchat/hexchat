/******************************************************************
* $Id$
*
* $Log$
*
* Copyright © 2005 David Cullen, All rights reserved
*
******************************************************************/
#include "stdafx.h"
#include "wmpa.h"
#include "WMPADialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CWmpaApp

BEGIN_MESSAGE_MAP(CWmpaApp, CWinApp)
	//{{AFX_MSG_MAP(CWmpaApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWmpaApp construction

CWmpaApp::CWmpaApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
   m_pDialog = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWmpaApp object

CWmpaApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWmpaApp initialization

BOOL CWmpaApp::InitInstance()
{
	// Register all OLE server (factories) as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();

   // WARNING: This function enables the ActiveX control container
   // Without this function you will not be able to load the WMP
   // In fact you will get the following error:
   // >>> If this dialog has OLE controls:
   // >>> AfxEnableControlContainer has not been called yet.
   // >>> You should call it in your app's InitInstance function.
   AfxEnableControlContainer();

   // WARNING: This function initializes the COM library for use
   // Without this function you will not be able to load the WMP
   // In fact you will get the following error:
   // CoCreateInstance of OLE control {6BF52A52-394A-11D3-B153-00C04F79FAA6} failed.
   // >>> Result code: 0x800401f0
   // >>> Is the control is properly registered?
   // The Error Lookup tool will tell you result code 0x800401F0 means
   // CoInitialize has not been called.
   CoInitialize(NULL);

   return TRUE;
}

/******************************************************************
* ShowWMPA
******************************************************************/
BOOL CWmpaApp::ShowWMPA(void)
{
   HRESULT result = FALSE;
   BOOL created;

   // WARNING: The following two funcions make sure we look for
   // our resources in our DLL not in the calling EXE
   // Without these functions you will not be able to load the
   // Windows Media Player
   HMODULE hModule = GetModuleHandle("WMPA.DLL");
   AfxSetResourceHandle((HINSTANCE) hModule);

   if (m_pDialog == NULL) m_pDialog = new CWMPADialog;

   created = m_pDialog->Create(IDD_WMPADIALOG, m_pDialog);
   if (!created) return(E_FAIL);
   m_pDialog->m_hIcon = LoadIcon(IDI_XCHAT);
   m_pDialog->SetIcon(m_pDialog->m_hIcon, TRUE);

   result = m_pDialog->ShowWindow(SW_SHOWNORMAL);

   return(created);
}

/////////////////////////////////////////////////////////////////////////////
// Special entry points required for inproc servers

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllCanUnloadNow();
}

// by exporting DllRegisterServer, you can use regsvr.exe
STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	COleObjectFactory::UpdateRegistryAll();
   return(S_OK);
}

/******************************************************************
* DestroyWMPA
******************************************************************/
BOOL CWmpaApp::DestroyWMPA(void)
{
   if (theApp.m_pDialog == NULL) return(FALSE);

   theApp.m_pDialog->m_WMP.GetControls().stop();
   theApp.m_pDialog->DeleteTrayIcon();
   theApp.m_pDialog->DestroyWindow();

   return(TRUE);
}

/******************************************************************
* StartWindowsMediaPlayer
******************************************************************/
BOOL StartWindowsMediaPlayer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

   BOOL result = theApp.ShowWMPA();

   return(result);
}

/******************************************************************
* StopWindowsMediaPlayer
******************************************************************/
BOOL StopWindowsMediaPlayer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

   BOOL result = theApp.DestroyWMPA();

   return(result);
}

/******************************************************************
* GetWindowsMediaPlayer
******************************************************************/
CWMPPlayer4 *GetWindowsMediaPlayer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (theApp.m_pDialog == NULL) return(NULL);

	return(&(theApp.m_pDialog->m_WMP));
}

/******************************************************************
* GetWMPADialog
******************************************************************/
CWMPADialog *GetWMPADialog(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (theApp.m_pDialog == NULL) return(NULL);

	return(theApp.m_pDialog);
}

