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

#include <glib.h>

char *split(char *text, char separator)
{
	int pos = -1;
	size_t i;
	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == separator) {
			pos = i;
			i = strlen(text) + 1;
		}
	}

	if (pos == -1)
	{
		return text;
	}

	text[pos] = 0;
	return &(text[pos + 1]);
}

int endsWith(char *text, char *suffix){
    char *tmp=strstr(text,suffix);
    if (tmp==NULL) return 0;
    if (strlen(tmp)==strlen(suffix)) return 1;
    return 0;
}

int inStr(char *s1, size_t sl1, char *s2)
{
	size_t i;
	for (i = 0; i < sl1 - strlen(s2); i++)
	{
		size_t j;
		for (j = 0; j < strlen(s2); j++)
		{
			if (s1[i + j] != s2[j])
			{
				j = strlen(s2) + 2;
			}
		}

		if (j == strlen(s2))
		{
			return i;
		}
	}

	return -1;
}

static char *subString(char *text, int first, int length, int spcKill){
//if (DEBUG==1) putlog("creating substring");
	char *ret = g_new (char, length + 1);
	int i;
	ret[length]=0;
	for (i=0;i<length;i++){
		ret[i]=text[i+first];
		//if (ret[i]==0) ret[i]='0';
	}
	if (spcKill==1){
	   for (i=length-1;i>=0;i--){
           if (ret[i]==32) ret[i]=0;
           else i=-1;
       }
    }
    //if (DEBUG==1) putlog("substring created");
	return ret;
}

static char *substring(char *text, int first, int length){return subString(text,first,length,0);}


char *readLine(FILE *f){
     //if (DEBUG==1) putlog("reading line from file");
     char *buffer = g_new (char, 1024);
     int pos=0;
     int cc=0;
     while((cc!=EOF)&&(pos<1024)&&(cc!=10)){
          cc=fgetc(f);
          if ((cc!=10)&&(cc!=13)){
             if (cc==EOF) buffer[pos]=0;
             else buffer[pos]=(char)cc;pos++;
          }
     }
     if (buffer[pos]==EOF) hexchat_printf(ph,"EOF: %i\n",pos);
     return buffer;
}

char *toUpper(char *text)
{
	char *ret = (char*) calloc(strlen(text) + 1, sizeof(char));

	size_t i;
	for (i = 0; i < strlen(text); i++)
	{
		ret[i] = toupper(text[i]);
	}

	ret[strlen(text)] = 0;

	return ret;
}

static char *str3cat(char *s1, char *s2, char *s3){
       //if (DEBUG==1) putlog("cating 3 strings");
       char *ret=(char*)calloc(strlen(s1)+strlen(s2)+strlen(s3)+1,sizeof(char));
       strcpy(ret,s1);strcat(ret,s2);strcat(ret,s3);
       ret[strlen(s1)+strlen(s2)+strlen(s3)]=0;
       //if (DEBUG==1) putlog("strings cated");
       return ret;
}

char *replace(char *text, char *from, char *to){
     //if (DEBUG==1) putlog("replacing");
     char *ret=(char*)calloc( strlen(text)+(strlen(to)-strlen(from)),sizeof(char));
     char *left;
     char *right;
     int pos=inStr(text,strlen(text),from);
     if (pos!=-1){
           left=substring(text,0,pos);
           right=substring(text,pos+strlen(from),strlen(text)-(pos+strlen(from)));          
           ret=str3cat(left,to,right);
           return replace(ret,from,to);
     }
     //if (DEBUG==1) putlog("replaced");
     return text;
}

char *intReplaceF(char *text, char *from, int to, char *form){
     //if (DEBUG==1) putlog("replaceF");
     char *buffer=(char*) calloc(16,sizeof(char));
     sprintf(buffer,form,to);
     //if (DEBUG==1) putlog("replaceF done");
     return replace(text,from,buffer);
}

char *intReplace(char *text, char *from, int to){return intReplaceF(text,from,to,"%i");}
