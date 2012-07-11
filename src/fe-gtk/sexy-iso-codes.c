/*
 *  Copyright (C) 2005 Nathan Fredrickson
 *  Borrowed from Galeon, renamed, and simplified to only use iso-codes with no
 *  fallback method.
 *
 *  Copyright (C) 2004 Christian Persch
 *  Copyright (C) 2004 Crispin Flowerday
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 */

#include "../../config.h"

#include "sexy-iso-codes.h"

#include <glib/gi18n.h>

#include <string.h>

#include <libxml/xmlreader.h>

static GHashTable *iso_639_table = NULL;
static GHashTable *iso_3166_table = NULL;

#define ISO_639_DOMAIN	"iso_639"
#define ISO_3166_DOMAIN	"iso_3166"

#ifdef HAVE_ISO_CODES

#define ISOCODESLOCALEDIR "/share/locale"

static void
read_iso_639_entry (xmlTextReaderPtr reader,
		    GHashTable *table)
{
	xmlChar *code, *name;

	code = xmlTextReaderGetAttribute (reader, (const xmlChar *) "iso_639_1_code");
	name = xmlTextReaderGetAttribute (reader, (const xmlChar *) "name");

	/* Get iso-639-2 code */
	if (code == NULL || code[0] == '\0')
	{
		xmlFree (code);
		/* FIXME: use the 2T or 2B code? */
		code = xmlTextReaderGetAttribute (reader, (const xmlChar *) "iso_639_2T_code");
	}

	if (code != NULL && code[0] != '\0' && name != NULL && name[0] != '\0')
	{
		g_hash_table_insert (table, code, name);
	}
	else
	{
		xmlFree (code);
		xmlFree (name);
	}
}

static void
read_iso_3166_entry (xmlTextReaderPtr reader,
		     GHashTable *table)
{
	xmlChar *code, *name;

	code = xmlTextReaderGetAttribute (reader, (const xmlChar *) "alpha_2_code");
	name = xmlTextReaderGetAttribute (reader, (const xmlChar *) "name");

	if (code != NULL && code[0] != '\0' && name != NULL && name[0] != '\0')
	{
		char *lcode;

		lcode = g_ascii_strdown ((char *) code, -1);
		xmlFree (code);

		g_hash_table_insert (table, lcode, name);
	}
	else
	{
		xmlFree (code);
		xmlFree (name);
	}

}

typedef enum
{
	STATE_START,
	STATE_STOP,
	STATE_ENTRIES,
} ParserState;

static gboolean
load_iso_entries (int iso,
		  GFunc read_entry_func,
		  gpointer user_data)
{
	xmlTextReaderPtr reader;
	ParserState state = STATE_START;
	xmlChar iso_entries[32], iso_entry[32];
	char *filename;
	int ret = -1;

#ifdef WIN32
	filename = g_strdup_printf (".\\share\\xml\\iso-codes\\iso_%d.xml", iso);
#else
	filename = g_strdup_printf ("/usr/share/xml/iso-codes/iso_%d.xml", iso);
#endif
	reader = xmlNewTextReaderFilename (filename);
	if (reader == NULL) goto out;

	xmlStrPrintf (iso_entries, sizeof (iso_entries),
				  (xmlChar *)"iso_%d_entries", iso);
	xmlStrPrintf (iso_entry, sizeof (iso_entry),
				  (xmlChar *)"iso_%d_entry", iso);

	ret = xmlTextReaderRead (reader);

	while (ret == 1)
	{
		const xmlChar *tag;
		xmlReaderTypes type;

		tag = xmlTextReaderConstName (reader);
		type = xmlTextReaderNodeType (reader);

		if (state == STATE_ENTRIES &&
		    type == XML_READER_TYPE_ELEMENT &&
		    xmlStrEqual (tag, iso_entry))
		{
			read_entry_func (reader, user_data);
		}
		else if (state == STATE_START &&
			 type == XML_READER_TYPE_ELEMENT &&
			 xmlStrEqual (tag, iso_entries))
		{
			state = STATE_ENTRIES;
		}
		else if (state == STATE_ENTRIES &&
			 type == XML_READER_TYPE_END_ELEMENT &&
			 xmlStrEqual (tag, iso_entries))
		{
			state = STATE_STOP;
		}
		else if (type == XML_READER_TYPE_SIGNIFICANT_WHITESPACE ||
			 type == XML_READER_TYPE_WHITESPACE ||
			 type == XML_READER_TYPE_TEXT ||
			 type == XML_READER_TYPE_COMMENT)
		{
			/* eat it */
		}
		else
		{
			/* ignore it */
		}

		ret = xmlTextReaderRead (reader);
	}

	xmlFreeTextReader (reader);

out:
	if (ret < 0 || state != STATE_STOP)
	{
		/* This is not critical, we will fallback to our own code */
		g_free (filename);
		return FALSE;
	}

	g_free (filename);

	return TRUE;
}

#endif /* HAVE_ISO_CODES */


static void
ensure_iso_codes_initialised (void)
{
	static gboolean initialised = FALSE;

	if (initialised == TRUE)
	{
		return;
	}
	initialised = TRUE;

#if defined (ENABLE_NLS) && defined (HAVE_ISO_CODES)
	bindtextdomain (ISO_639_DOMAIN, ISOCODESLOCALEDIR);
	bind_textdomain_codeset (ISO_639_DOMAIN, "UTF-8");

	bindtextdomain(ISO_3166_DOMAIN, ISOCODESLOCALEDIR);
	bind_textdomain_codeset (ISO_3166_DOMAIN, "UTF-8");
#endif

	iso_639_table = g_hash_table_new_full (g_str_hash, g_str_equal,
					       (GDestroyNotify) xmlFree,
					       (GDestroyNotify) xmlFree);

	iso_3166_table = g_hash_table_new_full (g_str_hash, g_str_equal,
						(GDestroyNotify) g_free,
						(GDestroyNotify) xmlFree);
	
#ifdef HAVE_ISO_CODES
	load_iso_entries (639, (GFunc) read_iso_639_entry, iso_639_table);
	load_iso_entries (3166, (GFunc) read_iso_3166_entry, iso_3166_table);
#endif
}


static char *
get_iso_name_for_lang_code (const char *code)
{
	char **str;
	char *name = NULL;
	const char *langname, *localename;
	int len;

	str = g_strsplit (code, "_", -1);

	/* count the entries */
	for (len = 0; str[len]; len++ ) /* empty */;

	g_return_val_if_fail (len != 0, NULL);

	langname = (const char *) g_hash_table_lookup (iso_639_table, str[0]);

	if (len == 1 && langname != NULL)
	{
		name = g_strdup (dgettext (ISO_639_DOMAIN, langname));
	}
	else if (len == 2 && langname != NULL)
	{
		localename = (const char *) g_hash_table_lookup (iso_3166_table, str[1]);

		if (localename != NULL)
		{
			/* translators: the first %s is the language name, and the
			 * second %s is the locale name. Example:
			 * "French (France)
			 *
			 * Also: The text before the "|" is context to help you decide on
                         * the correct translation. You MUST OMIT it in the translated string.
			 */
			name = g_strdup_printf (Q_("language|%s (%s)"),
						dgettext (ISO_639_DOMAIN, langname),
						dgettext (ISO_3166_DOMAIN, localename));
		}
		else
		{
			name = g_strdup_printf (Q_("language|%s (%s)"),
						dgettext (ISO_639_DOMAIN, langname), str[1]);
		}
	}

	g_strfreev (str);

	return name;
}

/**
 * gtkspell_iso_codes_lookup_name_for_code:
 * @code: A language code, e.g. en_CA
 *
 * Looks up a name to display to the user for a language code,
 * this might use the iso-codes package if support was compiled
 * in, and it is available
 *
 * Returns: the UTF-8 string to display to the user, or NULL if 
 * a name for the code could not be found
 */
char *
gtkspell_iso_codes_lookup_name_for_code (const char *code)
{
	char * lcode;
	char * ret;

	g_return_val_if_fail (code != NULL, NULL);

	ensure_iso_codes_initialised ();

	lcode = g_ascii_strdown (code, -1);

	ret = get_iso_name_for_lang_code (lcode);

	g_free (lcode);

	return ret;
}

