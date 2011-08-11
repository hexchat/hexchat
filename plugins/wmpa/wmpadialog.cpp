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
#include "shellapi.h"
#include "xchat-plugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ID_TRAY_ICON    1000
#define WM_TRAY_ICON    (WM_APP + 1)

/////////////////////////////////////////////////////////////////////////////
// CWMPADialog dialog

CWMPADialog::CWMPADialog(CWnd* pParent /*=NULL*/)
	: CDialog(CWMPADialog::IDD, pParent)
{
	EnableAutomation();

	//{{AFX_DATA_INIT(CWMPADialog)
	//}}AFX_DATA_INIT
}


void CWMPADialog::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CDialog::OnFinalRelease();
}

void CWMPADialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWMPADialog)
	DDX_Control(pDX, IDC_SONGLIST, m_SongListBox);
	DDX_Control(pDX, IDC_PLAYLIST, m_PlaylistBox);
	DDX_Control(pDX, IDC_WMP, m_WMP);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWMPADialog, CDialog)
	//{{AFX_MSG_MAP(CWMPADialog)
	ON_LBN_DBLCLK(IDC_PLAYLIST, OnDblclkPlaylist)
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	ON_LBN_DBLCLK(IDC_SONGLIST, OnDblclkSonglist)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CWMPADialog, CDialog)
	//{{AFX_DISPATCH_MAP(CWMPADialog)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IWMPADialog to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {01C1B3AA-C7FC-4023-89A5-C814E1B62B9B}
static const IID IID_IWMPADialog =
{ 0x1c1b3aa, 0xc7fc, 0x4023, { 0x89, 0xa5, 0xc8, 0x14, 0xe1, 0xb6, 0x2b, 0x9b } };

BEGIN_INTERFACE_MAP(CWMPADialog, CDialog)
	INTERFACE_PART(CWMPADialog, IID_IWMPADialog, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWMPADialog message handlers

void CWMPADialog::OnDblclkPlaylist()
{
	// TODO: Add your control notification handler code here
   long index;

   // Get the playlist name
   index = m_PlaylistBox.GetCurSel();
   CString playlistName;
   m_PlaylistBox.GetText(index, playlistName);

   // Get the playlist
   CWMPPlaylistCollection pc = m_WMP.GetPlaylistCollection();
   CWMPPlaylistArray pa = pc.getByName((LPCTSTR) playlistName);
   CWMPPlaylist playlist = pa.Item(0);
   m_WMP.SetCurrentPlaylist(playlist);

   // Set the song list
   UpdateSongList();

   m_WMP.GetControls().play();
   if (autoAnnounce) {
      xchat_commandf(ph, "me is playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
   else {
      xchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
}

void CWMPADialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	// TODO: Add your message handler code here
   if (bShow) {
      if (!trayInit) {
         ZeroMemory(&nid, sizeof(nid));
         nid.cbSize = sizeof(nid);
         nid.hWnd = m_hWnd;
         nid.uID = ID_TRAY_ICON;
         nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
         nid.uCallbackMessage = WM_TRAY_ICON;
         nid.hIcon = m_hIcon;
         strcpy(nid.szTip, "WMPA");
         strcat(nid.szTip,  VER_STRING);
         strcat(nid.szTip, "\nWindows Media\nPlayer Announcer");
         nid.dwState = 0;
         nid.dwStateMask = 0;
         strcpy(nid.szInfo, "WMPA ");
         strcat(nid.szInfo, VER_STRING);
         nid.uTimeout = 10000; // 10 second time out
         strcat(nid.szInfoTitle, "WMPA");
         nid.dwInfoFlags = 0;
         trayInit = TRUE;
      }
   }

}


void CWMPADialog::PostNcDestroy()
{
	// TODO: Add your specialized code here and/or call the base class
   delete this;

	CDialog::PostNcDestroy();
}


BOOL CWMPADialog::OnInitDialog()
{
	CDialog::OnInitDialog();

   CString title = "WMPA";
   title += " ";
   title += VER_STRING;
   SetWindowText((LPCTSTR) title);

   UpdatePlayLists();

   autoAnnounce = FALSE;
   trayInit = FALSE;
   trayClicked = FALSE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWMPADialog::OnClose()
{
	// TODO: Add your message handler code here and/or call default
   Shell_NotifyIcon(NIM_ADD, &nid);
   ShowWindow(SW_HIDE);

   // Don't let the user close the dialog
//   CDialog::OnClose();
}

void CWMPADialog::OnCancel()
{
//   this->DestroyWindow();
}

void CWMPADialog::OnDblclkSonglist()
{
	// TODO: Add your control notification handler code here
   int index = m_SongListBox.GetCurSel();
   m_WMP.GetControls().playItem(m_WMP.GetCurrentPlaylist().GetItem(index));
   if (autoAnnounce) {
      xchat_commandf(ph, "me is playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
   else {
      xchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
}

BEGIN_EVENTSINK_MAP(CWMPADialog, CDialog)
    //{{AFX_EVENTSINK_MAP(CWMPADialog)
	ON_EVENT(CWMPADialog, IDC_WMP, 5806 /* CurrentItemChange */, OnCurrentItemChangeWmp, VTS_DISPATCH)
	ON_EVENT(CWMPADialog, IDC_WMP, 5101 /* PlayStateChange */, OnPlayStateChangeWmp, VTS_I4)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CWMPADialog::OnCurrentItemChangeWmp(LPDISPATCH pdispMedia)
{
	// TODO: Add your control notification handler code here

   int state = m_WMP.GetPlayState();
   if (state == 3) { // Playing
      if (autoAnnounce) {
         xchat_commandf(ph, "me is playing %s", (LPCTSTR) wmpaGetSongTitle());
      }
      else {
         xchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());
      }
   }

   SelectCurrentSong();
}

void CWMPADialog::OnDestroy()
{
	CDialog::OnDestroy();
	
   // TODO: Add your message handler code here
}

void CWMPADialog::UpdateSongList()
{
   char buffer[32];
   CString name;
   CString artist;
   CString title;
   CString album;
   CString bitrate;
   CString duration;
   CString song;
   long index;
   long count;

   m_SongListBox.ResetContent();
   CWMPPlaylist playlist = m_WMP.GetCurrentPlaylist();
   count = playlist.GetCount();
   m_SongListBox.ResetContent();
   for (index = 0; index < count; index++) {
      name         = playlist.GetItem(index).GetName();
      artist       = playlist.GetItem(index).getItemInfo("Artist");
      title        = playlist.GetItem(index).getItemInfo("Title");
      album        = playlist.GetItem(index).getItemInfo("Album");
      bitrate      = playlist.GetItem(index).getItemInfo("Bitrate");
      duration     = playlist.GetItem(index).GetDurationString();

      long krate = strtoul((LPCTSTR) bitrate, NULL, 10) / 1000;
      _ultoa(krate, buffer, 10);
      bitrate = CString(buffer);

      if (album.IsEmpty()) {
         playlist.removeItem(playlist.GetItem(index));
         count = playlist.GetCount();
      }
      else {
         song = "";
         song += artist;
         if (song.IsEmpty()) song = "Various";
         song += " - ";
         song += title;
         song += " (";
         song += album;
         song += ") [";
         song += duration;
         song += "/";
         song += bitrate;
         song += "Kbps]";
         m_SongListBox.AddString((LPCTSTR) song);
      }

   }
   m_SongListBox.SetCurSel(0);
}

void CWMPADialog::SelectCurrentSong()
{
   CWMPMedia media;
   long index;
   long count;

   count = m_WMP.GetCurrentPlaylist().GetCount();
   for (index = 0; index < count; index++) {
      media = m_WMP.GetCurrentPlaylist().GetItem(index);
      if (m_WMP.GetCurrentMedia().GetIsIdentical(media)) {
         m_SongListBox.SetCurSel(index);
      }
   }
}

void CWMPADialog::UpdatePlayLists()
{
	// TODO: Add extra initialization here
   CWMPPlaylistCollection pc = m_WMP.GetPlaylistCollection();
   CWMPPlaylistArray pa = pc.getAll();
   CWMPPlaylist playlist;

   int index;
   int count = pa.GetCount();
   m_PlaylistBox.ResetContent();
   for (index = 0; index < count; index++) {
      playlist = pa.Item(index);
      m_PlaylistBox.AddString((LPCTSTR) playlist.GetName());
   }
}


void CWMPADialog::OnPlayStateChangeWmp(long NewState)
{
	// TODO: Add your control notification handler code here
}

void CWMPADialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	// TODO: Add your message handler code here
   switch (nType) {
      case SIZE_MINIMIZED:
         break;

      case SIZE_RESTORED:
         Shell_NotifyIcon(NIM_DELETE, &nid);
         break;

      default:
         break;
   }
}

LRESULT CWMPADialog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
   // TODO: Add your specialized code here and/or call the base class
   switch (message) {
      case WM_TRAY_ICON:
         switch (lParam) {
            case WM_LBUTTONDBLCLK:
               trayClicked = TRUE;
               break;

            case WM_LBUTTONUP:
               if (trayClicked == TRUE) {
                  ShowWindow(SW_RESTORE);
               }
               break;

            default:
               trayClicked = FALSE;
               break;
         }
         return(TRUE);
         break;

      default:
         return CDialog::WindowProc(message, wParam, lParam);
         break;
   }
}

void CWMPADialog::DeleteTrayIcon()
{
   Shell_NotifyIcon(NIM_DELETE, &nid);
}
