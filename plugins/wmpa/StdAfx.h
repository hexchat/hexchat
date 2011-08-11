// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__33D7BD1A_A9B6_4BDE_B867_5278529B95B2__INCLUDED_)
#define AFX_STDAFX_H__33D7BD1A_A9B6_4BDE_B867_5278529B95B2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

/******************************************************************
* Includes
******************************************************************/
#include "wmpcdrom.h"
#include "wmpcdromcollection.h"
#include "wmpclosedcaption.h"
#include "wmpcontrols.h"
#include "wmpdvd.h"
#include "wmperror.h"
#include "wmperroritem.h"
#include "wmpmedia.h"
#include "wmpmediacollection.h"
#include "wmpnetwork.h"
#include "wmpplayer4.h"
#include "wmpplayerapplication.h"
#include "wmpplaylist.h"
#include "wmpplaylistarray.h"
#include "wmpplaylistcollection.h"
#include "wmpsettings.h"
#include "wmpstringcollection.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__33D7BD1A_A9B6_4BDE_B867_5278529B95B2__INCLUDED_)
