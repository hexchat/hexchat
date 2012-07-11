/******************************************************************
* $Id$
*
* $Log$
*
* Copyright © 2005 David Cullen, All rights reserved
*
******************************************************************/
//{{AFX_INCLUDES()
#include "wmpplayer4.h"
//}}AFX_INCLUDES
#if !defined(AFX_WMPADIALOG_H__D3838BCC_9E26_4FC0_BD42_C8D8EDF057E3__INCLUDED_)
#define AFX_WMPADIALOG_H__D3838BCC_9E26_4FC0_BD42_C8D8EDF057E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WMPADialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWMPADialog dialog

class CWMPADialog : public CDialog
{
// Construction
public:
	CWMPADialog(CWnd* pParent = NULL);   // standard constructor
   virtual void OnCancel();
   void UpdatePlayLists();
   void UpdateSongList();
   void SelectCurrentSong();
   void DeleteTrayIcon();
   BOOL autoAnnounce;
   HICON m_hIcon;

private:
   BOOL trayInit;
   BOOL trayClicked;
   NOTIFYICONDATA nid;

public:
// Dialog Data
	//{{AFX_DATA(CWMPADialog)
	enum { IDD = IDD_WMPADIALOG };
	CListBox	m_SongListBox;
	CListBox	m_PlaylistBox;
	CWMPPlayer4	m_WMP;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWMPADialog)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWMPADialog)
	afx_msg void OnDblclkPlaylist();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnDblclkSonglist();
	afx_msg void OnCurrentItemChangeWmp(LPDISPATCH pdispMedia);
	afx_msg void OnDestroy();
	afx_msg void OnPlayStateChangeWmp(long NewState);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CWMPADialog)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMPADIALOG_H__D3838BCC_9E26_4FC0_BD42_C8D8EDF057E3__INCLUDED_)
