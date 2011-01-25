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

#include "xchat-plugin.h"

#define PLAYING 1
#define PAUSED 3

static xchat_plugin *ph;   /* plugin handle */

BOOL winamp_found = FALSE;

int status = 0;

/* Slightly modified from X-Chat's log_escape_strcpy */
static char *
song_strcpy (char *dest, char *src)
{
	while (*src)
	{
		*dest = *src;
		dest++;
		src++;

		if (*src == '%')
		{
			dest[0] = '%';
			dest++;
		}
	}

	dest[0] = 0;
	return dest - 1;
}

static int
winamp(char *word[], char *word_eol[], void *userdata)
{

char current_play[2048], *p;
char p_esc[2048];
char cur_esc[2048];
char truc[2048];
HWND hwndWinamp = FindWindow("Winamp v1.x",NULL);

    if (hwndWinamp)
	{
	    {
	        if (!stricmp("PAUSE", word[2]))
			{
			   if (SendMessage(hwndWinamp,WM_USER, 0, 104))
				{
			   	   SendMessage(hwndWinamp, WM_COMMAND, 40046, 0);
			
			       if (SendMessage(hwndWinamp, WM_USER, 0, 104) == PLAYING)
			   	       xchat_printf(ph, "Winamp: playing");
			       else
                       xchat_printf(ph, "Winamp: paused");
				}
            }
			else
		        if (!stricmp("STOP", word[2]))
			    {
			       SendMessage(hwndWinamp, WM_COMMAND, 40047, 0);
			       xchat_printf(ph, "Winamp: stopped");
			    }
			else
			    if (!stricmp("PLAY", word[2]))
			    {
			         SendMessage(hwndWinamp, WM_COMMAND, 40045, 0);
			         xchat_printf(ph, "Winamp: playing");
			    }
        	else

			    if (!stricmp("NEXT", word[2]))
			    {
			         SendMessage(hwndWinamp, WM_COMMAND, 40048, 0);
			         xchat_printf(ph, "Winamp: next playlist entry");
			    }
			else

                if (!stricmp("PREV", word[2]))
			    {
			         SendMessage(hwndWinamp, WM_COMMAND, 40044, 0);
			         xchat_printf(ph, "Winamp: previous playlist entry");
			    }
		    else

                if (!stricmp("START", word[2]))
			    {
			         SendMessage(hwndWinamp, WM_COMMAND, 40154, 0);
			         xchat_printf(ph, "Winamp: playlist start");
			    }

		    else

                if (!word_eol[2][0])
			    {
					GetWindowText(hwndWinamp, current_play, sizeof(current_play));

					if (strchr(current_play, '-'))
					{
	
					p = current_play + strlen(current_play) - 8;
					while (p >= current_play)
					{
						if (!strnicmp(p, "- Winamp", 8)) break;
							p--;
					}

					if (p >= current_play) p--;
	
					while (p >= current_play && *p == ' ') p--;
						*++p=0;
	
	
					p = strchr(current_play, '.') + 1;

 					song_strcpy(p_esc, p);
 					song_strcpy(cur_esc, current_play);
	
					if (p)
					{
						sprintf(truc, "me is now playing:%s", p_esc);
					}
					else
					{
						sprintf(truc, "me is now playing:%s", cur_esc);
					}
	
	   				xchat_commandf(ph, truc);
	
				}
				else xchat_print(ph, "Winamp: Nothing being played.");
			}
		    else
                xchat_printf(ph, "Usage: /WINAMP [PAUSE|PLAY|STOP|NEXT|PREV|START]\n");
         }

	}
	else
	{
       xchat_print(ph, "Winamp not found.\n");
	}
	return XCHAT_EAT_ALL;
}

int
xchat_plugin_init(xchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
	/* we need to save this for use with any xchat_* functions */
	ph = plugin_handle;

	*plugin_name = "Winamp";
	*plugin_desc = "Winamp plugin for XChat";
	*plugin_version = "0.5";

	xchat_hook_command (ph, "WINAMP", XCHAT_PRI_NORM, winamp, "Usage: /WINAMP [PAUSE|PLAY|STOP|NEXT|PREV|START] - control Winamp or show what's currently playing", 0);
   	xchat_command (ph, "MENU -ietc\\music.png ADD \"Window/Display Current Song (Winamp)\" \"WINAMP\"");

	xchat_print (ph, "Winamp plugin loaded\n");

	return 1;       /* return 1 for success */
}

int
xchat_plugin_deinit(void)
{
	xchat_command (ph, "MENU DEL \"Window/Display Current Song (Winamp)\"");
	xchat_print (ph, "Winamp plugin unloaded\n");
	return 1;
}
