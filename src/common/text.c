/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "hexchat.h"
#include "cfgfiles.h"
#include "chanopt.h"
#include "plugin.h"
#include "fe.h"
#include "server.h"
#include "util.h"
#include "outbound.h"
#include "hexchatc.h"
#include "text.h"
#include "typedef.h"
#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_LIBCANBERRA
#include <canberra.h>
#endif

const gchar* unicode_fallback_string = "\357\277\275"; /* The Unicode replacement character 0xFFFD */
const gchar* arbitrary_encoding_fallback_string = "?";

struct pevt_stage1
{
	int len;
	char *data;
	struct pevt_stage1 *next;
};

#ifdef USE_LIBCANBERRA
static ca_context *ca_con;
#endif

#define SCROLLBACK_MAX 32000

static void mkdir_p (char *filename);
static char *log_create_filename (char *channame);

static char *
scrollback_get_filename (session *sess)
{
	char *net, *chan, *buf, *ret = NULL;

	net = server_get_network (sess->server, FALSE);
	if (!net)
		return NULL;

	net = log_create_filename (net);
	buf = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "scrollback" G_DIR_SEPARATOR_S "%s" G_DIR_SEPARATOR_S "%s.txt", get_xdir (), net, "");
	mkdir_p (buf);
	g_free (buf);

	chan = log_create_filename (sess->channel);
	if (chan[0])
		buf = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "scrollback" G_DIR_SEPARATOR_S "%s" G_DIR_SEPARATOR_S "%s.txt", get_xdir (), net, chan);
	else
		buf = NULL;
	g_free (chan);
	g_free (net);

	if (buf)
	{
		ret = g_filename_from_utf8 (buf, -1, NULL, NULL, NULL);
		g_free (buf);
	}

	return ret;
}

void
scrollback_close (session *sess)
{
	g_clear_object (&sess->scrollfile);
}

/* shrink the file to roughly prefs.hex_text_max_lines */

static void
scrollback_shrink (session *sess)
{
	char *buf, *p;
	gsize len;
	gint offset, lines = 0;
	const gint max_lines = MIN(prefs.hex_text_max_lines, SCROLLBACK_MAX);

	if (!g_file_load_contents (sess->scrollfile, NULL, &buf, &len, NULL, NULL))
		return;

	/* count all lines */
	p = buf;
	while (p != buf + len)
	{
		if (*p == '\n')
			lines++;
		p++;
	}

	offset = lines - max_lines;

	/* now just go back to where we want to start the file */
	p = buf;
	lines = 0;
	while (p != buf + len)
	{
		if (*p == '\n')
		{
			lines++;
			if (lines == offset)
			{
				p++;
				break;
			}
		}
		p++;
	}

	if (g_file_replace_contents (sess->scrollfile, p, strlen(p), NULL, FALSE,
							G_FILE_CREATE_PRIVATE, NULL, NULL, NULL))
		sess->scrollwritten = lines;

	g_free (buf);
}

static void
scrollback_save (session *sess, char *text, time_t stamp)
{
	GOutputStream *ostream;
	char *buf;

	if (sess->type == SESS_SERVER && prefs.hex_gui_tab_server == 1)
		return;

	if (sess->text_scrollback == SET_DEFAULT)
	{
		if (!prefs.hex_text_replay)
			return;
	}
	else
	{
		if (sess->text_scrollback != SET_ON)
			return;
	}

	if (!sess->scrollfile)
	{
		if ((buf = scrollback_get_filename (sess)) == NULL)
			return;

		sess->scrollfile = g_file_new_for_path (buf);
		g_free (buf);
	}
	else
	{
		/* Users can delete the folder after it's created... */
		GFile *parent = g_file_get_parent (sess->scrollfile);
		g_file_make_directory_with_parents (parent, NULL, NULL);
		g_object_unref (parent);
	}

	ostream = G_OUTPUT_STREAM(g_file_append_to (sess->scrollfile, G_FILE_CREATE_PRIVATE, NULL, NULL));
	if (!ostream)
		return;

	if (!stamp)
		stamp = time(0);
	if (sizeof (stamp) == 4)	/* gcc will optimize one of these out */
		buf = g_strdup_printf ("T %d ", (int) stamp);
	else
		buf = g_strdup_printf ("T %" G_GINT64_FORMAT " ", (gint64)stamp);

	g_output_stream_write (ostream, buf, strlen (buf), NULL, NULL);
	g_output_stream_write (ostream, text, strlen (text), NULL, NULL);
	if (!g_str_has_suffix (text, "\n"))
		g_output_stream_write (ostream, "\n", 1, NULL, NULL);

	g_free (buf);
	g_object_unref (ostream);

	sess->scrollwritten++;

	if ((sess->scrollwritten > prefs.hex_text_max_lines && prefs.hex_text_max_lines > 0) ||
       sess->scrollwritten > SCROLLBACK_MAX)
		scrollback_shrink (sess);
}

void
scrollback_load (session *sess)
{
	GInputStream *stream;
	GDataInputStream *istream;
	gchar *buf, *text;
	gint lines = 0;
	time_t stamp = 0;

	if (sess->text_scrollback == SET_DEFAULT)
	{
		if (!prefs.hex_text_replay)
			return;
	}
	else
	{
		if (sess->text_scrollback != SET_ON)
			return;
	}

	if (!sess->scrollfile)
	{
		if ((buf = scrollback_get_filename (sess)) == NULL)
			return;

		sess->scrollfile = g_file_new_for_path (buf);
		g_free (buf);
	}

	stream = G_INPUT_STREAM(g_file_read (sess->scrollfile, NULL, NULL));
	if (!stream)
		return;

	istream = g_data_input_stream_new (stream);
	/*
	 * This is to avoid any issues moving between windows/unix
	 * but the docs mention an invalid \r without a following \n
	 * can lock up the program... (Our write() always adds \n)
	 */
	g_data_input_stream_set_newline_type (istream, G_DATA_STREAM_NEWLINE_TYPE_ANY);
	g_object_unref (stream);

	while (1)
	{
		GError *err = NULL;
		gsize n_bytes;

		buf = g_data_input_stream_read_line_utf8 (istream, &n_bytes, NULL, &err);

		if (!err && buf)
		{
			/*
			 * Some scrollback lines have three blanks after the timestamp and a newline
			 * Some have only one blank and a newline
			 * Some don't even have a timestamp
			 * Some don't have any text at all
			 */
			if (buf[0] == 'T' && buf[1] == ' ')
			{
				if (sizeof (time_t) == 4)
					stamp = strtoul (buf + 2, NULL, 10);
				else
					stamp = g_ascii_strtoull (buf + 2, NULL, 10); /* in case time_t is 64 bits */

				if (G_UNLIKELY(stamp == 0))
				{
					g_warning ("Invalid timestamp in scrollback file");
					continue;
				}

				text = strchr (buf + 3, ' ');
				if (text && text[1])
				{
					if (prefs.hex_text_stripcolor_replay)
					{
						text = strip_color (text + 1, -1, STRIP_COLOR);
					}

					fe_print_text (sess, text, stamp, TRUE);

					if (prefs.hex_text_stripcolor_replay)
					{
						g_free (text);
					}
				}
				else
				{
					fe_print_text (sess, "  ", stamp, TRUE);
				}
			}
			else
			{
				if (strlen (buf))
					fe_print_text (sess, buf, 0, TRUE);
				else
					fe_print_text (sess, "  ", 0, TRUE);
			}
			lines++;

			g_free (buf);
		}
		else if (err)
		{
			/* If its only an encoding error it may be specific to the line */
			if (g_error_matches (err, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE))
			{
				g_warning ("Invalid utf8 in scrollback file");
				g_clear_error (&err);
				continue;
			}

			/* For general errors just give up */
			g_clear_error (&err);
			break;
		}
		else /* No new line */
		{
			break;
		}
	}

	g_object_unref (istream);

	sess->scrollwritten = lines;

	if (lines)
	{
		text = ctime (&stamp);
		buf = g_strdup_printf ("\n*\t%s %s\n", _("Loaded log from"), text);
		fe_print_text (sess, buf, 0, TRUE);
		g_free (buf);
		/*EMIT_SIGNAL (XP_TE_GENMSG, sess, "*", buf, NULL, NULL, NULL, 0);*/
	}
}

void
log_close (session *sess)
{
	char obuf[512];
	time_t currenttime;

	if (sess->logfd != -1)
	{
		currenttime = time (NULL);
		write (sess->logfd, obuf,
			 g_snprintf (obuf, sizeof (obuf) - 1, _("**** ENDING LOGGING AT %s\n"),
						  ctime (&currenttime)));
		close (sess->logfd);
		sess->logfd = -1;
	}
}

/*
 * filename should be in utf8 encoding and will be
 * converted to filesystem encoding automatically.
 */
static void
mkdir_p (char *filename)
{
	char *dirname, *dirname_fs;
	GError *err = NULL;
	
	dirname = g_path_get_dirname (filename);
	dirname_fs = g_filename_from_utf8 (dirname, -1, NULL, NULL, &err);
	if (!dirname_fs)
	{
		g_warning ("%s", err->message);
		g_error_free (err);
		g_free (dirname);
		return;
	}

	g_mkdir_with_parents (dirname_fs, 0700);

	g_free (dirname);
	g_free (dirname_fs);
}

static char *
log_create_filename (char *channame)
{
	char *tmp, *ret;
	int mbl;

	ret = tmp = g_strdup (channame);
	while (*tmp)
	{
		mbl = g_utf8_skip[((unsigned char *)tmp)[0]];
		if (mbl == 1)
		{
#ifndef WIN32
			*tmp = rfc_tolower (*tmp);
			if (*tmp == '/')
#else
			/* win32 can't handle filenames with \|/><:"*? characters */
			if (*tmp == '\\' || *tmp == '|' || *tmp == '/' ||
				 *tmp == '>'  || *tmp == '<' || *tmp == ':' ||
				 *tmp == '\"' || *tmp == '*' || *tmp == '?')
#endif
				*tmp = '_';
		}
		tmp += mbl;
	}

	return ret;
}

/* like strcpy, but % turns into %% */

static char *
log_escape_strcpy (char *dest, char *src, char *end)
{
	while (*src)
	{
		*dest = *src;
		if (dest + 1 == end)
			break;
		dest++;
		src++;

		if (*src == '%')
		{
			if (dest + 1 == end)
				break;
			dest[0] = '%';
			dest++;
		}
	}

	dest[0] = 0;
	return dest - 1;
}

/* substitutes %c %n %s into buffer */

static void
log_insert_vars (char *buf, int bufsize, char *fmt, char *c, char *n, char *s)
{
	char *end = buf + bufsize;

	while (1)
	{
		switch (fmt[0])
		{
		case 0:
			buf[0] = 0;
			return;

		case '%':
			fmt++;
			switch (fmt[0])
			{
			case 'c':
				buf = log_escape_strcpy (buf, c, end);
				break;
			case 'n':
				buf = log_escape_strcpy (buf, n, end);
				break;
			case 's':
				buf = log_escape_strcpy (buf, s, end);
				break;
			default:
				buf[0] = '%';
				buf++;
				buf[0] = fmt[0];
				break;
			}
			break;

		default:
			buf[0] = fmt[0];
		}
		fmt++;
		buf++;
		/* doesn't fit? */
		if (buf == end)
		{
			buf[-1] = 0;
			return;
		}
	}
}

static char *
log_create_pathname (char *servname, char *channame, char *netname)
{
	char fname[384];
	char fnametime[384];
	time_t now;

	if (!netname)
	{
		netname = g_strdup ("NETWORK");
	}
	else
	{
		netname = log_create_filename (netname);
	}

	/* first, everything is in UTF-8 */
	if (!rfc_casecmp (channame, servname))
	{
		channame = g_strdup ("server");
	}
	else
	{
		channame = log_create_filename (channame);
	}

	servname = log_create_filename (servname);

	log_insert_vars (fname, sizeof (fname), prefs.hex_irc_logmask, channame, netname, servname);
	g_free (channame);
	g_free (netname);
	g_free (servname);

	/* insert time/date */
	now = time (NULL);
	strftime_utf8 (fnametime, sizeof (fnametime), fname, now);

	/* If one uses log mask variables, such as "%c/...", %c will be empty upon
	 * connecting since there's no channel name yet, so we have to make sure
	 * we won't try to write to the FS root. */
	if (g_path_is_absolute (prefs.hex_irc_logmask))
	{
		g_snprintf (fname, sizeof (fname), "%s", fnametime);
	}
	else	/* relative path */
	{
		g_snprintf (fname, sizeof (fname), "%s" G_DIR_SEPARATOR_S "logs" G_DIR_SEPARATOR_S "%s", get_xdir (), fnametime);
	}

	/* create all the subdirectories */
	mkdir_p (fname);

	return g_strdup (fname);
}

static int
log_open_file (char *servname, char *channame, char *netname)
{
	char buf[512];
	int fd;
	char *file;
	time_t currenttime;

	file = log_create_pathname (servname, channame, netname);
	if (!file)
		return -1;

	fd = g_open (file, O_CREAT | O_APPEND | O_WRONLY | OFLAGS, 0644);
	g_free (file);

	if (fd == -1)
		return -1;
	currenttime = time (NULL);
	write (fd, buf,
			 g_snprintf (buf, sizeof (buf), _("**** BEGIN LOGGING AT %s\n"),
						  ctime (&currenttime)));

	return fd;
}

static void
log_open (session *sess)
{
	static gboolean log_error = FALSE;

	log_close (sess);
	sess->logfd = log_open_file (sess->server->servername, sess->channel,
										  server_get_network (sess->server, FALSE));

	if (!log_error && sess->logfd == -1)
	{
		char *filename = log_create_pathname (sess->server->servername, sess->channel, server_get_network (sess->server, FALSE));
		char *message = g_strdup_printf (_("* Can't open log file(s) for writing. Check the\npermissions on %s"), filename);

		g_free (filename);

		fe_message (message, FE_MSG_WAIT | FE_MSG_ERROR);

		g_free (message);

		log_error = TRUE;
	}
}

void
log_open_or_close (session *sess)
{
	if (sess->text_logging == SET_DEFAULT)
	{
		if (prefs.hex_irc_logging)
			log_open (sess);
		else
			log_close (sess);
	}
	else
	{
		if (sess->text_logging)
			log_open (sess);
		else
			log_close (sess);
	}
}

int
get_stamp_str (char *fmt, time_t tim, char **ret)
{
	char dest[128];
	gsize len_locale;
	gsize len_utf8;

	/* strftime requires the format string to be in locale encoding. */
	fmt = g_locale_from_utf8 (fmt, -1, NULL, NULL, NULL);

	len_locale = strftime_validated (dest, sizeof (dest), fmt, localtime (&tim));

	g_free (fmt);

	if (len_locale == 0)
	{
		return 0;
	}

	*ret = g_locale_to_utf8 (dest, len_locale, NULL, &len_utf8, NULL);
	if (*ret == NULL)
	{
		return 0;
	}

	return len_utf8;
}

static void
log_write (session *sess, char *text, time_t ts)
{
	char *temp;
	char *stamp;
	char *file;
	int len;

	if (sess->text_logging == SET_DEFAULT)
	{
		if (!prefs.hex_irc_logging)
			return;
	}
	else
	{
		if (sess->text_logging != SET_ON)
			return;
	}

	if (sess->logfd == -1)
	{
		log_open (sess);
	}

	/* change to a different log file? */
	file = log_create_pathname (sess->server->servername, sess->channel, server_get_network (sess->server, FALSE));
	if (file)
	{
		if (g_access (file, F_OK) != 0)
		{
			if (sess->logfd != -1)
			{
				close (sess->logfd);
			}

			sess->logfd = log_open_file (sess->server->servername, sess->channel, server_get_network (sess->server, FALSE));
		}

		g_free (file);
	}

	if (sess->logfd == -1)
	{
		return;
	}

	if (prefs.hex_stamp_log)
	{
		if (!ts) ts = time(0);
		len = get_stamp_str (prefs.hex_stamp_log_format, ts, &stamp);
		if (len)
		{
			write (sess->logfd, stamp, len);
			g_free (stamp);
		}
	}

	temp = strip_color (text, -1, STRIP_ALL);
	len = strlen (temp);
	write (sess->logfd, temp, len);
	/* lots of scripts/plugins print without a \n at the end */
	if (temp[len - 1] != '\n')
		write (sess->logfd, "\n", 1);	/* emulate what xtext would display */
	g_free (temp);
}

/**
 * Converts a given string using the given iconv converter. This is similar to g_convert_with_fallback, except that it is tolerant of sequences in
 * the original input that are invalid even in from_encoding. g_convert_with_fallback fails for such text, whereas this function replaces such a
 * sequence with the fallback string.
 *
 * If len is -1, strlen(text) is used to calculate the length. Do not pass -1 if text is supposed to contain \0 bytes, such as if from_encoding is a
 * multi-byte encoding like UTF-16.
 */
gchar *
text_convert_invalid (const gchar* text, gssize len, GIConv converter, const gchar *fallback, gsize *len_out)
{
	gchar *result_part;
	gsize result_part_len;
	const gchar *end;
	gsize invalid_start_pos;
	GString *result;
	const gchar *current_start;

	if (len == -1)
	{
		len = strlen (text);
	}

	end = text + len;

	/* Find the first position of an invalid sequence. */
	result_part = g_convert_with_iconv (text, len, converter, &invalid_start_pos, &result_part_len, NULL);
	g_iconv (converter, NULL, NULL, NULL, NULL);

	if (result_part != NULL)
	{
		/* All text converted successfully on the first try. Return it. */

		if (len_out != NULL)
		{
			*len_out = result_part_len;
		}

		return result_part;
	}

	/* One or more invalid sequences exist that need to be replaced with the fallback. */

	result = g_string_sized_new (len);
	current_start = text;

	for (;;)
	{
		g_assert (current_start + invalid_start_pos < end);

		/* Convert everything before the position of the invalid sequence. It should be successful.
		 * But iconv may not convert everything till invalid_start_pos since the last few bytes may be part of a shift sequence.
		 * So get the new bytes_read and use it as the actual invalid_start_pos to handle this.
		 *
		 * See https://github.com/hexchat/hexchat/issues/1758
		 */
		result_part = g_convert_with_iconv (current_start, invalid_start_pos, converter, &invalid_start_pos, &result_part_len, NULL);
		g_iconv (converter, NULL, NULL, NULL, NULL);

		g_assert (result_part != NULL);
		g_string_append_len (result, result_part, result_part_len);
		g_free (result_part);

		/* Append the fallback */
		g_string_append (result, fallback);

		/* Now try converting everything after the invalid sequence. */
		current_start += invalid_start_pos + 1;

		result_part = g_convert_with_iconv (current_start, end - current_start, converter, &invalid_start_pos, &result_part_len, NULL);
		g_iconv (converter, NULL, NULL, NULL, NULL);

		if (result_part != NULL)
		{
			/* The rest of the text converted successfully. Append it and return the whole converted text. */

			g_string_append_len (result, result_part, result_part_len);
			g_free (result_part);

			if (len_out != NULL)
			{
				*len_out = result->len;
			}

			return g_string_free (result, FALSE);
		}

		/* The rest of the text didn't convert successfully. invalid_start_pos has the position of the next invalid sequence. */
	}
}

/**
 * Replaces any invalid UTF-8 in the given text with the unicode replacement character.
 */
gchar *
text_fixup_invalid_utf8 (const gchar* text, gssize len, gsize *len_out)
{
	static GIConv utf8_fixup_converter = NULL;
	if (utf8_fixup_converter == NULL)
	{
		utf8_fixup_converter = g_iconv_open ("UTF-8", "UTF-8");
	}

	return text_convert_invalid (text, len, utf8_fixup_converter, unicode_fallback_string, len_out);
}

void
PrintTextTimeStamp (session *sess, char *text, time_t timestamp)
{
	if (!sess)
	{
		if (!sess_list)
			return;
		sess = (session *) sess_list->data;
	}

	/* make sure it's valid utf8 */
	if (text[0] == '\0')
	{
		text = g_strdup ("\n");
	}
	else
	{
		text = text_fixup_invalid_utf8 (text, -1, NULL);
	}

	log_write (sess, text, timestamp);
	scrollback_save (sess, text, timestamp);
	fe_print_text (sess, text, timestamp, FALSE);
	g_free (text);
}

void
PrintText (session *sess, char *text)
{
	PrintTextTimeStamp (sess, text, 0);
}

void
PrintTextf (session *sess, const char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	PrintText (sess, buf);
	g_free (buf);
}

void
PrintTextTimeStampf (session *sess, time_t timestamp, const char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	PrintTextTimeStamp (sess, buf, timestamp);
	g_free (buf);
}

/* Print Events stuff here --AGL */

/* Consider the following a NOTES file:

   The main upshot of this is:
   * Plugins and Perl scripts (when I get round to signaling perl.c) can intercept text events and do what they like
   * The default text engine can be config'ed

   By default it should appear *exactly* the same (I'm working hard not to change the default style) but if you go into Settings->Edit Event Texts you can change the text's. The format is thus:

   The normal %Cx (color) and %B (bold) etc work

   $x is replaced with the data in var x (e.g. $1 is often the nick)

   $axxx is replace with a single byte of value xxx (in base 10)

   AGL (990507)
 */

/* These lists are thus:
   pntevts_text[] are the strings the user sees (WITH %x etc)
   pntevts[] are the data strings with \000 etc
 */

/* To add a new event:

   Think up a name (like "Join")
   Make up a pevt_name_help struct
	Add an entry to textevents.in
	Type: make textevents
 */

/* Internals:

   On startup ~/.xchat/printevents.conf is loaded if it doesn't exist the
   defaults are loaded. Any missing events are filled from defaults.
   Each event is parsed by pevt_build_string and a binary output is produced
   which looks like:

   (byte) value: 0 = {
   (int) numbers of bytes
   (char []) that number of byte to be memcpy'ed into the buffer
   }
   1 =
   (byte) number of varable to insert
   2 = end of buffer

   Each XP_TE_* signal is hard coded to call text_emit which calls
   display_event which decodes the data

   This means that this system *should be faster* than g_snprintf because
   it always 'knows' that format of the string (basically is preparses much
   of the work)

   --AGL
 */

char *pntevts_text[NUM_XP];
char *pntevts[NUM_XP];

#define pevt_generic_none_help NULL

static char * const pevt_genmsg_help[] = {
	N_("Left message"),
	N_("Right message"),
};

#if 0
static char * const pevt_identd_help[] = {
	N_("IP address"),
	N_("Username")
};
#endif

static char * const pevt_join_help[] = {
	N_("The nick of the joining person"),
	N_("The channel being joined"),
	N_("The host of the person"),
	N_("The account of the person"),
};

static char * const pevt_chanaction_help[] = {
	N_("Nickname"),
	N_("The action"),
	N_("Mode char"),
	N_("Identified text"),
};

static char * const pevt_chanmsg_help[] = {
	N_("Nickname"),
	N_("The text"),
	N_("Mode char"),
	N_("Identified text"),
};

static char * const pevt_privmsg_help[] = {
	N_("Nickname"),
	N_("The message"),
	N_("Identified text")
};

static char * const pevt_capack_help[] = {
	N_("Server Name"),
	N_("Acknowledged Capabilities")
};

static char * const pevt_capdel_help[] = {
	N_("Server Name"),
	N_("Removed Capabilities")
};

static char * const pevt_caplist_help[] = {
	N_("Server Name"),
	N_("Server Capabilities")
};

static char * const pevt_capreq_help[] = {
	N_("Requested Capabilities")
};

static char * const pevt_changenick_help[] = {
	N_("Old nickname"),
	N_("New nickname"),
};

static char * const pevt_newtopic_help[] = {
	N_("Nick of person who changed the topic"),
	N_("Topic"),
	N_("Channel"),
};

static char * const pevt_topic_help[] = {
	N_("Channel"),
	N_("Topic"),
};

static char * const pevt_kick_help[] = {
	N_("The nickname of the kicker"),
	N_("The person being kicked"),
	N_("The channel"),
	N_("The reason"),
};

static char * const pevt_part_help[] = {
	N_("The nick of the person leaving"),
	N_("The host of the person"),
	N_("The channel"),
};

static char * const pevt_chandate_help[] = {
	N_("The channel"),
	N_("The time"),
};

static char * const pevt_topicdate_help[] = {
	N_("The channel"),
	N_("The creator"),
	N_("The time"),
};

static char * const pevt_quit_help[] = {
	N_("Nick"),
	N_("Reason"),
	N_("Host"),
};

static char * const pevt_pingrep_help[] = {
	N_("Who it's from"),
	N_("The time in x.x format (see below)"),
};

static char * const pevt_notice_help[] = {
	N_("Who it's from"),
	N_("The message"),
};

static char * const pevt_channotice_help[] = {
	N_("Who it's from"),
	N_("The Channel it's going to"),
	N_("The message"),
};

static char * const pevt_uchangenick_help[] = {
	N_("Old nickname"),
	N_("New nickname"),
};

static char * const pevt_ukick_help[] = {
	N_("The person being kicked"),
	N_("The channel"),
	N_("The nickname of the kicker"),
	N_("The reason"),
};

static char * const pevt_partreason_help[] = {
	N_("The nick of the person leaving"),
	N_("The host of the person"),
	N_("The channel"),
	N_("The reason"),
};

static char * const pevt_ctcpsnd_help[] = {
	N_("The sound"),
	N_("The nick of the person"),
	N_("The channel"),
};

static char * const pevt_ctcpgen_help[] = {
	N_("The CTCP event"),
	N_("The nick of the person"),
};

static char * const pevt_ctcpgenc_help[] = {
	N_("The CTCP event"),
	N_("The nick of the person"),
	N_("The Channel it's going to"),
};

static char * const pevt_chansetkey_help[] = {
	N_("The nick of the person who set the key"),
	N_("The key"),
};

static char * const pevt_chansetlimit_help[] = {
	N_("The nick of the person who set the limit"),
	N_("The limit"),
};

static char * const pevt_chanop_help[] = {
	N_("The nick of the person who did the op'ing"),
	N_("The nick of the person who has been op'ed"),
};

static char * const pevt_chanhop_help[] = {
	N_("The nick of the person who has been halfop'ed"),
	N_("The nick of the person who did the halfop'ing"),
};

static char * const pevt_chanvoice_help[] = {
	N_("The nick of the person who did the voice'ing"),
	N_("The nick of the person who has been voice'ed"),
};

static char * const pevt_chanban_help[] = {
	N_("The nick of the person who did the banning"),
	N_("The ban mask"),
};

static char * const pevt_chanquiet_help[] = {
	N_("The nick of the person who did the quieting"),
	N_("The quiet mask"),
};

static char * const pevt_chanrmkey_help[] = {
	N_("The nick who removed the key"),
};

static char * const pevt_chanrmlimit_help[] = {
	N_("The nick who removed the limit"),
};

static char * const pevt_chandeop_help[] = {
	N_("The nick of the person who did the deop'ing"),
	N_("The nick of the person who has been deop'ed"),
};
static char * const pevt_chandehop_help[] = {
	N_("The nick of the person who did the dehalfop'ing"),
	N_("The nick of the person who has been dehalfop'ed"),
};

static char * const pevt_chandevoice_help[] = {
	N_("The nick of the person who did the devoice'ing"),
	N_("The nick of the person who has been devoice'ed"),
};

static char * const pevt_chanunban_help[] = {
	N_("The nick of the person who did the unban'ing"),
	N_("The ban mask"),
};

static char * const pevt_chanunquiet_help[] = {
	N_("The nick of the person who did the unquiet'ing"),
	N_("The quiet mask"),
};

static char * const pevt_chanexempt_help[] = {
	N_("The nick of the person who did the exempt"),
	N_("The exempt mask"),
};

static char * const pevt_chanrmexempt_help[] = {
	N_("The nick of the person removed the exempt"),
	N_("The exempt mask"),
};

static char * const pevt_chaninvite_help[] = {
	N_("The nick of the person who did the invite"),
	N_("The invite mask"),
};

static char * const pevt_chanrminvite_help[] = {
	N_("The nick of the person removed the invite"),
	N_("The invite mask"),
};

static char * const pevt_chanmodegen_help[] = {
	N_("The nick of the person setting the mode"),
	N_("The mode's sign (+/-)"),
	N_("The mode letter"),
	N_("The channel it's being set on"),
};

static char * const pevt_whois1_help[] = {
	N_("Nickname"),
	N_("Username"),
	N_("Host"),
	N_("Full name"),
};

static char * const pevt_whois2_help[] = {
	N_("Nickname"),
	N_("Channel Membership/\"is an IRC operator\""),
};

static char * const pevt_whois3_help[] = {
	N_("Nickname"),
	N_("Server Information"),
};

static char * const pevt_whois4_help[] = {
	N_("Nickname"),
	N_("Idle time"),
};

static char * const pevt_whois4t_help[] = {
	N_("Nickname"),
	N_("Idle time"),
	N_("Signon time"),
};

static char * const pevt_whois5_help[] = {
	N_("Nickname"),
	N_("Away reason"),
};

static char * const pevt_whois6_help[] = {
	N_("Nickname"),
};

static char * const pevt_whoisid_help[] = {
	N_("Nickname"),
	N_("Message"),
	"Numeric"
};

static char * const pevt_whoisauth_help[] = {
	N_("Nickname"),
	N_("Message"),
	N_("Account"),
};

static char * const pevt_whoisrealhost_help[] = {
	N_("Nickname"),
	N_("Real user@host"),
	N_("Real IP"),
	N_("Message"),
};

static char * const pevt_generic_channel_help[] = {
	N_("Channel Name"),
};

static char * const pevt_saslauth_help[] = {
	N_("Username"),
	N_("Mechanism")
};

static char * const pevt_saslresponse_help[] = {
	N_("Server Name"),
	N_("Raw Numeric or Identifier"),
	N_("Username"),
	N_("Message")
};

static char * const pevt_servertext_help[] = {
	N_("Text"),
	N_("Server Name"),
	N_("Raw Numeric or Identifier")
};

static char * const pevt_sslmessage_help[] = {
	N_("Text"),
	N_("Server Name")
};

static char * const pevt_invited_help[] = {
	N_("Channel Name"),
	N_("Nick of person who invited you"),
	N_("Server Name"),
};

static char * const pevt_usersonchan_help[] = {
	N_("Channel Name"),
	N_("Users"),
};

static char * const pevt_nickclash_help[] = {
	N_("Nickname in use"),
	N_("Nick being tried"),
};

static char * const pevt_connfail_help[] = {
	N_("Error"),
};

static char * const pevt_connect_help[] = {
	N_("Host"),
	N_("IP"),
	N_("Port"),
};

static char * const pevt_sconnect_help[] = {
	"PID"
};

static char * const pevt_generic_nick_help[] = {
	N_("Nickname"),
	N_("Server Name"),
	N_("Network")
};

static char * const pevt_chanmodes_help[] = {
	N_("Channel Name"),
	N_("Modes string"),
};

static char * const pevt_chanurl_help[] = {
	N_("Channel Name"),
	N_("URL"),
};

static char * const pevt_rawmodes_help[] = {
	N_("Nickname"),
	N_("Modes string"),
};

static char * const pevt_kill_help[] = {
	N_("Nickname"),
	N_("Reason"),
};

static char * const pevt_dccchaterr_help[] = {
	N_("Nickname"),
	N_("IP address"),
	N_("Port"),
	N_("Error"),
};

static char * const pevt_dccstall_help[] = {
	N_("DCC Type"),
	N_("Filename"),
	N_("Nickname"),
};

static char * const pevt_generic_file_help[] = {
	N_("Filename"),
	N_("Error"),
};

static char * const pevt_dccrecverr_help[] = {
	N_("Filename"),
	N_("Destination filename"),
	N_("Nickname"),
	N_("Error"),
};

static char * const pevt_dccrecvcomp_help[] = {
	N_("Filename"),
	N_("Destination filename"),
	N_("Nickname"),
	N_("CPS"),
};

static char * const pevt_dccconfail_help[] = {
	N_("DCC Type"),
	N_("Nickname"),
	N_("Error"),
};

static char * const pevt_dccchatcon_help[] = {
	N_("Nickname"),
	N_("IP address"),
};

static char * const pevt_dcccon_help[] = {
	N_("Nickname"),
	N_("IP address"),
	N_("Filename"),
};

static char * const pevt_dccsendfail_help[] = {
	N_("Filename"),
	N_("Nickname"),
	N_("Error"),
};

static char * const pevt_dccsendcomp_help[] = {
	N_("Filename"),
	N_("Nickname"),
	N_("CPS"),
};

static char * const pevt_dccoffer_help[] = {
	N_("Filename"),
	N_("Nickname"),
	N_("Pathname"),
};

static char * const pevt_dccfileabort_help[] = {
	N_("Nickname"),
	N_("Filename")
};

static char * const pevt_dccchatabort_help[] = {
	N_("Nickname"),
};

static char * const pevt_dccresumeoffer_help[] = {
	N_("Nickname"),
	N_("Filename"),
	N_("Position"),
};

static char * const pevt_dccsendoffer_help[] = {
	N_("Nickname"),
	N_("Filename"),
	N_("Size"),
	N_("IP address"),
};

static char * const pevt_dccgenericoffer_help[] = {
	N_("DCC String"),
	N_("Nickname"),
};

static char * const pevt_notifyaway_help[] = {
	N_("Nickname"),
	N_("Away Reason"),
};

static char * const pevt_notifynumber_help[] = {
	N_("Number of notify items"),
};

static char * const pevt_serverlookup_help[] = {
	N_("Server Name"),
};

static char * const pevt_servererror_help[] = {
	N_("Text"),
};

static char * const pevt_foundip_help[] = {
	N_("IP"),
};

static char * const pevt_dccrename_help[] = {
	N_("Old Filename"),
	N_("New Filename"),
};

static char * const pevt_ctcpsend_help[] = {
	N_("Receiver"),
	N_("Message"),
};

static char * const pevt_ignoreaddremove_help[] = {
	N_("Hostmask"),
};

static char * const pevt_resolvinguser_help[] = {
	N_("Nickname"),
	N_("Hostname"),
};

static char * const pevt_malformed_help[] = {
	N_("Nickname"),
	N_("The Packet"),
};

static char * const pevt_pingtimeout_help[] = {
	N_("Seconds"),
};

static char * const pevt_uinvite_help[] = {
	N_("Nick of person who have been invited"),
	N_("Channel Name"),
	N_("Server Name"),
};

static char * const pevt_banlist_help[] = {
	N_("Channel"),
	N_("Banmask"),
	N_("Who set the ban"),
	N_("Ban time"),
};

static char * const pevt_discon_help[] = {
	N_("Error"),
};

#include "textevents.h"

static void
pevent_load_defaults (void)
{
	int i;

	for (i = 0; i < NUM_XP; i++)
	{
		g_free (pntevts_text[i]);

		/* make-te.c sets this 128 flag (DON'T call gettext() flag) */
		if (te[i].num_args & 128)
			pntevts_text[i] = g_strdup (te[i].def);
		else
			pntevts_text[i] = g_strdup (_(te[i].def));
	}
}

void
pevent_make_pntevts (void)
{
	int i, m;

	for (i = 0; i < NUM_XP; i++)
	{
		g_free (pntevts[i]);
		if (pevt_build_string (pntevts_text[i], &(pntevts[i]), &m) != 0)
		{
			/* make-te.c sets this 128 flag (DON'T call gettext() flag) */
			const gboolean translate = !(te[i].num_args & 128);

			g_warning ("Error parsing event %s\nLoading default.", te[i].name);
			g_free (pntevts_text[i]);

			if (translate)
				pntevts_text[i] = g_strdup (_(te[i].def));
			else
				pntevts_text[i] = g_strdup (te[i].def);

			if (pevt_build_string (pntevts_text[i], &(pntevts[i]), &m) != 0 && !translate)
			{
				g_error ("HexChat CRITICAL *** default event text failed to build!");
			}
			else
			{
				g_warning ("Error parsing translated event %s\nLoading untranslated.", te[i].name);
				g_free (pntevts_text[i]);

				pntevts_text[i] = g_strdup (te[i].def);

				if (pevt_build_string (pntevts_text[i], &(pntevts[i]), &m) != 0)
				{
					g_error ("HexChat CRITICAL *** default event text failed to build!");
				}
			}
		}
	}
}

/* Loading happens at 2 levels:
   1) File is read into blocks
   2) Pe block is parsed and loaded

   --AGL */

/* Better hope you pass good args.. --AGL */

static void
pevent_trigger_load (int *i_penum, char **i_text, char **i_snd)
{
	int penum = *i_penum;
	char *text = *i_text, *snd = *i_snd;

	if (penum != -1 && text != NULL)
	{
		g_free (pntevts_text[penum]);
		pntevts_text[penum] = g_strdup (text);
	}

	g_free (text);
	g_free (snd);
	*i_text = NULL;
	*i_snd = NULL;
	*i_penum = 0;
}

static int
pevent_find (char *name, int *i_i)
{
	int i = *i_i, j;

	j = i + 1;
	while (1)
	{
		if (j == NUM_XP)
			j = 0;
		if (strcmp (te[j].name, name) == 0)
		{
			*i_i = j;
			return j;
		}
		if (j == i)
			return -1;
		j++;
	}
}

int
pevent_load (char *filename)
{
	/* AGL, I've changed this file and pevent_save, could you please take a look at
	 *      the changes and possibly modify them to suit you
	 *      //David H
	 */
	char *buf, *ibuf;
	int fd, i = 0, pnt = 0;
	struct stat st;
	char *text = NULL, *snd = NULL;
	int penum = 0;
	char *ofs;

	if (filename == NULL)
		fd = hexchat_open_file ("pevents.conf", O_RDONLY, 0, 0);
	else
		fd = hexchat_open_file (filename, O_RDONLY, 0, XOF_FULLPATH);

	if (fd == -1)
		return 1;
	if (fstat (fd, &st) != 0)
	{
		close (fd);
		return 1;
	}
	ibuf = g_malloc (st.st_size);
	read (fd, ibuf, st.st_size);
	close (fd);

	while (buf_get_line (ibuf, &buf, &pnt, st.st_size))
	{
		if (buf[0] == '#')
			continue;
		if (strlen (buf) == 0)
			continue;

		ofs = strchr (buf, '=');
		if (!ofs)
			continue;
		*ofs = 0;
		ofs++;

		if (strcmp (buf, "event_name") == 0)
		{
			if (penum >= 0)
				pevent_trigger_load (&penum, &text, &snd);
			penum = pevent_find (ofs, &i);
			continue;
		} else if (strcmp (buf, "event_text") == 0)
		{
			g_free (text);
			text = g_strdup (ofs);
			continue;
		}

		continue;
	}

	pevent_trigger_load (&penum, &text, &snd);
	g_free (ibuf);
	return 0;
}

static void
pevent_check_all_loaded (void)
{
	int i;

	for (i = 0; i < NUM_XP; i++)
	{
		if (pntevts_text[i] == NULL)
		{
			/*printf ("%s\n", te[i].name);
			g_snprintf(out, sizeof(out), "The data for event %s failed to load. Reverting to defaults.\nThis may be because a new version of HexChat is loading an old config file.\n\nCheck all print event texts are correct", evtnames[i]);
			   gtkutil_simpledialog(out); */
			/* make-te.c sets this 128 flag (DON'T call gettext() flag) */
			if (te[i].num_args & 128)
				pntevts_text[i] = g_strdup (te[i].def);
			else
				pntevts_text[i] = g_strdup (_(te[i].def));
		}
	}
}

void
load_text_events ()
{
	memset (&pntevts_text, 0, sizeof (char *) * (NUM_XP));
	memset (&pntevts, 0, sizeof (char *) * (NUM_XP));

	if (pevent_load (NULL))
		pevent_load_defaults ();
	pevent_check_all_loaded ();
	pevent_make_pntevts ();
}

/*
	CL: format_event now handles filtering of arguments:
	1) if prefs.hex_text_stripcolor_msg is set, filter all style control codes from arguments
	2) always strip \010 (ATTR_HIDDEN) from arguments: it is only for use in the format string itself
*/
#define ARG_FLAG(argn) (1 << (argn))

void
format_event (session *sess, int index, char **args, char *o, gsize sizeofo, unsigned int stripcolor_args)
{
	int len, ii, numargs;
	gsize oi;
	char *i, *ar, d, a, done_all = FALSE;

	i = pntevts[index];
	numargs = te[index].num_args & 0x7f;

	oi = ii = len = d = a = 0;
	o[0] = 0;

	if (i == NULL)
		return;

	while (done_all == FALSE)
	{
		d = i[ii++];
		switch (d)
		{
		case 0:
			memcpy (&len, &(i[ii]), sizeof (int));
			ii += sizeof (int);
			if (oi + len > sizeofo)
			{
				printf ("Overflow in display_event (%s)\n", i);
				o[0] = 0;
				return;
			}
			memcpy (&(o[oi]), &(i[ii]), len);
			oi += len;
			ii += len;
			break;
		case 1:
			a = i[ii++];
			if (a > numargs)
			{
				fprintf (stderr,
							"HexChat DEBUG: display_event: arg > numargs (%d %d %s)\n",
							a, numargs, i);
				break;
			}
			ar = args[(int) a + 1];
			if (ar == NULL)
			{
				printf ("arg[%d] is NULL in print event\n", a + 1);
			} else
			{
				if (strlen (ar) > sizeofo - oi - 4)
					ar[sizeofo - oi - 4] = 0;	/* Avoid buffer overflow */
				if (stripcolor_args & ARG_FLAG(a + 1)) len = strip_color2 (ar, -1, &o[oi], STRIP_ALL);
				else len = strip_hidden_attribute (ar, &o[oi]);
				oi += len;
			}
			break;
		case 2:
			o[oi++] = '\n';
			o[oi++] = 0;
			done_all = TRUE;
			continue;
		case 3:
			if (prefs.hex_text_indent)
				o[oi++] = '\t';
			else
				o[oi++] = ' ';
			break;
		}
	}
	o[oi] = 0;
	if (*o == '\n')
		o[0] = 0;
}

static void
display_event (session *sess, int event, char **args, 
					unsigned int stripcolor_args, time_t timestamp)
{
	char o[4096];
	format_event (sess, event, args, o, sizeof (o), stripcolor_args);
	if (o[0])
		PrintTextTimeStamp (sess, o, timestamp);
}

int
pevt_build_string (const char *input, char **output, int *max_arg)
{
	struct pevt_stage1 *s = NULL, *base = NULL, *last = NULL, *next;
	int clen;
	char o[4096], d, *obuf, *i;
	int oi, ii, max = -1, len, x;

	len = strlen (input);
	i = g_malloc (len + 1);
	memcpy (i, input, len + 1);
	check_special_chars (i, TRUE);

	len = strlen (i);

	clen = oi = ii = 0;

	for (;;)
	{
		if (ii == len)
			break;
		d = i[ii++];
		if (d != '$')
		{
			o[oi++] = d;
			continue;
		}
		if (i[ii] == '$')
		{
			o[oi++] = '$';
			continue;
		}
		if (oi > 0)
		{
			s = g_new (struct pevt_stage1, 1);
			if (base == NULL)
				base = s;
			if (last != NULL)
				last->next = s;
			last = s;
			s->next = NULL;
			s->data = g_malloc (oi + sizeof (int) + 1);
			s->len = oi + sizeof (int) + 1;
			clen += oi + sizeof (int) + 1;
			s->data[0] = 0;
			memcpy (&(s->data[1]), &oi, sizeof (int));
			memcpy (&(s->data[1 + sizeof (int)]), o, oi);
			oi = 0;
		}
		if (ii == len)
		{
			fe_message ("String ends with a $", FE_MSG_WARN);
			goto err;
		}
		d = i[ii++];
		if (d == 'a')
		{
			/* Hex value */
			if (ii == len)
				goto a_len_error;
			d = i[ii++];
			d -= '0';
			x = d * 100;
			if (ii == len)
				goto a_len_error;
			d = i[ii++];
			d -= '0';
			x += d * 10;
			if (ii == len)
				goto a_len_error;
			d = i[ii++];
			d -= '0';
			x += d;
			if (x > 255)
				goto a_range_error;
			o[oi++] = x;
			continue;

		a_len_error:
			fe_message ("String ends in $a", FE_MSG_WARN);
			goto err;
		a_range_error:
			fe_message ("$a value is greater than 255", FE_MSG_WARN);
			goto err;
		}
		if (d == 't')
		{
			/* Tab - if tabnicks is set then write '\t' else ' ' */
			s = g_new (struct pevt_stage1, 1);
			if (base == NULL)
				base = s;
			if (last != NULL)
				last->next = s;
			last = s;
			s->next = NULL;
			s->data = g_malloc (1);
			s->len = 1;
			clen += 1;
			s->data[0] = 3;

			continue;
		}
		if (d < '1' || d > '9')
		{
			g_snprintf (o, sizeof (o), "Error, invalid argument $%c\n", d);
			fe_message (o, FE_MSG_WARN);
			goto err;
		}
		d -= '0';
		if (max < d)
			max = d;
		s = g_new (struct pevt_stage1, 1);
		if (base == NULL)
			base = s;
		if (last != NULL)
			last->next = s;
		last = s;
		s->next = NULL;
		s->data = g_malloc (2);
		s->len = 2;
		clen += 2;
		s->data[0] = 1;
		s->data[1] = d - 1;
	}
	if (oi > 0)
	{
		s = g_new (struct pevt_stage1, 1);
		if (base == NULL)
			base = s;
		if (last != NULL)
			last->next = s;
		last = s;
		s->next = NULL;
		s->data = g_malloc (oi + sizeof (int) + 1);
		s->len = oi + sizeof (int) + 1;
		clen += oi + sizeof (int) + 1;
		s->data[0] = 0;
		memcpy (&(s->data[1]), &oi, sizeof (int));
		memcpy (&(s->data[1 + sizeof (int)]), o, oi);
		oi = 0;
	}
	s = g_new (struct pevt_stage1, 1);
	if (base == NULL)
		base = s;
	if (last != NULL)
		last->next = s;
	s->next = NULL;
	s->data = g_malloc (1);
	s->len = 1;
	clen += 1;
	s->data[0] = 2;

	oi = 0;
	s = base;
	obuf = g_malloc (clen);

	while (s)
	{
		next = s->next;
		memcpy (&obuf[oi], s->data, s->len);
		oi += s->len;
		g_free (s->data);
		g_free (s);
		s = next;
	}

	g_free (i);

	if (max_arg)
		*max_arg = max;
	if (output)
		*output = obuf;
	else
		g_free (obuf);

	return 0;

err:
	while (s)
	{
		next = s->next;
		g_free (s->data);
		g_free (s);
		s = next;
	}

	g_free(i);

	return 1;
}


/* black n white(0/1) are bad colors for nicks, and we'll use color 2 for us */
/* also light/dark gray (14/15) */
/* 5,7,8 are all shades of yellow which happen to look damn near the same */

static char rcolors[] = { 19, 20, 22, 24, 25, 26, 27, 28, 29 };

int
text_color_of (char *name)
{
	int i = 0, sum = 0;

	while (name[i])
		sum += name[i++];
	sum %= sizeof (rcolors) / sizeof (char);
	return rcolors[sum];
}


/* called by EMIT_SIGNAL macro */

void
text_emit (int index, session *sess, char *a, char *b, char *c, char *d,
			  time_t timestamp)
{
	char *word[PDIWORDS];
	int i;
	tab_state_flags current_state = sess->tab_state;
	tab_state_flags plugin_state = sess->last_tab_state;
	unsigned int stripcolor_args = (chanopt_is_set (prefs.hex_text_stripcolor_msg, sess->text_strip) ? 0xFFFFFFFF : 0);
	char tbuf[NICKLEN + 4];

	if (prefs.hex_text_color_nicks && (index == XP_TE_CHANACTION || index == XP_TE_CHANMSG))
	{
		g_snprintf (tbuf, sizeof (tbuf), "\003%d%s", text_color_of (a), a);
		a = tbuf;
		stripcolor_args &= ~ARG_FLAG(1);	/* don't strip color from this argument */
	}

	word[0] = te[index].name;
	word[1] = (a ? a : "\000");
	word[2] = (b ? b : "\000");
	word[3] = (c ? c : "\000");
	word[4] = (d ? d : "\000");
	for (i = 5; i < PDIWORDS; i++)
		word[i] = "\000";

	/* We want to ignore the tab state if the plugin emits new events
	 * and restore it if it doesn't eat the current one */
	sess->tab_state = plugin_state;
	if (plugin_emit_print (sess, word, timestamp))
		return;

	/* The plugin may have changed the state which we should respect.
	 * If the state is NEW_DATA we don't actually know if that was on
	 * purpose though as print() sets it, so for now we ignore that. FIXME */
	if (sess->tab_state == plugin_state || sess->tab_state == TAB_STATE_NEW_DATA)
		sess->tab_state = current_state;

	/* If a plugin's callback executes "/close", 'sess' may be invalid */
	if (!is_session (sess))
		return;

	switch (index)
	{
	case XP_TE_JOIN:
	case XP_TE_PART:
	case XP_TE_PARTREASON:
	case XP_TE_QUIT:
		/* implement ConfMode / Hide Join and Part Messages */
		if (chanopt_is_set (prefs.hex_irc_conf_mode, sess->text_hidejoinpart))
			return;
		break;

	/* ===Private message=== */
	case XP_TE_PRIVMSG:
	case XP_TE_DPRIVMSG:
	case XP_TE_PRIVACTION:
	case XP_TE_DPRIVACTION:
		if (chanopt_is_set (prefs.hex_input_beep_priv, sess->alert_beep) && (!prefs.hex_away_omit_alerts || !sess->server->is_away))
			sound_beep (sess);
		if (chanopt_is_set (prefs.hex_input_flash_priv, sess->alert_taskbar) && (!prefs.hex_away_omit_alerts || !sess->server->is_away))
			fe_flash_window (sess);
		/* why is this one different? because of plugin-tray.c's hooks! ugly */
		if (sess->alert_tray == SET_ON)
			fe_tray_set_icon (FE_ICON_MESSAGE);
		break;

	/* ===Highlighted message=== */
	case XP_TE_HCHANACTION:
	case XP_TE_HCHANMSG:
		if (chanopt_is_set (prefs.hex_input_beep_hilight, sess->alert_beep) && (!prefs.hex_away_omit_alerts || !sess->server->is_away))
			sound_beep (sess);
		if (chanopt_is_set (prefs.hex_input_flash_hilight, sess->alert_taskbar) && (!prefs.hex_away_omit_alerts || !sess->server->is_away))
			fe_flash_window (sess);
		if (sess->alert_tray == SET_ON)
			fe_tray_set_icon (FE_ICON_MESSAGE);
		break;

	/* ===Channel message=== */
	case XP_TE_CHANACTION:
	case XP_TE_CHANMSG:
		if (chanopt_is_set (prefs.hex_input_beep_chans, sess->alert_beep) && (!prefs.hex_away_omit_alerts || !sess->server->is_away))
			sound_beep (sess);
		if (chanopt_is_set (prefs.hex_input_flash_chans, sess->alert_taskbar) && (!prefs.hex_away_omit_alerts || !sess->server->is_away))
			fe_flash_window (sess);
		if (sess->alert_tray == SET_ON)
			fe_tray_set_icon (FE_ICON_MESSAGE);
		break;

	/* ===Nick change message=== */
	case XP_TE_CHANGENICK:
		if (prefs.hex_irc_hide_nickchange)
			return;
		break;
	}

	sound_play_event (index);
	display_event (sess, index, word, stripcolor_args, timestamp);
}

char *
text_find_format_string (char *name)
{
	int i = 0;

	i = pevent_find (name, &i);
	if (i >= 0)
		return pntevts_text[i];

	return NULL;
}

int
text_emit_by_name (char *name, session *sess, time_t timestamp,
				   char *a, char *b, char *c, char *d)
{
	int i = 0;

	i = pevent_find (name, &i);
	if (i >= 0)
	{
		text_emit (i, sess, a, b, c, d, timestamp);
		return 1;
	}

	return 0;
}

void
pevent_save (char *fn)
{
	int fd, i;
	char buf[1024];

	if (!fn)
		fd = hexchat_open_file ("pevents.conf", O_CREAT | O_TRUNC | O_WRONLY,
									 0x180, XOF_DOMODE);
	else
		fd = hexchat_open_file (fn, O_CREAT | O_TRUNC | O_WRONLY, 0x180,
									 XOF_FULLPATH | XOF_DOMODE);
	if (fd == -1)
	{
		/*
		   fe_message ("Error opening config file\n", FALSE); 
		   If we get here when X-Chat is closing the fe-message causes a nice & hard crash
		   so we have to use perror which doesn't rely on GTK
		 */

		perror ("Error opening config file\n");
		return;
	}

	for (i = 0; i < NUM_XP; i++)
	{
		write (fd, buf, g_snprintf (buf, sizeof (buf),
										  "event_name=%s\n", te[i].name));
		write (fd, buf, g_snprintf (buf, sizeof (buf),
										  "event_text=%s\n\n", pntevts_text[i]));
	}

	close (fd);
}

/* =========================== */
/* ========== SOUND ========== */
/* =========================== */

char *sound_files[NUM_XP];

void
sound_beep (session *sess)
{
	if (!prefs.hex_gui_focus_omitalerts || fe_gui_info (sess, 0) != 1)
	{
		if (sound_files[XP_TE_BEEP] && sound_files[XP_TE_BEEP][0])
			/* user defined beep _file_ */
			sound_play_event (XP_TE_BEEP);
		else
			/* system beep */
			fe_beep (sess);
	}
}

void
sound_play (const char *file, gboolean quiet)
{
	char *buf;
	char *wavfile;
#ifndef WIN32
	char *cmd;
#endif

	/* the pevents GUI editor triggers this after removing a soundfile */
	if (!file[0])
	{
		return;
	}

	/* check for fullpath */
	if (g_path_is_absolute (file))
	{
		wavfile = g_strdup (file);
	}
	else
	{
		wavfile = g_build_filename (get_xdir (), HEXCHAT_SOUND_DIR, file, NULL);
	}

	if (g_access (wavfile, R_OK) == 0)
	{
#ifdef WIN32
		gunichar2 *wavfile_utf16 = g_utf8_to_utf16 (wavfile, -1, NULL, NULL, NULL);

		if (wavfile_utf16 != NULL)
		{
			PlaySoundW (wavfile_utf16, NULL, SND_NODEFAULT | SND_FILENAME | SND_ASYNC);

			g_free (wavfile_utf16);
		}
#else
#ifdef USE_LIBCANBERRA
		if (ca_con == NULL)
		{
			ca_context_create (&ca_con);
			ca_context_change_props (ca_con,
											CA_PROP_APPLICATION_ID, "hexchat",
											CA_PROP_APPLICATION_NAME, "HexChat",
											CA_PROP_APPLICATION_ICON_NAME, "hexchat", NULL);
		}

		if (ca_context_play (ca_con, 0, CA_PROP_MEDIA_FILENAME, wavfile, NULL) != 0)
#endif
		{
			cmd = g_find_program_in_path ("play");
	
			if (cmd)
			{
				buf = g_strdup_printf ("%s \"%s\"", cmd, wavfile);
				hexchat_exec (buf);
				g_free (buf);
				g_free (cmd);
			}
		}
#endif
	}
	else
	{
		if (!quiet)
		{
			buf = g_strdup_printf (_("Cannot read sound file:\n%s"), wavfile);
			fe_message (buf, FE_MSG_ERROR);
			g_free (buf);
		}
	}

	g_free (wavfile);
}

void
sound_play_event (int i)
{
	if (sound_files[i])
	{
		sound_play (sound_files[i], FALSE);
	}
}

static void
sound_load_event (char *evt, char *file)
{
	int i = 0;

	if (file[0] && pevent_find (evt, &i) != -1)
	{
		g_free (sound_files[i]);
		sound_files[i] = g_strdup (file);
	}
}

void
sound_load ()
{
	int fd;
	char buf[512];
	char evt[128];

	memset (&sound_files, 0, sizeof (char *) * (NUM_XP));

	fd = hexchat_open_file ("sound.conf", O_RDONLY, 0, 0);
	if (fd == -1)
		return;

	evt[0] = 0;
	while (waitline (fd, buf, sizeof buf, FALSE) != -1)
	{
		if (strncmp (buf, "event=", 6) == 0)
		{
			safe_strcpy (evt, buf + 6, sizeof (evt));
		}
		else if (strncmp (buf, "sound=", 6) == 0)
		{
			if (evt[0] != 0)
			{
				sound_load_event (evt, buf + 6);
				evt[0] = 0;
			}
		}
	}

	close (fd);
}

void
sound_save ()
{
	int fd, i;
	char buf[512];

	fd = hexchat_open_file ("sound.conf", O_CREAT | O_TRUNC | O_WRONLY, 0x180,
								 XOF_DOMODE);
	if (fd == -1)
		return;

	for (i = 0; i < NUM_XP; i++)
	{
		if (sound_files[i] && sound_files[i][0])
		{
			write (fd, buf, g_snprintf (buf, sizeof (buf),
											  "event=%s\n", te[i].name));
			write (fd, buf, g_snprintf (buf, sizeof (buf),
											  "sound=%s\n\n", sound_files[i]));
		}
	}

	close (fd);
}
