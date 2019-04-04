/* gtkspell - a spell-checking addon for GTK's TextView widget
* Copyright (c) 2013 Sandro Mani
*
* Based on gtkhtml-editor-spell-language.c code which is
* Copyright (C) 2008 Novell, Inc.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License along
*    with this program; if not, write to the Free Software Foundation, Inc.,
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "config.h"

#include "sexy-iso-codes.h"
#include <libintl.h>
#include <string.h>

#define ISO_639_DOMAIN	"iso_639"
#define ISO_3166_DOMAIN	"iso_3166"

static GHashTable *iso_639_table = NULL;
static GHashTable *iso_3166_table = NULL;

static void
iso_639_start_element (GMarkupParseContext *context,
const gchar *element_name,
const gchar **attribute_names,
const gchar **attribute_values,
gpointer data,
GError **error)
{
	GHashTable *hash_table = data;
	const gchar *name = NULL;
	const gchar *code = NULL;
	int i;

	if (strcmp (element_name, "iso_639_entry") != 0)
		return;

	for (i = 0; attribute_names[i] != NULL; i++)
	{
		if (strcmp (attribute_names[i], "name") == 0)
			name = attribute_values[i];
		else if (strcmp (attribute_names[i], "iso_639_1_code") == 0)
			code = attribute_values[i];
	}

	if (code != NULL && *code != '\0' && name != NULL && *name != '\0')
		g_hash_table_insert (hash_table, g_strdup (code),
		g_strdup (dgettext (ISO_639_DOMAIN, name)));
}

static void
iso_3166_start_element (GMarkupParseContext *context,
const gchar *element_name,
const gchar **attribute_names,
const gchar **attribute_values,
gpointer data,
GError **error)
{
	GHashTable *hash_table = data;
	const gchar *name = NULL;
	const gchar *code = NULL;
	int i;

	if (strcmp (element_name, "iso_3166_entry") != 0)
		return;

	for (i = 0; attribute_names[i] != NULL; i++)
	{
		if (strcmp (attribute_names[i], "name") == 0)
			name = attribute_values[i];
		else if (strcmp (attribute_names[i], "alpha_2_code") == 0)
			code = attribute_values[i];
	}

	if (code != NULL && *code != '\0' && name != NULL && *name != '\0')
		g_hash_table_insert (hash_table, g_strdup (code),
		g_strdup (dgettext (ISO_3166_DOMAIN, name)));
}

static void
iso_codes_parse (const GMarkupParser *parser,
const gchar *basename,
GHashTable *hash_table)
{
	GMappedFile *mapped_file;
	gchar *filename;
	GError *error = NULL;

	filename = g_build_filename (ISO_CODES_PREFIX, "share", "xml", "iso-codes",
		basename, NULL);
	mapped_file = g_mapped_file_new (filename, FALSE, &error);
	g_free (filename);

	if (mapped_file != NULL)
	{
		GMarkupParseContext *context;
		const gchar *contents;
		gsize length;

		context = g_markup_parse_context_new (parser, 0, hash_table, NULL);
		contents = g_mapped_file_get_contents (mapped_file);
		length = g_mapped_file_get_length (mapped_file);
		g_markup_parse_context_parse (context, contents, length, &error);
		g_markup_parse_context_free (context);
		g_mapped_file_unref (mapped_file);
	}

	if (error != NULL)
	{
		g_warning ("%s: %s", basename, error->message);
		g_error_free (error);
	}
}

/**
* codetable_init:
*
* Initializes the code table.
*/
void
codetable_init (void)
{
	GMarkupParser iso_639_parser = {
		iso_639_start_element, NULL, NULL, NULL, NULL
	};

	GMarkupParser iso_3166_parser = {
		iso_3166_start_element, NULL, NULL, NULL, NULL
	};

	g_return_if_fail (iso_639_table == NULL);
	g_return_if_fail (iso_3166_table == NULL);

#ifdef ENABLE_NLS
	bindtextdomain (ISO_639_DOMAIN, ISO_CODES_LOCALEDIR);
	bind_textdomain_codeset (ISO_639_DOMAIN, "UTF-8");

	bindtextdomain (ISO_3166_DOMAIN, ISO_CODES_LOCALEDIR);
	bind_textdomain_codeset (ISO_3166_DOMAIN, "UTF-8");
#endif

	iso_639_table = g_hash_table_new_full (g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)g_free);
	iso_3166_table = g_hash_table_new_full (g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)g_free);

	iso_codes_parse (&iso_639_parser, "iso_639.xml", iso_639_table);
	iso_codes_parse (&iso_3166_parser, "iso_3166.xml", iso_3166_table);
}

/**
* codetable_free:
*
* Frees the code table.
*/
void
codetable_free (void)
{
	g_return_if_fail (iso_639_table != NULL);
	g_return_if_fail (iso_3166_table != NULL);

	g_hash_table_unref (iso_639_table);
	g_hash_table_unref (iso_3166_table);

	iso_639_table = NULL;
	iso_3166_table = NULL;
}

/**
* codetable_lookup:
* @language_code: A language code (i.e. "en_US")
* @language_name: (out) (transfer none): Pointer to the name of the language.
* This pointer is owned by the code table and must not be freed.
* @country_name: (out) (transfer none): Pointer to the name of the country.
* This pointer is owned by the code table and must not be freed.
*
* Looks up the language and country name for the specified language code.
* If no matching entries are found, language_name and country_name will
* simply contain the parts of the language code (i.e. "en" and "US").
*/
void
codetable_lookup (const gchar *language_code, const gchar **language_name, const gchar** country_name)
{
	gchar **parts;

	g_return_if_fail (iso_639_table != NULL);
	g_return_if_fail (iso_3166_table != NULL);

	/* Split language code into parts. */
	parts = g_strsplit (language_code, "_", 2);

	g_return_if_fail (*parts != NULL);

	*language_name = g_hash_table_lookup (iso_639_table, parts[0]);
	if (*language_name == NULL)
	{
		g_hash_table_insert (iso_639_table, g_strdup (parts[0]),
			g_strdup (parts[0]));
		*language_name = g_hash_table_lookup (iso_639_table, parts[0]);
	}

	if (g_strv_length (parts) == 2)
	{
		*country_name = g_hash_table_lookup (iso_3166_table, parts[1]);
		if (*country_name == NULL)
		{
			g_hash_table_insert (iso_3166_table, g_strdup (parts[1]),
				g_strdup (parts[1]));
			*country_name = g_hash_table_lookup (iso_3166_table, parts[1]);
		}
	}

	g_strfreev (parts);
}
