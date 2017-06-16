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

static int getOggInt(char *buff, int beg, int bytes){
//if (DEBUG==1) putlog("getOggInt");
	int ret=0;
	int i;
	for (i=0;i<bytes;i++){
		if (buff[i+beg]>=0) ret+=buff[i+beg]*iPow(256,i);else ret+=(256+buff[i+beg])*iPow(256,i);
		//printf("[%i]=%i\n",i,buff[i+beg]);
	}
	return ret;
}

static char *upperStr(char *text)
{
	char *ret = (char*) malloc(sizeof(char)*(strlen(text) + 1));

	size_t i;
	for (i = 0; i < strlen(text); i++)
	{
		ret[i] = toupper(text[i]);
	}

	ret[strlen(text)] = 0;

	return ret;
}

struct tagInfo getOggHeader(char *file){
//if (DEBUG==1) putlog("reading ogg header");
	char header[4096];
	int i, c;
	int h1pos, h3pos, maxBr, nomBr, minBr, pos, count, tagLen;
	char *sub;
	char *name;
	char *val;
	char HEADLOC1[]="_vorbis", HEADLOC3[]="_vorbis", HEADLOC5[]="_vorbis";
	FILE *f;
	struct tagInfo info;

	info.artist=NULL;
	f = fopen(file,"rb");
	if (f==NULL){
       hexchat_print(ph,"file not found while trying to read ogg header");
       //if (DEBUG==1) putlog("file not found while trying to read ogg header");
       return info;
    }

	for (i=0;i<4095;i++) {c=fgetc(f);header[i]=(char)c;}
	fclose(f);
	HEADLOC1[0]=1;
	HEADLOC3[0]=3;
	HEADLOC5[0]=5;
	h1pos=inStr(header,4096,HEADLOC1);
	h3pos=inStr(header,4096,HEADLOC3);
	//int h5pos=inStr(header,4096,HEADLOC5); //not needed
	
	//printf("loc1: %i\n",h1pos);printf("loc3: %i\n",h3pos);printf("loc5: %i\n",h5pos);
	maxBr=getOggInt(header,h1pos+7+9,4);
	nomBr=getOggInt(header,h1pos+7+13,4);
	minBr=getOggInt(header,h1pos+7+17,4);
	info.freq=getOggInt(header,h1pos+7+5,4);
	info.mode=header[h1pos+7+4];
	info.bitrate=nomBr;
	if (((maxBr==nomBr)&&(nomBr=minBr))||((minBr==0)&&(maxBr==0))||((minBr=-1)&&(maxBr=-1)) )info.cbr=1;else info.cbr=0;
	printf("bitrates: %i|%i|%i\n",maxBr,nomBr,minBr);
	printf("freq: %u\n",info.freq);
	pos=h3pos+7;
	pos+=getOggInt(header,pos,4)+4;
	count=getOggInt(header,pos,4);
	//printf("tags: %i\n",count);
	pos+=4;

	info.artist=NULL;info.title=NULL;info.album=NULL;info.comment=NULL;info.genre=NULL;
	for (i=0;i<count;i++){
		tagLen=getOggInt(header,pos,4);
		//printf("taglength: %i\n",tagLen);
		sub=substring(header,pos+4,tagLen);
		name=upperStr(substring(sub,0,inStr(sub,tagLen,"=")));
		val=substring(sub,inStr(sub,tagLen,"=")+1,tagLen-inStr(sub,tagLen,"=")-1);
		//printf("Tag: %s\n",sub);
		//printf("Name: %s\n",name);
		//printf("value: %s\n",val);
		if (strcmp(name,"ARTIST")==0) info.artist=val;
		if (strcmp(name,"TITLE")==0) info.title=val;
		if (strcmp(name,"ALBUM")==0) info.album=val;
		if (strcmp(name,"GENRE")==0) info.genre=val;
		if (strcmp(name,"COMMENT")==0) info.comment=val;
		pos+=4+tagLen;
		free(name);
	}
	if (info.artist==NULL) info.artist="";
	if (info.album==NULL) info.album ="";
	if (info.title==NULL) info.title="";
	if (info.genre==NULL) info.genre="";
	if (info.comment==NULL) info.comment="";
	
	printf("Artist: %s\nTitle: %s\nAlbum: %s\n",info.artist,info.title, info.album);
	printf("Genre: %s\nComment: %s\nMode: %i\nCBR: %i\n",info.genre,info.comment,info.mode,info.cbr);
	//if (DEBUG==1) putlog("ogg header readed");
	return info;
}

/*
void printOggInfo(char *file){
	printf("Scanning Ogg-File for Informations: %s\n",file);
	printf("size:\t%10d byte\n",getSize(file));
	struct tagInfo info = getOggHeader(file);
}
*/
