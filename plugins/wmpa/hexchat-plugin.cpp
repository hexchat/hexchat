/******************************************************************
* $Id$
*
* $Log$
*
* Copyright © 2005 David Cullen, All rights reserved
*
******************************************************************/
#include "stdafx.h"
#include "hexchat-plugin.h"
#include <windows.h>
#include <tchar.h>
#include "wmpa.h"
#include "WMPADialog.h"

#define XMMS_SESSION 0

/******************************************************************
* Globalss
******************************************************************/
hexchat_plugin *ph = NULL;
CWMPPlayer4 *wmp;
static const char subKey[] = "Software\\FlowerSoft\\WMPA";

/******************************************************************
* hexchat_plugin_init
******************************************************************/
int hexchat_plugin_init(hexchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
   BOOL success;

   ph = plugin_handle;

   *plugin_name = "WMPA";
   *plugin_desc = "Announce the current song from Windows Media Player.";
   *plugin_version = VER_STRING;

   // Show the song browser
   success = StartWindowsMediaPlayer();
   if (!success) {
      hexchat_printf(ph, "WMPA: Failed to show the song browser.");
      hexchat_printf(ph, "WMPA: Could not load plug-in version %s.", VER_STRING);
      return(E_FAIL);
   }

   // Get a pointer to the Windows Media Player control
   wmp = GetWindowsMediaPlayer();
   if (wmp == NULL) {
      hexchat_printf(ph, "WMPA: Failed to get a pointer to the Windows Media Player interface.");
      hexchat_printf(ph, "WMPA: Could not load plug-in version %s.", VER_STRING);
      return(E_POINTER);
   }

   // Restore the settings (need wmp first)
   success = wmpaRestoreSettings();
   if (!success) {
      hexchat_printf(ph, "WMPA: Failed to restore the settings.");
   }

   hexchat_hook_command(ph, "auto", HEXCHAT_PRI_NORM, wmpaAuto, 0, 0);
   hexchat_hook_command(ph, "curr", HEXCHAT_PRI_NORM, wmpaCurr, 0, 0);
   hexchat_hook_command(ph, "find", HEXCHAT_PRI_NORM, wmpaFind, 0, 0);
   hexchat_hook_command(ph, "slist", HEXCHAT_PRI_NORM, wmpaList, 0, 0);
   hexchat_hook_command(ph, "next", HEXCHAT_PRI_NORM, wmpaNext, 0, 0);
   hexchat_hook_command(ph, "play", HEXCHAT_PRI_NORM, wmpaPlay, 0, 0);
   hexchat_hook_command(ph, "pause", HEXCHAT_PRI_NORM, wmpaPause, 0, 0);
   hexchat_hook_command(ph, "prev", HEXCHAT_PRI_NORM, wmpaPrev, 0, 0);
   hexchat_hook_command(ph, "song", HEXCHAT_PRI_NORM, wmpaSong, 0, 0);
   hexchat_hook_command(ph, "stop", HEXCHAT_PRI_NORM, wmpaStop, 0, 0);
   hexchat_hook_command(ph, "volume", HEXCHAT_PRI_NORM, wmpaVolume, 0, 0);
   hexchat_hook_command(ph, "wmpahelp", HEXCHAT_PRI_NORM, wmpaHelp, 0, 0);

   hexchat_printf(ph, "WMPA %s successfully loaded.", VER_STRING);
   wmpaCommands();
   hexchat_printf(ph, "WMPA: e-mail me if you find any bugs: dcullen@intergate.com");

   return 1;
}

/******************************************************************
* hexchat_plugin_deinit
******************************************************************/
int hexchat_plugin_deinit(void)
{
   BOOL success;

   hexchat_printf(ph, "WMPA %s is unloading.", VER_STRING);

   // Save the settings
   success = wmpaSaveSettings();
   if (!success) {
      hexchat_printf(ph, "WMPA: Failed to save the settings.");
   }

   wmp = NULL;

   BOOL result = StopWindowsMediaPlayer();
   if (!result) {
      hexchat_printf(ph, "WMPA could not shut down Windows Media Player.");
   }

   hexchat_printf(ph, "WMPA %s has unloaded.", VER_STRING);
   return 1;
}

/******************************************************************
* hexchat_plugin_get_info
******************************************************************/
void hexchat_plugin_get_info(char **name, char **desc, char **version, void **reserved)
{
   *name = "WMPA";
   *desc = "Announce the current song from Windows Media Player.";
   *version = VER_STRING;
   if (reserved) *reserved = NULL;
}

/******************************************************************
* wmpaCommands
******************************************************************/
void wmpaCommands(void)
{
   hexchat_printf(ph, "WMPA: /auto [on/off]   : Turn on/off auto announce of the current song or display the current setting");
   hexchat_printf(ph, "WMPA: /curr            : Tell what song is currently playing");
   hexchat_printf(ph, "WMPA: /find [word]     : Find songs with \"word\" in their title, create a new playlist, and play it");
   hexchat_printf(ph, "WMPA: /slist [word]    : List songs with \"word\" in their title");
   hexchat_printf(ph, "WMPA: /next            : Play the next song");
   hexchat_printf(ph, "WMPA: /play            : Play the current song");
   hexchat_printf(ph, "WMPA: /pause           : Pause the current song");
   hexchat_printf(ph, "WMPA: /prev            : Play the previous song");
   hexchat_printf(ph, "WMPA: /song            : Announce the current song from Windows Media Player in HexChat");
   hexchat_printf(ph, "WMPA: /stop            : Stop the current song");
   hexchat_printf(ph, "WMPA: /volume [volume] : Set the volume (0 to 100) or display the current volume");
   hexchat_printf(ph, "WMPA: /wmpahelp        : Display this help.");
}

/******************************************************************
* wmpaAuto
******************************************************************/
int wmpaAuto(char *word[], char *word_eol[], void *user_data)
{
   CWMPADialog *pDialog;
   char *state;

   pDialog = GetWMPADialog();
   if (pDialog == NULL) return(HEXCHAT_EAT_ALL);

   if (CString(word[2]).IsEmpty()) {
      if (pDialog->autoAnnounce) {
         state = "on";
      }
      else {
         state = "off";
      }
   }
   else {
      state = word[2];
      if (CString(state) == "on") {
         pDialog->autoAnnounce = TRUE;
      }
      if (CString(state) == "off") {
         pDialog->autoAnnounce = FALSE;
      }
      wmpaSaveSettings();
   }

   hexchat_printf(ph, "WMPA: auto is %s", state);

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaCurr
******************************************************************/
int wmpaCurr(char *word[], char *word_eol[], void *user_data)
{
   hexchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaFind
******************************************************************/
int wmpaFind(char *word[], char *word_eol[], void *user_data)
{
   long index;
   long count;
   long found;

   if (wmp != NULL) {
      CWMPMediaCollection mc = wmp->GetMediaCollection();
      CWMPPlaylist all = mc.getAll();
      CWMPPlaylistCollection pc = wmp->GetPlaylistCollection();
      CWMPPlaylistArray pa = pc.getAll();
      CWMPPlaylist playlist;
      CWMPMedia media;

      for (index = 0; index < pc.getAll().GetCount(); index++) {
         if (pc.getAll().Item(index).GetName() == CString(word_eol[2])) {
            playlist = pc.getAll().Item(index);
            pc.remove(playlist);
         }
      }

      playlist = pc.newPlaylist(word_eol[2]);

      count = all.GetCount();
      found = 0;
      for (index = 0; index < count; index++) {
         media = all.GetItem(index);
         CString artist = media.getItemInfo("Artist");
         CString title  = media.getItemInfo("Title");
         CString album  = media.getItemInfo("Album");
         if ( (artist.Find(word_eol[2]) != -1) ||
              (title.Find(word_eol[2])  != -1) ||
              (album.Find(word_eol[2])  != -1) ) {
            playlist.appendItem(media);
            found++;
         }
      }

      if (found > 0) {
         hexchat_printf(ph, "WMPA: Found %d songs with \"%s\" in them", found, word_eol[2]);
         wmp->SetCurrentPlaylist(playlist);
         wmp->GetControls().play();
         hexchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());

         CWMPADialog *dialog = GetWMPADialog();
         if (dialog != NULL) {
            dialog->UpdateSongList();
            dialog->SelectCurrentSong();
            dialog->UpdatePlayLists();
         }

      }
      else {
         hexchat_printf(ph, "WMPA: Could not find %s", word_eol[2]);
      }

   }

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaList
******************************************************************/
int wmpaList(char *word[], char *word_eol[], void *user_data)
{
   long index;
   long count;
   long found;

   if (wmp != NULL) {
      hexchat_printf(ph, "WMPA: Listing songs with \"%s\" in them", word_eol[2]);

      CWMPMediaCollection mc = wmp->GetMediaCollection();
      CWMPPlaylist all = mc.getAll();
      CWMPMedia media;

      count = all.GetCount();
      found = 0;
      for (index = 0; index < count; index++) {
         media = all.GetItem(index);
         CString artist = media.getItemInfo("Artist");
         CString title  = media.getItemInfo("Title");
         CString album  = media.getItemInfo("Album");
         if ( (artist.Find(word_eol[2]) != -1) ||
              (title.Find(word_eol[2])  != -1) ||
              (album.Find(word_eol[2])  != -1) ) {
            hexchat_printf(ph, "WMPA: Found \"%s - %s (%s)\"", artist, title, album);
            found++;
         }
      }

      if (found > 0) {
         if (found == 1)
            hexchat_printf(ph, "WMPA: Found %d song with \"%s\" in it", found, word_eol[2]);
         else
            hexchat_printf(ph, "WMPA: Found %d songs with \"%s\" in them", found, word_eol[2]);
      }
      else {
         hexchat_printf(ph, "WMPA: Could not find any songs with \"%s\" in them", word_eol[2]);
      }

   }

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaNext
******************************************************************/
int wmpaNext(char *word[], char *word_eol[], void *user_data)
{
   if (wmp != NULL) {
      wmp->GetControls().next();
      hexchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaPlay
******************************************************************/
int wmpaPlay(char *word[], char *word_eol[], void *user_data)
{
   if (wmp != NULL) {
      wmp->GetControls().play();
      hexchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaPause
******************************************************************/
int wmpaPause(char *word[], char *word_eol[], void *user_data)
{
   if (wmp != NULL) {
      wmp->GetControls().pause();
      hexchat_printf(ph, "WMPA: Pausing %s", (LPCTSTR) wmpaGetSongTitle());
   }
   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaPrev
******************************************************************/
int wmpaPrev(char *word[], char *word_eol[], void *user_data)
{
   if (wmp != NULL) {
      wmp->GetControls().previous();
      hexchat_printf(ph, "WMPA: Playing %s", (LPCTSTR) wmpaGetSongTitle());
   }
   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaSong
******************************************************************/
int wmpaSong(char *word[], char *word_eol[], void *user_data)
{
   CString songTitle = wmpaGetSongTitle();

   hexchat_commandf(ph, "me is playing %s", (LPCTSTR) songTitle);

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaStop
******************************************************************/
int wmpaStop(char *word[], char *word_eol[], void *user_data)
{
   if (wmp != NULL) {
      wmp->GetControls().stop();
      hexchat_printf(ph, "WMPA: Stopping %s", (LPCTSTR) wmpaGetSongTitle());
   }
   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaHelp
******************************************************************/
int wmpaHelp(char *word[], char *word_eol[], void *user_data)
{
   hexchat_printf(ph, "\n");
   hexchat_printf(ph, "WMPA %s Help", VER_STRING);
   wmpaCommands();
   hexchat_printf(ph, "\n");

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaVolume
******************************************************************/
int wmpaVolume(char *word[], char *word_eol[], void *user_data)
{
   char *endPtr;
   long volume;

   if (CString(word[2]).IsEmpty()) {
      volume = wmp->GetSettings().GetVolume();
   }
   else {
      volume = strtol(word[2], &endPtr, 10);

      if ((wmp != NULL) && (volume >= 0) && (volume <= 100)) {
         wmp->GetSettings().SetVolume(volume);
         wmpaSaveSettings();
      }
   }

   hexchat_printf(ph, "WMPA: volume is %d", volume);

   return(HEXCHAT_EAT_ALL);
}

/******************************************************************
* wmpaRestoreSettings
******************************************************************/
BOOL wmpaRestoreSettings(void)
{
   CWMPADialog *pDialog;
   DWORD type;
   int volume;
   BOOL autoAnnounce;
   DWORD size;
   BOOL result;

   if (wmp == NULL) return(FALSE);

   volume = 50;
   result = GetSetting("Volume", &type, (LPBYTE) &volume, &size);
   wmp->GetSettings().SetVolume(volume);

   autoAnnounce = FALSE;
   pDialog = GetWMPADialog();
   if (pDialog != NULL) {
      result = result && GetSetting("Auto", &type, (LPBYTE) &autoAnnounce, &size);
      pDialog->autoAnnounce = autoAnnounce;
   }
   else {
      result = FALSE;
   }

   return(result);
}

/******************************************************************
* wmpaSaveSettings
******************************************************************/
BOOL wmpaSaveSettings(void)
{
   CWMPADialog *pDialog;
   int volume;
   BOOL autoAnnounce;
   BOOL result;

   if (wmp == NULL) return(FALSE);

   volume = wmp->GetSettings().GetVolume();
   result = SaveSetting("Volume", REG_DWORD, (CONST BYTE *) &volume, sizeof(volume));

   pDialog = GetWMPADialog();
   if (pDialog != NULL) {
      autoAnnounce = pDialog->autoAnnounce;
      result = result && SaveSetting("Auto", REG_DWORD, (CONST BYTE *) &autoAnnounce, sizeof(autoAnnounce));
   }
   else {
      result = FALSE;
   }

   return(result);
}

/******************************************************************
* wmpaGetSongTitle
******************************************************************/
CString wmpaGetSongTitle(void)
{
   char buffer[32];

   if (wmp == NULL) return(CString());

   CWMPMedia media      = wmp->GetCurrentMedia();
   if (media == NULL) {
      hexchat_printf(ph, "WMPA: Could not get current media");
      return(CString ("%d", HEXCHAT_EAT_ALL));
   }

   CString artist       = media.getItemInfo("Artist");
   CString title        = media.getItemInfo("Title");
   CString album        = media.getItemInfo("Album");
   CString bitrate      = media.getItemInfo("Bitrate");
   CString duration     = media.GetDurationString();

   long krate = strtoul((LPCTSTR) bitrate, NULL, 10) / 1000;
   _ultoa(krate, buffer, 10);
   bitrate = CString(buffer);

   // Creatte the song title
   CString songTitle("");
   songTitle += artist;
   if (songTitle.IsEmpty()) songTitle += "Various";
   songTitle += " - ";
   songTitle += title;
   songTitle += " (";
   songTitle += album;
   songTitle += ") [";
   songTitle += duration;
   songTitle += "/";
   songTitle += bitrate;
   songTitle += "Kbps]";

   return(songTitle);
}

/******************************************************************
* SaveSetting
******************************************************************/
BOOL SaveSetting(LPCTSTR name, DWORD type, CONST BYTE *value, DWORD size)
{
   HKEY hKey;
   DWORD disposition;
   LONG result;

   if (wmp == NULL) return(FALSE);
   if (name == NULL) return(FALSE);

   result = RegOpenKeyEx(HKEY_CURRENT_USER,
                         subKey,
                         0,
                         KEY_WRITE,
                         &hKey);

   if (result != ERROR_SUCCESS) {
      result = RegCreateKeyEx(HKEY_CURRENT_USER,
                              subKey,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &disposition);

      if (result != ERROR_SUCCESS) return(FALSE);
   }

   result = RegSetValueEx(hKey,
                          name,
                          0,
                          type,
                          value,
                          size);

   if (result == ERROR_SUCCESS) {
      RegCloseKey(hKey);
      return(TRUE);
   }

   RegCloseKey(hKey);
   return(FALSE);
}

/******************************************************************
* GetSetting
******************************************************************/
BOOL GetSetting(LPCTSTR name, DWORD *type, LPBYTE value, DWORD *size)
{
   HKEY hKey;
   LONG result;

   if (wmp == NULL) return(FALSE);
   if (type == NULL) return(FALSE);
   if (value == NULL) return(FALSE);
   if (size == NULL) return(FALSE);

   result = RegOpenKeyEx(HKEY_CURRENT_USER,
                         subKey,
                         0,
                         KEY_READ,
                         &hKey);

   if (result != ERROR_SUCCESS) return(FALSE);

   result = RegQueryValueEx(hKey,
                            name,
                            0,
                            type,
                            value,
                            size);

   RegCloseKey(hKey);

   if (result == ERROR_SUCCESS) {
      return(TRUE);
   }

   RegCloseKey(hKey);
   return(FALSE);
}

