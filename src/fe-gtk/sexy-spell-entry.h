/* libsexy
 * Copyright (C) 2004-2006 Christian Hammond.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef _SEXY_SPELL_ENTRY_H_
#define _SEXY_SPELL_ENTRY_H_

typedef struct _SexySpellEntry      SexySpellEntry;
typedef struct _SexySpellEntryClass SexySpellEntryClass;
typedef struct _SexySpellEntryPriv  SexySpellEntryPriv;

#include <gtk/gtk.h>

#define SEXY_TYPE_SPELL_ENTRY            (sexy_spell_entry_get_type())
#define SEXY_SPELL_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SEXY_TYPE_SPELL_ENTRY, SexySpellEntry))
#define SEXY_SPELL_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SEXY_TYPE_SPELL_ENTRY, SexySpellEntryClass))
#define SEXY_IS_SPELL_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SEXY_TYPE_SPELL_ENTRY))
#define SEXY_IS_SPELL_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SEXY_TYPE_SPELL_ENTRY))
#define SEXY_SPELL_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), SEXY_TYPE_SPELL_ENTRY, SexySpellEntryClass))

#define SEXY_SPELL_ERROR                 (sexy_spell_error_quark())

typedef enum {
	SEXY_SPELL_ERROR_BACKEND,
} SexySpellError;

struct _SexySpellEntry
{
	GtkEntry parent_object;

	SexySpellEntryPriv *priv;

	void (*gtk_reserved1)(void);
	void (*gtk_reserved2)(void);
	void (*gtk_reserved3)(void);
	void (*gtk_reserved4)(void);
};

struct _SexySpellEntryClass
{
	GtkEntryClass parent_class;

	/* Signals */
	gboolean (*word_check)(SexySpellEntry *entry, const gchar *word);

	void (*gtk_reserved1)(void);
	void (*gtk_reserved2)(void);
	void (*gtk_reserved3)(void);
	void (*gtk_reserved4)(void);
};

G_BEGIN_DECLS

GType      sexy_spell_entry_get_type(void);
GtkWidget *sexy_spell_entry_new(void);
GQuark     sexy_spell_error_quark(void);

GSList    *sexy_spell_entry_get_languages(const SexySpellEntry *entry);
gchar     *sexy_spell_entry_get_language_name(const SexySpellEntry *entry, const gchar *lang);
gboolean   sexy_spell_entry_language_is_active(const SexySpellEntry *entry, const gchar *lang);
gboolean   sexy_spell_entry_activate_language(SexySpellEntry *entry, const gchar *lang, GError **error);
void       sexy_spell_entry_deactivate_language(SexySpellEntry *entry, const gchar *lang);
gboolean   sexy_spell_entry_set_active_languages(SexySpellEntry *entry, GSList *langs, GError **error);
GSList    *sexy_spell_entry_get_active_languages(SexySpellEntry *entry);
gboolean   sexy_spell_entry_is_checked(SexySpellEntry *entry);
void       sexy_spell_entry_set_checked(SexySpellEntry *entry, gboolean checked);
void       sexy_spell_entry_set_parse_attributes (SexySpellEntry *entry, gboolean parse);
void       sexy_spell_entry_activate_default_languages(SexySpellEntry *entry);

G_END_DECLS

#endif
