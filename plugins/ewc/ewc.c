/*
EasyWinampControl - A Winamp "What's playing" plugin for Xchat
Copyright (C) Yann HAMON & contributors

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "xchat-plugin.h"
#include <windows.h>

static xchat_plugin *ph;   /* plugin handle */
static int enable = 1;

// For example, circularstrstr("winamp", "pwi", 3) would return 5 (the index of p)
int circularstrstr(char* a, char* b, int nb)
{
  int equal = 1;
  int length;
  int pos=-1;
  int i, j;

  length = strlen(a);

  for (i=0; i<length && pos == -1; ++i) {
    equal = 1;
    for (j=0; j<nb;j++) {
      if (a[(i+j)%length] != b[j])
        equal = 0;
    }
    if (equal == 1)
      pos = i;
  }

  return pos;
}

void GetCurrentSongsName(HWND hwndWinamp, char* title, int titlesize)
{
  int pos;
  char *title2;
  int i, j=0;
  int length;
  char *p;

  GetWindowText(hwndWinamp, title, titlesize);
  length = strlen(title);

  if ((pos = circularstrstr(title, "- Winamp ***", 12)) != -1) {
    // The option "scroll song title in taskbar" is on
    title2 = (char*) malloc (titlesize*sizeof(char));

    for (i=(pos+12)%length; i!=pos; i=(i+1)%length)
      title2[j++] = title[i];

    title2[j] = '\0';

    p = title2;
    while (p<title2+titlesize && *p != '.')
      p++;
    p+=2; // Delete the . and the following white space

    strcpy(title, p);
    free(title2);
  }
  else {
    p = title;
    while (p<title+titlesize && *p != '.')
      p++;
    p+=2; // Delete the . and the following white space
    if (p<title+titlesize)
      strncpy(title, p, titlesize-(p-title));

    // Delete the trailing "- winamp"
    p = title + titlesize - 1;
    while (p>title && *p != '-') p--;
    *p = '\0';
  }
}


// Controlling winamp
static int wcmd_cb(char *word[], char *word_eol[], void *userdata)
{
  // Everything's here : http://winamp.com/nsdn/winamp2x/dev/sdk/api.php
  // The previous url seems dead, see http://forums.winamp.com/showthread.php?threadid=180297
  HWND hwndWinamp = NULL;

  if ((hwndWinamp = FindWindow("Winamp v1.x",NULL)) == NULL) {
    xchat_print(ph, "Winamp's window not found - Is winamp really running?\n");
  }
  else {
    if (strcmp(word[1], "") == 0)
      xchat_print(ph, "Usage: wcmd [command]\n");
    else if (strcmp(word[2], "next") == 0) {
      xchat_print(ph, "Loading next song...\n");
      SendMessage (hwndWinamp, WM_COMMAND, 40048, 0);
    }
    else if (strcmp(word[2], "prev") == 0) {
      xchat_print(ph, "Loading previous song...\n");
      SendMessage (hwndWinamp, WM_COMMAND, 40044, 0);
    }
    else if (strcmp(word[2], "play") == 0) {
      xchat_print(ph, "Playin'...\n");
      SendMessage (hwndWinamp, WM_COMMAND, 40045, 0);
    }
    else if (strcmp(word[2], "stop") == 0) {
      xchat_print(ph, "Winamp stopped!...\n");
      SendMessage (hwndWinamp, WM_COMMAND, 40047, 0);
    }
    else if (strcmp(word[2], "pause") == 0) {
      SendMessage (hwndWinamp, WM_COMMAND, 40046, 0);
    }
  }

  return XCHAT_EAT_ALL;
}


// Display current song
static int wp_cb(char *word[], char *word_eol[], void *userdata)
{
  HWND hwndWinamp = NULL;
  int bitrate, length, elapsed, minutes, seconds, eminutes, eseconds, samplerate, nbchannels;
  char elapsedtime[7];
  char totaltime[7];
  char this_title[1024];

  if ((hwndWinamp = FindWindow("Winamp v1.x",NULL)) == NULL)
    xchat_print(ph, "Winamp's window not found - Is winamp really running?\n");
  else {
    //Winamp's running
    // Seems buggy when winamp2's agent is running, and winamp not (or winamp3) -> crashes xchat.
    SendMessage(hwndWinamp, WM_USER, (WPARAM)0, (LPARAM)125);

    if ((samplerate = SendMessage(hwndWinamp, WM_USER, (WPARAM)0, (LPARAM)126)) == 0) {
      xchat_print(ph, "Could not get current song's samplerate... !?\n");
      return XCHAT_EAT_ALL;
    }
    if ((bitrate = SendMessage(hwndWinamp, WM_USER, (WPARAM)1, (LPARAM)126)) == 0) {
      xchat_print(ph, "Could not get current song's bitrate... !?\n");
      return XCHAT_EAT_ALL;
    }
    if ((nbchannels = SendMessage(hwndWinamp, WM_USER, (WPARAM)2, (LPARAM)126)) == 0) {
      xchat_print(ph, "Could not get the number of channels... !?\n");
      return XCHAT_EAT_ALL;
    }
    if ((length = SendMessage(hwndWinamp, WM_USER, (WPARAM)1, (LPARAM)105)) == 0) {
      // Could be buggy when streaming audio or video, returned length is unexpected;
      // How to detect is Winamp is streaming, and display ??:?? in that case?
      xchat_print(ph, "Could not get current song's length... !?\n");
      return XCHAT_EAT_ALL;
    }
    else {
      minutes = length/60;
      seconds = length%60;

      if (seconds>9)
        wsprintf(totaltime, "%d:%d", minutes, seconds);
      else
        wsprintf(totaltime, "%d:0%d", minutes, seconds);
    }
    if ((elapsed = SendMessage(hwndWinamp, WM_USER, (WPARAM)0, (LPARAM)105)) == 0) {
      xchat_print(ph, "Could not get current song's elapsed time... !?\n");
      return XCHAT_EAT_ALL;
    }
    else {
      eminutes = (elapsed/1000)/60;   /* kinda stupid sounding, but e is for elapsed */
      eseconds = (elapsed/1000)%60;

      if (eseconds>9)
        wsprintf(elapsedtime, "%d:%d", eminutes, eseconds);
      else
        wsprintf(elapsedtime, "%d:0%d", eminutes, eseconds);
    }

    if ((bitrate = SendMessage(hwndWinamp, WM_USER, (WPARAM)1, (LPARAM)126)) == 0) {
      xchat_print(ph, "Could not get current song's bitrate... !?\n");
      return XCHAT_EAT_ALL;
    }

    GetCurrentSongsName(hwndWinamp, this_title, 1024);

    xchat_commandf(ph, "dispcurrsong %d %d %d %s %s %s", samplerate, bitrate, nbchannels, elapsedtime, totaltime, this_title);
  }

  return XCHAT_EAT_ALL;   /* eat this command so xchat and other plugins can't process it */
}



int xchat_plugin_init(xchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
  /* we need to save this for use with any xchat_* functions */
  ph = plugin_handle;

  *plugin_name = "EasyWinampControl";
  *plugin_desc = "Some commands to remotely control winamp";
  *plugin_version = "1.2";

  xchat_hook_command(ph, "wp", XCHAT_PRI_NORM, wp_cb,
                    "Usage: wp", 0);

  xchat_hook_command(ph, "wcmd", XCHAT_PRI_NORM, wcmd_cb,
                    "Usage: wcmd [play|pause|stop|prev|next]", 0);

  xchat_print(ph, "EasyWinampControl plugin loaded\n");

  return 1;       /* return 1 for success */
}

int xchat_plugin_deinit(void)
{
  xchat_print(ph, "EasyWinampControl plugin unloaded\n");
  return 1;
}
