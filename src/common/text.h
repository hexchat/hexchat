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

#include <time.h>
#include "textenums.h"

#ifndef HEXCHAT_TEXT_H
#define HEXCHAT_TEXT_H

/* timestamp is non-zero if we are using server-time */
#define EMIT_SIGNAL_TIMESTAMP(i, sess, a, b, c, d, e, timestamp) \
	text_emit(i, sess, a, b, c, d, timestamp)
#define EMIT_SIGNAL(i, sess, a, b, c, d, e) \
	text_emit(i, sess, a, b, c, d, 0)

struct text_event
{
	char *name;
	char * const *help;
	int num_args;
	char *def;
};

void scrollback_close (session *sess);
void scrollback_load (session *sess);

int text_word_check (char *word, int len);
void PrintText (session *sess, char *text);
void PrintTextTimeStamp (session *sess, char *text, time_t timestamp);
void PrintTextf (session *sess, const char *format, ...) G_GNUC_PRINTF (2, 3);
void PrintTextTimeStampf (session *sess, time_t timestamp, const char *format, ...) G_GNUC_PRINTF (3, 4);
void log_close (session *sess);
void log_open_or_close (session *sess);
void load_text_events (void);
void pevent_save (char *fn);
int pevt_build_string (const char *input, char **output, int *max_arg);
int pevent_load (char *filename);
void pevent_make_pntevts (void);
int text_color_of (char *name);
void text_emit (int index, session *sess, char *a, char *b, char *c, char *d,
		time_t timestamp);
int text_emit_by_name (char *name, session *sess, time_t timestamp,
					   char *a, char *b, char *c, char *d);
gchar *text_convert_invalid (const gchar* text, gssize len, GIConv converter, const gchar *fallback, gsize *len_out);
gchar *text_fixup_invalid_utf8 (const gchar* text, gssize len, gsize *len_out);
int get_stamp_str (char *fmt, time_t tim, char **ret);
void format_event (session *sess, int index, char **args, char *o, gsize sizeofo, unsigned int stripcolor_args);
char *text_find_format_string (char *name);

extern const gchar* unicode_fallback_string;
extern const gchar* arbitrary_encoding_fallback_string;

void sound_play (const char *file, gboolean quiet);
void sound_play_event (int i);
void sound_beep (session *);
void sound_load (void);
void sound_save (void);

#endif
