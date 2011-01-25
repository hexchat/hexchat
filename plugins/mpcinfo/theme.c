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

#include <time.h>

struct theme{
       int size;
       char **line;
};

static struct theme notRunTheme;
static struct theme titleTheme;
static struct theme mp3Theme;
static struct theme oggTheme;


void themeInit(){
     //if (DEBUG==1) putlog("init theme");
     /*mp3Theme.size=0;oggTheme.size=0;cueTheme.size=0;streamTheme.size=0;etcTheme.size=0;
     stopTheme.size=0;pauseTheme.size=0;*/
     
     notRunTheme.size=0;titleTheme.size=0;
     srand((unsigned int)time((time_t *)NULL));
     //if (DEBUG==1) putlog("theme init done");
}

void printTheme(struct theme data){
     int i;
     for (i=0;i<data.size;i++) xchat_printf(ph,"line[%i]=%s\n",i,data.line[i]);
}

void printThemes(){
     xchat_printf(ph,"\nNotRun-Theme:\n");printTheme(notRunTheme);
     xchat_printf(ph,"\nMP3-Theme:\n");printTheme(mp3Theme);
     xchat_printf(ph,"\nOGG-Theme:\n");printTheme(oggTheme);
     xchat_printf(ph,"\nTitle-Theme:\n");printTheme(titleTheme);
}

void cbFix(char *line){
     //if (DEBUG==1) putlog("cbfix");
     int i, j;
     for (i=0;i<strlen(line);i++){
         if (line[i]=='%'){
            if ((line[i+1]=='C')||(line[i+1]=='B')||(line[i+1]=='U')||(line[i+1]=='O')||(line[i+1]=='R')){
               if(line[i+1]=='C') line[i]=3;
               if(line[i+1]=='B') line[i]=2;
               if(line[i+1]=='U') line[i]=37;
               if(line[i+1]=='O') line[i]=17;
               if(line[i+1]=='R') line[i]=26;

               for (j=i+1;j<strlen(line)-1;j++) line[j]=line[j+1];
               line[strlen(line)-1]=0;
            }
         }
     }
     //if (DEBUG==1) putlog("cbfix done");
}

struct theme themeAdd(struct theme data, char *info){
       //if (DEBUG==1) putlog("adding theme");
       struct theme ret;
       char **newLine=(char **)calloc(data.size+1,sizeof(char*));
       int i;
       for (i=0;i<data.size;i++) newLine[i]=data.line[i];
       cbFix(info);
       newLine[data.size]=info;
       ret.line=newLine;ret.size=data.size+1;
       //if (DEBUG==1) putlog("theme added");
       return ret;
}

void loadThemes(){
    char *hDir, *hFile, *line, *val;
	FILE *f;
	xchat_print(ph,"loading themes\n");
    hDir=(char*)calloc(1024,sizeof(char));
    strcpy(hDir,xchat_get_info(ph,"xchatdirfs"));
    hFile=str3cat(hDir,"\\","mpcInfo.theme.txt");
    f = fopen(hFile,"r");
    if(f==NULL)
	{
		xchat_print(ph,"no theme in homedir, checking global theme");
		f=fopen("mpcInfo.theme.txt","r");
    }
	//xchat_printf(ph,"file_desc: %p\n",f);
	if (f==NULL) xchat_print(ph, "no theme found, using hardcoded\n");
	else {
		if (f > 0)
		{
			line=" ";
		} else
		{
			line="\0";
		}

		while (line[0]!=0)
		{
			line=readLine(f);
			val=split(line,'=');
			printf("line: %s\n",line);
			printf("val: %s\n",val);
			if (strcmp(toUpper(line),"OFF_LINE")==0) notRunTheme=themeAdd(notRunTheme,val);
			if (strcmp(toUpper(line),"TITLE_LINE")==0) titleTheme=themeAdd(titleTheme,val);
			if (strcmp(toUpper(line),"MP3_LINE")==0) mp3Theme=themeAdd(mp3Theme,val);
			if (strcmp(toUpper(line),"OGG_LINE")==0) mp3Theme=themeAdd(oggTheme,val);
		}
		fclose(f);
		xchat_print(ph, "theme loaded successfull\n");
	}
	if (notRunTheme.size==0) notRunTheme=themeAdd(notRunTheme,"say Media Player Classic not running");
	if (titleTheme.size==0) titleTheme=themeAdd(titleTheme,"say Playing %title in Media Player Classic");
	if (mp3Theme.size==0) mp3Theme=themeAdd(mp3Theme,"me listens to %art with %tit from %alb [%gen|%br kbps|%frq kHz|%mode] in Media Player Classic ");
	if (oggTheme.size==0) oggTheme=themeAdd(oggTheme,"me listens to %art with %tit from %alb [%gen|%br kbps|%frq kHz|%chan channels] in Media Player Classic ");
	//mp3Theme=themeAdd(mp3Theme,"me listens to %art with %tit from %alb [%time|%length|%perc%|%br kbps|%frq kHz|%mode] in Media Player Classic ");
}

int rnd(int max){
    return rand()%max;
}

char *randomLine(struct theme data){
     return data.line[rnd(data.size)];
}
