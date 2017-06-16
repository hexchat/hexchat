/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* cfgfiles.h */

#ifndef HEXCHAT_CFGFILES_H
#define HEXCHAT_CFGFILES_H

#include "hexchat.h"

#define LANGUAGES_LENGTH 53

extern char *xdir;
extern const char * const languages[LANGUAGES_LENGTH];

char *cfg_get_str (char *cfg, const char *var, char *dest, int dest_len);
int cfg_get_bool (char *var);
int cfg_get_int_with_result (char *cfg, char *var, int *result);
int cfg_get_int (char *cfg, char *var);
int cfg_put_int (int fh, int value, char *var);
int cfg_get_color (char *cfg, char *var, guint16 *r, guint16 *g, guint16 *b);
int cfg_put_color (int fh, guint16 r, guint16 g, guint16 b, char *var);
char *get_xdir (void);
int check_config_dir (void);
void load_default_config (void);
int make_config_dirs (void);
int make_dcc_dirs (void);
int load_config (void);
int save_config (void);
void list_free (GSList ** list);
void list_loadconf (char *file, GSList ** list, char *defaultconf);
int list_delentry (GSList ** list, char *name);
void list_addentry (GSList ** list, char *cmd, char *name);
int cmd_set (session *sess, char *tbuf, char *word[], char *word_eol[]);
int hexchat_open_file (const char *file, int flags, int mode, int xof_flags);
FILE *hexchat_fopen_file (const char *file, const char *mode, int xof_flags);

#define XOF_DOMODE 1
#define XOF_FULLPATH 2

#define STRUCT_OFFSET_STR(type,field) \
( (unsigned int) (((char *) (&(((type *) NULL)->field)))- ((char *) NULL)) )

#define STRUCT_OFFSET_INT(type,field) \
( (unsigned int) (((int *) (&(((type *) NULL)->field)))- ((int *) NULL)) )

#define P_OFFSET(field) STRUCT_OFFSET_STR(struct hexchatprefs, field),sizeof(prefs.field)
#define P_OFFSETNL(field) STRUCT_OFFSET_STR(struct hexchatprefs, field)
#define P_OFFINT(field) STRUCT_OFFSET_INT(struct hexchatprefs, field),0
#define P_OFFINTNL(field) STRUCT_OFFSET_INT(struct hexchatprefs, field)

struct prefs
{
	char *name;
	unsigned short offset;
	unsigned short len;
	unsigned short type;
	/*
	 * an optional function which will be called after the preference value has
	 * been updated.
	 */
	void (*after_update)(void);
};

#define TYPE_STR 0
#define TYPE_INT 1
#define TYPE_BOOL 2

#define HEXCHAT_SOUND_DIR "sounds"

#endif
