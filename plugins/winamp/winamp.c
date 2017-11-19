/********************* Winamp Plugin 0.3******************************
 *
 *   Distribution: GPL
 *
 *   Originally written by: Leo - leo.nard@free.fr
 *   Modified by: SilvereX - SilvereX@karklas.mif.vu.lt
 *   Modified again by: Derek Buitenhuis - daemon404@gmail.com
 *   Modified yet again by: Berke Viktor - berkeviktor@aol.com
 *********************************************************************/

#include "windows.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "hexchat-plugin.h"

#define PLAYING 1
#define PAUSED 3

static hexchat_plugin *ph;   /* plugin handle */

static int
winamp(char *word[], char *word_eol[], void *userdata)
{
	HWND hwndWinamp = FindWindowW(L"Winamp v1.x",NULL);

	if (hwndWinamp)
	{
		if (!stricmp("PAUSE", word[2]))
		{
			if (SendMessage(hwndWinamp,WM_USER, 0, 104))
			{
				SendMessage(hwndWinamp, WM_COMMAND, 40046, 0);
			
				if (SendMessage(hwndWinamp, WM_USER, 0, 104) == PLAYING)
					hexchat_printf(ph, "Winamp: playing");
				else
					hexchat_printf(ph, "Winamp: paused");
			}
		}
		else if (!stricmp("STOP", word[2]))
		{
			SendMessage(hwndWinamp, WM_COMMAND, 40047, 0);
			hexchat_printf(ph, "Winamp: stopped");
		}
		else if (!stricmp("PLAY", word[2]))
		{
			SendMessage(hwndWinamp, WM_COMMAND, 40045, 0);
			hexchat_printf(ph, "Winamp: playing");
		}
		else if (!stricmp("NEXT", word[2]))
		{
			SendMessage(hwndWinamp, WM_COMMAND, 40048, 0);
			hexchat_printf(ph, "Winamp: next playlist entry");
		}
		else if (!stricmp("PREV", word[2]))
		{
			SendMessage(hwndWinamp, WM_COMMAND, 40044, 0);
			hexchat_printf(ph, "Winamp: previous playlist entry");
		}
		else if (!stricmp("START", word[2]))
		{
			SendMessage(hwndWinamp, WM_COMMAND, 40154, 0);
			hexchat_printf(ph, "Winamp: playlist start");
		}
		else if (!word_eol[2][0])
		{
			wchar_t wcurrent_play[2048];
			char *current_play, *p;
			int len = GetWindowTextW(hwndWinamp, wcurrent_play, G_N_ELEMENTS(wcurrent_play));

			current_play = g_utf16_to_utf8 (wcurrent_play, len, NULL, NULL, NULL);
			if (!current_play)
			{
				hexchat_print (ph, "Winamp: Error getting song information.");
				return HEXCHAT_EAT_ALL;
			}

			if (strchr(current_play, '-'))
			{
				/* Remove any trailing text and whitespace */
				p = current_play + strlen(current_play) - 8;
				while (p >= current_play)
				{
					if (!strnicmp(p, "- Winamp", 8))
						break;
					p--;
				}

				if (p >= current_play)
					p--;

				while (p >= current_play && *p == ' ')
					p--;
				*++p = '\0';

				/* Ignore any leading track number */
				p = strstr (current_play, ". ");
				if (p)
					p += 2;
				else
					p = current_play;

				if (*p != '\0')
					hexchat_commandf (ph, "me is now playing: %s", p);
				else
					hexchat_print (ph, "Winamp: No song information found.");
				g_free (current_play);
			}
			else
				hexchat_print(ph, "Winamp: Nothing being played.");
		}
		else
			hexchat_printf(ph, "Usage: /WINAMP [PAUSE|PLAY|STOP|NEXT|PREV|START]\n");
	}
	else
	{
		hexchat_print(ph, "Winamp not found.\n");
	}
	return HEXCHAT_EAT_ALL;
}

int
hexchat_plugin_init(hexchat_plugin *plugin_handle,
					  char **plugin_name,
					  char **plugin_desc,
					  char **plugin_version,
					  char *arg)
{
	/* we need to save this for use with any hexchat_* functions */
	ph = plugin_handle;

	*plugin_name = "Winamp";
	*plugin_desc = "Winamp plugin for HexChat";
	*plugin_version = "0.6";

	hexchat_hook_command (ph, "WINAMP", HEXCHAT_PRI_NORM, winamp, "Usage: /WINAMP [PAUSE|PLAY|STOP|NEXT|PREV|START] - control Winamp or show what's currently playing", 0);
   	hexchat_command (ph, "MENU -ishare\\music.png ADD \"Window/Display Current Song (Winamp)\" \"WINAMP\"");

	hexchat_print (ph, "Winamp plugin loaded\n");

	return 1;	   /* return 1 for success */
}

int
hexchat_plugin_deinit(void)
{
	hexchat_command (ph, "MENU DEL \"Window/Display Current Song (Winamp)\"");
	hexchat_print (ph, "Winamp plugin unloaded\n");
	return 1;
}
