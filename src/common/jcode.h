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

#ifndef __JCODE_H__
#define __JCODE_H__

enum {
    JCODE_ASCII,
    JCODE_EUC,
    JCODE_JIS,
    JCODE_SJIS,
    EUCORSJIS };

char *kanji_conv(char *str, const char *dstset, const char *srcset);
char *kanji_conv_auto(char *str, const char *dstset);
char *kanji_conv_to_locale(char *str);

#endif
