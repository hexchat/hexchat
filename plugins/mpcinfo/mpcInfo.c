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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

//static int DEBUG=0;
static char *VERSION="0.0.6";

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "xchat-plugin.h"
static xchat_plugin *ph;

#include "functions.c"
#include "mp3Info.c"
#include "oggInfo.c"
#include "theme.c"

static int print_themes (char *word[], char *word_eol[], void *userdata){
       printThemes();
       return XCHAT_EAT_ALL;
}

static int mpc_themeReload(char *word[], char *word_eol[], void *userdata){
   themeInit();
   loadThemes();
   return XCHAT_EAT_ALL;
}

static int mpc_tell(char *word[], char *word_eol[], void *userdata){
       char *tTitle, *zero, *oggLine, *line;
	   struct tagInfo info;
	   HWND hwnd = FindWindow("MediaPlayerClassicW",NULL);
       if (hwnd==0) {xchat_command(ph, randomLine(notRunTheme));return XCHAT_EAT_ALL;}
       
       tTitle=(char*)malloc(sizeof(char)*1024);
       GetWindowText(hwnd, tTitle, 1024);
       zero=strstr(tTitle," - Media Player Classic");
       if (zero!=NULL) zero[0]=0;
       else xchat_print(ph,"pattern not found");
       
       if ((tTitle[1]==':')&&(tTitle[2]=='\\')){
          //xchat_print(ph,"seams to be full path");
          if (endsWith(tTitle,".mp3")==1){
             //xchat_print(ph,"seams to be a mp3 file");
             info = readHeader(tTitle);
             
             if ((info.artist!=NULL)&&(strcmp(info.artist,"")!=0)){
                char *mode=MODES[info.mode];
                //xchat_printf(ph,"mode: %s\n",mode);
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
                xchat_command(ph, mp3Line);
                return XCHAT_EAT_ALL;
             }
          }
          if (endsWith(tTitle,".ogg")==1){
             xchat_printf(ph,"Ogg detected\n");
             info = getOggHeader(tTitle);
             if (info.artist!=NULL){
                char *cbr;
                if (info.cbr==1) cbr="CBR"; else cbr="VBR";
                oggLine=randomLine(oggTheme);
                //if (cue==1) oggLine=cueLine;
                //xchat_printf(ph,"ogg-line: %s\n",oggLine);
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
                xchat_command(ph, oggLine);
                return XCHAT_EAT_ALL;
             }
          }
       }
       line=randomLine(titleTheme);
       line=replace(line,"%title", tTitle);
       xchat_command(ph,line); 
       return XCHAT_EAT_ALL;
}

int xchat_plugin_init(xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg){
	ph = plugin_handle;
	*plugin_name = "mpcInfo";
	*plugin_desc = "Information-Script for Media Player Classic"; 
	*plugin_version=VERSION;

	xchat_hook_command(ph, "mpc", XCHAT_PRI_NORM, mpc_tell,"no help text", 0);
	xchat_hook_command(ph, "mpc_themes", XCHAT_PRI_NORM, print_themes,"no help text", 0);
	xchat_hook_command(ph, "mpc_reloadthemes", XCHAT_PRI_NORM, mpc_themeReload,"no help text", 0);
	xchat_command (ph, "MENU -ietc\\music.png ADD \"Window/Display Current Song (MPC)\" \"MPC\"");

	themeInit();
	loadThemes();
	xchat_printf(ph, "%s %s plugin loaded\n",*plugin_name, VERSION);

	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_command (ph, "MENU DEL \"Window/Display Current Song (MPC)\"");
	xchat_print (ph, "mpcInfo plugin unloaded\n");
	return 1;
}
