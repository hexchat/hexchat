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

#ifndef GTK_SPELL_CODETABLE_H
#define GTK_SPELL_CODETABLE_H

#include <glib.h>

G_BEGIN_DECLS

void codetable_init (void);
void codetable_free (void);
void codetable_lookup (const gchar *language_code,
	const gchar **language_name,
	const gchar **country_name);

G_END_DECLS

#endif /* GTK_SPELL_CODETABLE_H */