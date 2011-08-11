/******************************************************************
* $Id$
*
* $Log$
*
* Copyright © 2005 David Cullen, All rights reserved
*
******************************************************************/
#if !defined(AFX_WMPA_H__11200FE3_F137_48DD_8020_91CF7BBB283B__INCLUDED_)
#define AFX_WMPA_H__11200FE3_F137_48DD_8020_91CF7BBB283B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "WMPADialog.h"

/////////////////////////////////////////////////////////////////////////////
// CWmpaApp
// See wmpa.cpp for the implementation of this class
//

class CWmpaApp : public CWinApp
{
public:
	CWmpaApp();

   BOOL ShowWMPA(void);
   BOOL DestroyWMPA(void);

   CWMPADialog *m_pDialog;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmpaApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CWmpaApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BOOL StartWindowsMediaPlayer(void);
BOOL StopWindowsMediaPlayer(void);
CWMPPlayer4 *GetWindowsMediaPlayer(void);
CWMPADialog *GetWMPADialog(void);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMPA_H__11200FE3_F137_48DD_8020_91CF7BBB283B__INCLUDED_)
