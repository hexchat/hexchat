 /*
  *  jcode.c  code detect and convert with iconv(3)
  *
  *  Copyright (c) 2001 Takuo Kitame <kitame@debian.org>
  *  All rights reserved.
  *
  *  You can use, modify and/or distribute this program under any license which
  *  is compliant with DFSG
  *
  *  about DFSG <URL:http://www.debian.org/social_contract#guidelines>
  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <iconv.h>
#include "jcode.h"

#define ESC          0x1b
#define SS2          0x8e

#define JCODE_LOCALE_EUC   "ja", "ja_JP", "ja_JP.ujis", "ja_JP.EUC", "ja_JP.eucJP"
#define JCODE_LOCALE_JIS   "ja_JP.JIS", "ja_JP.jis", "ja_JP.iso-2022-jp"
#define JCODE_LOCALE_SJIS  "ja_JP.SJIS", "ja_JP.sjis"

/****************************************************************************/
/* Japanese string code detector */
/****************************************************************************/
static int 
detect_kanji(unsigned char *str)
{
    int expected = JCODE_ASCII;
    register int c;
    int c1, c2;
    int euc_c = 0, sjis_c = 0;
    unsigned char *ptr = str;
    
    while((c = (int)*ptr)!= '\0') {
        if(c == ESC) {
            if((c = (int)*(++ptr)) == '\0')
                break;
            if(c == '$') {
                if((c = (int)*(++ptr)) == '\0')
                    break;
                if(c == 'B' || c == '@')
                    return JCODE_JIS;
            }
            ptr++;
            continue;
        }
        if((c >= 0x81 && c <= 0x8d) || (c >= 0x8f && c <= 0x9f))
            return JCODE_SJIS;
        
        if(c == SS2) {
            if((c = (int)*(++ptr)) == '\0')
                break;
            if((c >= 0x40 && c <= 0x7e) ||
               (c >= 0x80 && c <= 0xa0) || 
               (c >= 0xe0 && c <= 0xfc))
                return JCODE_SJIS;
            if(c >= 0xa1 && c <= 0xdf)
                break;
            
            ptr++;
            continue;
        }        
        if(c >= 0xa1 && c <= 0xdf) {
            if((c = (int)*(++ptr)) == '\0')
                break;
            
            if (c >= 0xe0 && c <= 0xfe)
                return JCODE_EUC;
            if (c >= 0xa1 && c <= 0xdf) {
                expected = EUCORSJIS;
                ptr++;
                continue;
            }
#if 1
            if(c == 0xa0 || (0xe0 <= c && c <= 0xfe))
                return JCODE_EUC;
            else {
                expected = EUCORSJIS;
                ptr++;
                continue;
            }
#else
            if(c <= 0x9f)
                return JCODE_SJIS;
            if(c >= 0xf0 && c <= 0xfe)
                return JCODE_EUC;
#endif
            
            if(c >= 0xe0 && c <= 0xef) {
                expected = EUCORSJIS;
                while(c >= 0x40) {
                    if(c >= 0x81) {
                        if(c <= 0x8d || (c >= 0x8f && c <= 0x9f))
                            return JCODE_SJIS;
                        else if(c >= 0xfd && c <= 0xfe) {
                            return JCODE_EUC;
                        }
                    }
                    if((c = (int)*(++ptr)) == '\0')
                        break;
                }
                ptr++;
                continue;
            }
            
            if(c >= 0xe0 && c <= 0xef) {
                if((c = (int)*(++ptr)) == '\0')
                    break;
                if((c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xa0))
                    return JCODE_SJIS;
                if(c >= 0xfd && c <= 0xfe)
                    return JCODE_EUC;
                if(c >= 0xa1 && c <= 0xfc)
                    expected = EUCORSJIS;
            }
        }
#if 1
        if (0xf0 <= c && c <= 0xfe)
            return JCODE_EUC;
#endif
        ptr++;
    }

   ptr = str;
   c2 = 0;
   while((c1 = (int)*ptr++) != '\0') {
       if(((c2 >  0x80 && c2 < 0xa0) || (c2 >= 0xe0 && c2 < 0xfd)) &&
          ((c1 >= 0x40 && c1 < 0x7f) || (c1 >= 0x80 && c1 < 0xfd)))
           sjis_c++, c1 = *ptr++;
       c2 = c1;
   }

/*
   if(sjis_c == 0)
       expected = JCODE_EUC;
   else {
*/
   {
       ptr = str, c2 = 0;
       while((c1 = (int)*ptr++) != '\0') {
	     if((c2 > 0xa0  && c2 < 0xff) &&
            (c1 > 0xa0  && c1 < 0xff))
             euc_c++, c1 = *ptr++;
	     c2 = c1;
       }
       if(sjis_c > euc_c)
           expected = JCODE_SJIS;
       else if (euc_c > 0)
           expected = JCODE_EUC;
       else 
           expected = JCODE_ASCII;
   }
   return expected;
}

static int
int_detect_JCode(char *str)
{
    int detected;

    if(!str)
        return 0;

    detected = detect_kanji((unsigned char *)str);
    
    if(detected == JCODE_ASCII)
        return JCODE_ASCII;
    
    switch(detected) {
    case JCODE_EUC:
        return JCODE_EUC;
        break;
    case JCODE_JIS:
        return JCODE_JIS;
        break;
    case JCODE_SJIS:  
        return JCODE_SJIS;
        break;
    default:
        return JCODE_ASCII;
        break;
    }
    
    /* not reached */
    return 0;
}

#if 0
static const char *
detect_JCode(char *str)
{
    int detected;

    if(!str)
        return NULL;

    detected = detect_kanji((unsigned char *)str);
    
    if(detected == JCODE_ASCII)
        return "ASCII";

    switch(detected) {
    case JCODE_EUC:
        return "EUC-JP";
        break;
    case JCODE_JIS:
        return "ISO-2022-JP";
        break;
    case JCODE_SJIS:  
        return "SJIS";
        break;
    default:
        return "ASCII";
        break;
    }
    
    /* not reached */
    return 0;
}
#endif

char *
kanji_conv_auto(char *str, const char *dstset)
{
    unsigned char *buf, *ret;
    iconv_t cd;
    size_t insize = 0;
    size_t outsize = 0;
    size_t nconv = 0;
    char *inptr;
    char *outptr;
    char *srcset;

    if(!str)
        return NULL;
    
    switch (int_detect_JCode(str)) {
    case JCODE_EUC:
        srcset = "EUC-JP";
        break;
    case JCODE_JIS:
        srcset = "ISO-2022-JP";
        break;
    case JCODE_SJIS:
        srcset = "SJIS";
        break;
    default:
        /* printf("failed!! \n"); */
        return strdup(str);
        break;
    }
#ifdef DEBUG
    printf("to %s from %s (%s)\n", dstset, srcset, str);
#endif
    buf = (unsigned char *)malloc(strlen(str)* 4 + 1);
    if(!buf)
        return NULL;
    
    insize = strlen(str);
    inptr = str;
    outsize = strlen(str) * 4 ;
    outptr = buf;
    
    cd = iconv_open(dstset, srcset);
    if(cd == (iconv_t) -1) {
        if(errno == EINVAL)
            return strdup(str);
    }
    
    nconv = iconv(cd, (const char **)&inptr, &insize, &outptr, &outsize);
    if(nconv == (size_t) -1) {
        if (errno == EINVAL)
            memmove (buf, inptr, insize);
    } else
        iconv(cd, NULL, NULL, &outptr, &outsize);
    
    *outptr = '\0';
    iconv_close(cd);
    
    ret = strdup(buf);
    free(buf);

    return ret;
}

/* convert to system locale code */
char *
kanji_conv_to_locale(char *str)
{
   static char *jpcode = NULL;
   static char *locale_euc[]  = { JCODE_LOCALE_EUC, NULL };
   static char *locale_jis[]  = { JCODE_LOCALE_JIS, NULL };
   static char *locale_sjis[] = { JCODE_LOCALE_SJIS, NULL };

   static struct LOCALE_TABLE {
       char *code;
       char **name_list;
   } locale_table[] = { 
       {"EUC-JP", locale_euc},
       {"ISO-2022-JP", locale_sjis},
       {"SJIS", locale_sjis}
   };

   if(!str)
       return NULL;

   if(jpcode == NULL) {
       char *ctype = setlocale(LC_CTYPE, "");
       int i, j;
       for( j=0; jpcode == NULL && 
                j < sizeof(locale_table)/sizeof(struct LOCALE_TABLE); j++ ) {
           char **name = locale_table[j].name_list;
           for( i=0; name[i]; i++ )
               if (strcasecmp(ctype, name[i]) == 0) {
                   jpcode = locale_table[j].code;
                   break;
               }
       }
       if(jpcode == NULL)
           jpcode = locale_table[1].code;
   }
   
   return kanji_conv_auto(str, jpcode);
}

char *
kanji_conv(char *str, const char *dstset, const char *srcset)
{
    unsigned char *buf, *ret;
    iconv_t cd;
    size_t insize = 0;
    size_t outsize = 0;
    size_t nconv = 0;
    char *inptr;
    char *outptr;
    
    if(!str)
        return NULL;
    
    buf = (unsigned char *)malloc(strlen(str) * 4 + 1);
    if(!buf)
        return NULL;
    
    insize = strlen(str);
    inptr = str;
    outsize = strlen(str) * 4 ;
    outptr = buf;
    
    cd = iconv_open (dstset, srcset);
    if(cd == (iconv_t) -1) {
        if(errno == EINVAL)
            return strdup(str);
    }
    
    nconv = iconv (cd, (const char **)&inptr, &insize, &outptr, &outsize);
    if (nconv == (size_t) -1) {
        if(errno == EINVAL)
            memmove (buf, inptr, insize);
    } else
       iconv (cd, NULL, NULL, &outptr, &outsize);
    
    *outptr = '\0';
    iconv_close(cd);
    
    ret = strdup(buf);
    free(buf);
    
    return ret;
}
