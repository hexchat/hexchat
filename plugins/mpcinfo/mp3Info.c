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

//#include <stdio.h>
#include <sys/stat.h>
//#include "functions.c"

struct tagInfo{
	int mode;
	int cbr;
	int bitrate;
	unsigned int freq;
	char *artist;
	char *title;
	char *album;
	char *comment;
	char *genre;
	//int genre;
	//int track;
};

static int RATES[2][3][15]={
				{//mpeg2
                    {-1,8,16,24,32,64,80,56,64,128,160,112,128,256,320},//layer3 (V2)    
					{-1,32,48,56,64,80,96,112,128,160,192,224,256,320,384},//layer2 (V2)
					{-1,32,64,96,128,160,192,224,256,288,320,352,384,416,448},//layer1 (V2)
                },
                {//mpeg1
					{-1,32,40,48,56,64,80,96,112,128,160,192,224,256,320},//layer3 (V1)
					{-1,32,48,56,64,80,96,112,128,160,192,224,256,320,384},//layer2 (V1)
                    {-1,32,64,96,128,160,192,224,256,288,320,352,384,416,448},//layer1 (V1)
        }};
static int FREQS[2][4]={{22050,24000,16000,-1},{44100,48000,32000,-1}};
//static double FRATES[]={38.5,32.5,27.8,0.0};

static char GENRES[][50]={"Blues","Classic Rock","Country","Dance","Disco","Funk","Grunge","Hip-Hop","Jazz","Metal",
"New Age","Oldies","Other","Pop","R&B","Rap","Reggae","Rock","Techno","Industrial",
"Alternative","Ska","Death Metal","Pranks","Soundtrack","Euro-Techno","Ambient","Trip-Hop","Vocal","Jazz+Funk",
"Fusion","Trance","Classical","Instrumental","Acid","House","Game","Sound Clip","Gospel","Noise",
"AlternRock","Bass","Soul","Punk","Space","Meditative","Instrumental Pop","Instrumental Rock","Ethnic","Gothic",
"Darkwave","Techno-Industrial","Electronic","Pop-Folk","Eurodance","Dream","Southern Rock","Comedy","Cult","Gangsta",
"Top 40","Christian Rap","Pop/Funk","Jungle","Native American","Cabaret","New Wave","Psychadelic","Rave","Showtunes",
"Trailer","Lo-Fi","Tribal","Acid Punk","Acid Jazz","Polka","Retro","Musical","Rock & Roll","Hard Rock",

//################## END OF OFFICIAL ID3 TAGS, WINAMP TAGS BELOW ########################################

"Folk","Folk/Rock","National Folk","Swing","Fast Fusion","Bebob","Latin","Revival","Celtic","Bluegrass",
"Avantgarde","Gothic Rock","Progressive Rock","Psychedelic Rock","Symphonic Rock","Slow Rock","Big Band","Chorus","Easy Listening","Acoustic",
"Humour","Speech","Chanson","Opera","Chamber Music","Sonata","Symphony","Booty Bass","Primus","Porn Groove",
"Satire","Slow Jam","Club","Tango","Samba","Folklore","Ballad","Poweer Ballad","Rhytmic Soul","Freestyle",
"Duet","Punk Rock","Drum Solo","A Capela","Euro-House","Dance Hall",

//################## FOUND AT http://en.wikipedia.org/wiki/ID3 ###########################################

"Goa","Drum & Bass","Club-House","Hardcore",
"Terror","Indie","BritPop","Negerpunk","Polsk Punk","Beat","Christian Gangsta Rap","Heavy Metal","Black Metal","Crossover",
"Contemporary Christian","Christian Rock","Merengue","Salsa","Thrash Metal","Anime","JPop","Synthpop"

};

static char MODES [][13]={"Stereo","Joint-Stereo","Dual-Channel","Mono"};

int iPow(int x, int y){return (int)(pow((double)x,(double) y));}

int str2int(char *text)
{
	int ret = 0;

	size_t i;
	for (i = 1; i <= strlen(text); i++)
	{
		if ((text[strlen(text) - i] > 57) || (text[strlen(text) - i] < 48))
		{
			hexchat_printf(ph, "invalid char in string: %i", (int) text[strlen(text) - i]);
			return 255;
		}

		ret += ((int) text[strlen(text) - i] - 48)*iPow(10, i - 1);
	}

	return ret;
}

static char *tagExtract(char *tag, int tagLen, char* info){
//if (DEBUG==1) putlog("extracting tag");
	int pos, len, i;
	pos=inStr(tag,tagLen,info);
//hexchat_printf(ph,"pos=%i",pos);
	if (pos==-1) return "";//NULL;
	//printf("position of %s = %i\n",info,pos);
	len=0;
	//for (i=pos;i<pos+10;i++)printf("tag[%i]=%i \n",i,tag[i]);
	for (i=0;i<4;i++) {
		len+=tag[pos+strlen(info)+i]*iPow(255,3-i);
	}
	//printf("Tag-Length: %i\n",len);
	if (strcmp("COMM",info)!=0) return substring(tag,pos+7+strlen(info),len-1);//11
	return substring(tag,pos+7+strlen(info),len-1);//11
	//char *ct=substring(tag,pos+7+strlen(info),len-1);//11
	//return substring(ct,strlen(ct)+1,len-1-strlen(ct)); //<-- do not understand, what i did here :(
	
}

struct tagInfo readID3V1(char *file){
//if (DEBUG==1) putlog("reading ID3V1");
	FILE *f;
	struct tagInfo ret;
	int res, i, c, val;
	char *tag;
	char *id;
	char *tmp;
	ret.artist=NULL;
	f=fopen(file,"rb");
	if (f==NULL){
       hexchat_print(ph,"file not found while trying to read id3v1");
       //if (DEBUG==1) putlog("file not found while trying to read id3v1");
       return ret;
    }
	//int offset=getSize(file)-128;
	res=fseek(f,-128,SEEK_END);
	if (res!=0) {printf("seek failed\n");fclose(f);return ret;}
	tag = (char*) malloc(sizeof(char)*129);
	//long int pos=ftell(f);
	//printf("position= %li\n",pos);
	for (i=0;i<128;i++) {
		c=fgetc(f);
		if (c==EOF) {hexchat_printf(ph,"read ID3V1 failed\n");fclose(f);free(tag);return ret;}
		tag[i]=(char)c;
	}
	fclose(f);
	//printf("tag readed: \n");
	id=substring(tag,0,3);
	//printf("header: %s\n",id);
	res=strcmp(id,"TAG");
	free(id);
	if (res!=0){hexchat_printf(ph,"no id3 v1 found\n");free(tag);return ret;}
	ret.title=subString(tag,3,30,1);
	ret.artist=subString(tag,33,30,1);
	ret.album=subString(tag,63,30,1);
	ret.comment=subString(tag,97,30,1);
	tmp=substring(tag,127,1);
	//ret.genre=substring(tag,127,1);
	
	val=(int)tmp[0];
	if (val<0)val+=256;
	//hexchat_printf(ph, "tmp[0]=%i (%i)",val,tmp[0]);
	if ((val<148)&&(val>=0)) 
       ret.genre=GENRES[val];//#############changed
	else {
         ret.genre="unknown";
         //hexchat_printf(ph, "tmp[0]=%i (%i)",val,tmp[0]);
    }
	//hexchat_printf(ph, "tmp: \"%s\" -> %i",tmp,tmp[0]);
	//hexchat_printf(ph,"genre \"%s\"",ret.genre);
	//if (DEBUG==1) putlog("id3v1 extracted");
	free(tmp);
	free(tag);
	return ret;
}

char *extractID3Genre(char *tag)
{
	if (tag[strlen(tag) - 1] == ')')
	{
		tag[strlen(tag) - 1] = 0;
		tag = &tag[1];
		return GENRES[str2int(tag)];
	}
	else
	{
		size_t i;
		for (i = 0; i < strlen(tag); i++)
		{
			if (tag[i] == ')')
			{
				tag = &tag[i] + 1;
				return tag;
			}
		}
	}

	return "[152] failed";
}

struct tagInfo readID3V2(char *file){
//if (DEBUG==1) putlog("reading id3v2");
	FILE *f;
	int i, c, len;
	char header[10];
	char *tag;
	struct tagInfo ret;

	f = fopen(file,"rb");
	//hexchat_printf(ph,"file :%s",file);
	if (f==NULL)
	{
       hexchat_print(ph,"file not found whilt trying to read ID3V2");
       //if (DEBUG==1)putlog("file not found while trying to read ID3V2");
       return ret;
    }

	ret.artist=NULL;
	for (i=0;i<10;i++){
        c=fgetc(f);
        if (c==EOF){
            fclose(f);
           //putlog("found eof while reading id3v2");
           return ret;
        }
        header[i]=(char)c;
    }
	if (strstr(header,"ID3")==header){
		//hexchat_printf(ph,"found id3v2\n");
		len=0;
		for (i=6;i<10;i++) len+=(int)header[i]*iPow(256,9-i);
		
		//char *tag=(char*)malloc(sizeof(char)*len);
		tag=(char*) calloc(len,sizeof(char)); //malloc(sizeof(char)*len);
		for (i=0;i<len;i++){c=fgetc(f);tag[i]=(char)c;}
//hexchat_printf(ph,"tag length: %i\n",len);
//hexchat_printf(ph,"tag: %s\n",tag);
		fclose(f);
		ret.comment=tagExtract(tag,len,"COMM");
//hexchat_printf(ph,"Comment: %s\n",ret.comment);
		ret.genre=tagExtract(tag,len,"TCON");
		//if (strcmp(ret.genre,"(127)")==0) ret.genre="unknown";
//hexchat_printf(ph, "ret.genre = %s",ret.genre);
		if ((ret.genre!=NULL)&&(ret.genre[0]=='(')) ret.genre=extractID3Genre(ret.genre);
//hexchat_printf(ph,"genre: %s\n",ret.genre);
		ret.title=tagExtract(tag,len,"TIT2");
//hexchat_printf(ph,"Title: %s\n",ret.title);
		ret.album=tagExtract(tag,len,"TALB");
//hexchat_printf(ph,"Album: %s\n",ret.album);
		ret.artist=tagExtract(tag,len,"TPE1");
//hexchat_printf(ph,"Artist: %s\n",ret.artist);
	}
	else{fclose(f);printf("no id3v2 tag found\n"); return ret;}
	//printf("id2v2 done\n");
	//if (DEBUG==1) putlog("id3v2 readed");
	return ret;
}

struct tagInfo readHeader(char *file){
//if (DEBUG==1) putlog("reading header");
	FILE *f;
	//int buffer[5120];
	int versionB, layerB, bitrateB, freqB, modeB;
	int header[4];
	int count=0;
	int cc=0;
	struct tagInfo info;
	info.artist=NULL;

	f = fopen(file,"rb");
	if (f==NULL)
	{
       hexchat_print(ph,"file not found while trying to read mp3 header");
       //if (DEBUG==1) putlog("file not found while trying to read mp3 header");
       return info;
    }
	//struct tagInfo tagv2
	
	info=readID3V2(file);
	//struct tagInfo tagv1;//=readID3V1(file);
	//if 	(tagv2.artist!=NULL){info=tagv2;}
	//else {
	if (info.artist==NULL){
		//printf("searching for id3v1\n");
		//tagv1=readID3V1(file);
		info=readID3V1(file); //#####################
	}
	/*
	if (tagv1.artist!=NULL){
		//printf("Artist: %s\nTitle: %s\nAlbum: %s\nComment: %s\nGenre: %s\n",tagv1.artist,tagv1.title,tagv1.album,tagv1.comment,tagv1.genre);
		info=tagv1;
	}
	*/
	while ((count<5120)&&(cc!=EOF)&&(cc!=255)) {cc=fgetc(f);count++;}
	if ((cc==EOF)||(count==5119)) printf("no header found\n");
	else {
		//printf("located header at %i\n",count);
		header[0]=255;
		for (count=1;count<4;count++){
			header[count]=fgetc(f);
			//printf("header[%i]=%i\n",count,header[count]);
		}
		versionB=(header[1]&8)>>3;
		layerB=(header[1]&6)>>1;
		bitrateB=(header[2]&240)>>4; //4
		freqB=(header[2]&12)>>2;//2
		modeB=(header[3]&192)>>6;//6
		//printf("Mpeg: %i\nLayer: %i\nBitrate: %i\nFreq: %i\nMode: %i\n",versionB, layerB, bitrateB, freqB, modeB);
		//int Bitrate=RATES[versionB][layerB-1][bitrateB];
		//int Freq=FREQS[versionB][freqB];
		info.bitrate=RATES[versionB][layerB-1][bitrateB];
		info.freq=FREQS[versionB][freqB];
		info.mode=modeB;	
	}
	fclose(f);
	//if (DEBUG==1) putlog("header readed");
	return info;
}
/*
static void printMp3Info(char *file){
	//printf("\nScanning Mp3-File for Informations: %s\n",file);
	//printf("size:\t%10d byte\n",getSize(file));
	struct tagInfo info =readHeader(file);
	printf("%s | %10d",file,getSize(file));
	if  (info.bitrate>0){
		//printf("Bitrate: %i\nFreq: %i\nMode: %s\n",info.bitrate,info.freq,MODES[info.mode]);
		printf(" | %i kbps | %i kHz | %s",info.bitrate,info.freq,MODES[info.mode]);
		//if (info.artist!=NULL) printf("\nArtist: %s\nTitle: %s\nAlbum: %s\nComment: %s\nGenre: %s\n",info.artist,info.title,info.album,info.comment,info.genre);
		if (info.artist!=NULL) {
			printf("| %s | %s | %s | %s | %s",info.artist,info.title,info.album,info.comment,info.genre);
			//printf("| %s ",info.title);//,info.title,info.album,info.comment,info.genre
		}
	}
	printf("\n");
	
}
*/
