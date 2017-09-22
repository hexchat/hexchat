/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

//static int DEBUG=0;
static char *VERSION="0.0.6";
static int show_plugin_messages = 0;

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "hexchat-plugin.h"
static hexchat_plugin *ph;

#include "functions.c"
#include "mp3Info.c"
#include "oggInfo.c"
#include "theme.c"

static int print_themes (char *word[], char *word_eol[], void *userdata){
       printThemes();
       return HEXCHAT_EAT_ALL;
}

static int mpc_themeReload(char *word[], char *word_eol[], void *userdata){
   themeInit();
   loadThemes();
   return HEXCHAT_EAT_ALL;
}

static int mpc_tell(char *word[], char *word_eol[], void *userdata){
       char *tTitle, *zero, *oggLine, *line;
	   struct tagInfo info;
	   HWND hwnd = FindWindow("MediaPlayerClassicW",NULL);
       if (hwnd==0) {hexchat_print(ph, randomLine(notRunTheme));return HEXCHAT_EAT_ALL;}
       
       tTitle = g_new(char, 1024);
       GetWindowText(hwnd, tTitle, 1024);
       zero = strstr (tTitle, " - Media Player Classic");
	   if (zero != NULL)
	   {
		   zero[0] = 0;
	   }
	   else
	   {
		   g_free(tTitle);
		   hexchat_print(ph, "pattern not found");
		   return HEXCHAT_EAT_ALL;
	   }

       if ((tTitle[1]==':')&&(tTitle[2]=='\\')){
          //hexchat_print(ph,"seams to be full path");
          if (endsWith(tTitle,".mp3")==1){
             //hexchat_print(ph,"seams to be a mp3 file");
             info = readHeader(tTitle);
             
             if ((info.artist!=NULL)&&(strcmp(info.artist,"")!=0)){
                char *mode=MODES[info.mode];
                //hexchat_printf(ph,"mode: %s\n",mode);
                char *mp3Line=randomLine(mp3Theme);
                mp3Line=replace(mp3Line,"%art",info.artist);
                mp3Line=replace(mp3Line,"%tit",info.title);
                mp3Line=replace(mp3Line,"%alb",info.album);
                mp3Line=replace(mp3Line,"%com",info.comment);
                mp3Line=replace(mp3Line,"%gen",info.genre);
                //mp3Line=replace(mp3Line,"%time",pos);
                //mp3Line=replace(mp3Line,"%length",len);
                //mp3Line=replace(mp3Line,"%ver",waVers);
                //mp3Line=intReplace(mp3Line,"%br",br);
                //mp3Line=intReplace(mp3Line,"%frq",frq);
                
                mp3Line=intReplace(mp3Line,"%br",info.bitrate);
                mp3Line=intReplace(mp3Line,"%frq",info.freq);
                mp3Line=replace(mp3Line,"%mode",mode);
                //mp3Line=replace(mp3Line,"%size",size);
                //mp3Line=intReplace(mp3Line,"%perc",perc);
                //mp3Line=replace(mp3Line,"%plTitle",title);
                mp3Line=replace(mp3Line,"%file",tTitle);
				g_free(tTitle);
				hexchat_command(ph, mp3Line);
                return HEXCHAT_EAT_ALL;
             }
          }
          if (endsWith(tTitle,".ogg")==1){
             hexchat_printf(ph,"Ogg detected\n");
             info = getOggHeader(tTitle);
             if (info.artist!=NULL){
                char *cbr;
                if (info.cbr==1) cbr="CBR"; else cbr="VBR";
                oggLine=randomLine(oggTheme);
                //if (cue==1) oggLine=cueLine;
                //hexchat_printf(ph,"ogg-line: %s\n",oggLine);
                oggLine=replace(oggLine,"%art",info.artist);
                oggLine=replace(oggLine,"%tit",info.title);
                oggLine=replace(oggLine,"%alb",info.album);
                oggLine=replace(oggLine,"%com",info.comment);
                oggLine=replace(oggLine,"%gen",info.genre);
                //oggLine=replace(oggLine,"%time",pos);
                //oggLine=replace(oggLine,"%length",len);
                //oggLine=replace(oggLine,"%ver",waVers);
                oggLine=intReplace(oggLine,"%chan",info.mode);
                oggLine=replace(oggLine,"%cbr",cbr);
                oggLine=intReplace(oggLine,"%br",info.bitrate/1000);//br);
                oggLine=intReplace(oggLine,"%frq",info.freq);
                //oggLine=replace(oggLine,"%size",size);
                //oggLine=intReplace(oggLine,"%perc",perc);
                //oggLine=replace(oggLine,"%plTitle",title);
                oggLine=replace(oggLine,"%file",tTitle);
				g_free(tTitle);
				hexchat_command(ph, oggLine);
                return HEXCHAT_EAT_ALL;
             }
          }
       }
       line=randomLine(titleTheme);
       line=replace(line,"%title", tTitle);
	   g_free(tTitle);
	   hexchat_command(ph, line);
       return HEXCHAT_EAT_ALL;
}

int hexchat_plugin_init(hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg){
	ph = plugin_handle;
	*plugin_name = "mpcInfo";
	*plugin_desc = "Information-Script for Media Player Classic"; 
	*plugin_version=VERSION;

	hexchat_hook_command(ph, "mpc", HEXCHAT_PRI_NORM, mpc_tell,"no help text", 0);
	hexchat_hook_command(ph, "mpc_themes", HEXCHAT_PRI_NORM, print_themes,"no help text", 0);
	hexchat_hook_command(ph, "mpc_reloadthemes", HEXCHAT_PRI_NORM, mpc_themeReload,"no help text", 0);
	hexchat_command (ph, "MENU -ishare\\music.png ADD \"Window/Display Current Song (MPC)\" \"MPC\"");

	themeInit();
	loadThemes();

	hexchat_get_prefs (ph, "gui_show_plugin_messages", NULL, &show_plugin_messages);

	if (show_plugin_messages)
	{
		hexchat_printf(ph, "%s plugin loaded\n", *plugin_name);
	}

	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_command (ph, "MENU DEL \"Window/Display Current Song (MPC)\"");
	if (show_plugin_messages)
	{
		hexchat_print (ph, "mpcInfo plugin unloaded\n");
	}

	return 1;
}
