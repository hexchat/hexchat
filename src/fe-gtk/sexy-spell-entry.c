/*
 * @file libsexy/sexy-icon-entry.c Entry widget
 *
 * @Copyright (C) 2004-2006 Christian Hammond.
 * Some of this code is from gtkspell, Copyright (C) 2002 Evan Martin.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include "sexy-spell-entry.h"
#include <string.h>
#include <glib/gi18n.h>
#include <sys/types.h>
/*#include "gtkspell-iso-codes.h"
#include "sexy-marshal.h"*/

/*
 * Bunch of poop to make enchant into a runtime dependency rather than a
 * compile-time dependency.  This makes it so I don't have to hear the
 * complaints from people with binary distributions who don't get spell
 * checking because they didn't check their configure output.
 */
struct EnchantDict;
struct EnchantBroker;

typedef void (*EnchantDictDescribeFn) (const char * const lang_tag,
                                       const char * const provider_name,
                                       const char * const provider_desc,
                                       const char * const provider_file,
                                       void * user_data);

static struct EnchantBroker * (*enchant_broker_init) (void);
static void (*enchant_broker_free) (struct EnchantBroker * broker);
static void (*enchant_broker_free_dict) (struct EnchantBroker * broker, struct EnchantDict * dict);
static void (*enchant_broker_list_dicts) (struct EnchantBroker * broker, EnchantDictDescribeFn fn, void * user_data);
static struct EnchantDict * (*enchant_broker_request_dict) (struct EnchantBroker * broker, const char *const tag);

static void (*enchant_dict_add_to_personal) (struct EnchantDict * dict, const char *const word, ssize_t len);
static void (*enchant_dict_add_to_session) (struct EnchantDict * dict, const char *const word, ssize_t len);
static int (*enchant_dict_check) (struct EnchantDict * dict, const char *const word, ssize_t len);
static void (*enchant_dict_describe) (struct EnchantDict * dict, EnchantDictDescribeFn fn, void * user_data);
static void (*enchant_dict_free_suggestions) (struct EnchantDict * dict, char **suggestions);
static void (*enchant_dict_store_replacement) (struct EnchantDict * dict, const char *const mis, ssize_t mis_len, const char *const cor, ssize_t cor_len);
static char ** (*enchant_dict_suggest) (struct EnchantDict * dict, const char *const word, ssize_t len, size_t * out_n_suggs);
static gboolean have_enchant = FALSE;

struct _SexySpellEntryPriv
{
	struct EnchantBroker *broker;
	PangoAttrList        *attr_list;
	gint                  mark_character;
	GHashTable           *dict_hash;
	GSList               *dict_list;
	gchar               **words;
	gint                 *word_starts;
	gint                 *word_ends;
	gboolean              checked;
};

static void sexy_spell_entry_class_init(SexySpellEntryClass *klass);
static void sexy_spell_entry_editable_init (GtkEditableClass *iface);
static void sexy_spell_entry_init(SexySpellEntry *entry);
static void sexy_spell_entry_finalize(GObject *obj);
static void sexy_spell_entry_destroy(GtkObject *obj);
static gint sexy_spell_entry_expose(GtkWidget *widget, GdkEventExpose *event);
static gint sexy_spell_entry_button_press(GtkWidget *widget, GdkEventButton *event);

/* GtkEditable handlers */
static void sexy_spell_entry_changed(GtkEditable *editable, gpointer data);

/* Other handlers */
static gboolean sexy_spell_entry_popup_menu(GtkWidget *widget, SexySpellEntry *entry);

/* Internal utility functions */
static gint       gtk_entry_find_position                     (GtkEntry             *entry,
                                                               gint                  x);
static gboolean   word_misspelled                             (SexySpellEntry       *entry,
                                                               int                   start,
                                                               int                   end);
static gboolean   default_word_check                          (SexySpellEntry       *entry,
                                                               const gchar          *word);
static gboolean   sexy_spell_entry_activate_language_internal (SexySpellEntry       *entry,
                                                               const gchar          *lang,
                                                               GError              **error);
static gchar     *get_lang_from_dict                          (struct EnchantDict   *dict);
static void       sexy_spell_entry_recheck_all                (SexySpellEntry       *entry);
static void       entry_strsplit_utf8                         (GtkEntry             *entry,
                                                               gchar              ***set,
                                                               gint                **starts,
                                                               gint                **ends);

static GtkEntryClass *parent_class = NULL;

G_DEFINE_TYPE_EXTENDED(SexySpellEntry, sexy_spell_entry, GTK_TYPE_ENTRY, 0, G_IMPLEMENT_INTERFACE(GTK_TYPE_EDITABLE, sexy_spell_entry_editable_init));

enum
{
	WORD_CHECK,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0};

static gboolean
spell_accumulator(GSignalInvocationHint *hint, GValue *return_accu, const GValue *handler_return, gpointer data)
{
	gboolean ret = g_value_get_boolean(handler_return);
	/* Handlers return TRUE if the word is misspelled.  In this
	 * case, it means that we want to stop if the word is checked
	 * as correct */
	g_value_set_boolean (return_accu, ret);
	return ret;
}

static void
initialize_enchant ()
{
	GModule *enchant;
	gpointer funcptr;

	enchant = g_module_open("libenchant.so.1", 0);
	if (enchant == NULL)
	{
		enchant = g_module_open("libenchant.so", 0);
		if (enchant == NULL)
			return;
	}

	have_enchant = TRUE;

#define MODULE_SYMBOL(name, func) \
	g_module_symbol(enchant, (name), &funcptr); \
	(func) = funcptr;

	MODULE_SYMBOL("enchant_broker_init", enchant_broker_init)
	MODULE_SYMBOL("enchant_broker_free", enchant_broker_free)
	MODULE_SYMBOL("enchant_broker_free_dict", enchant_broker_free_dict)
	MODULE_SYMBOL("enchant_broker_list_dicts", enchant_broker_list_dicts)
	MODULE_SYMBOL("enchant_broker_request_dict", enchant_broker_request_dict)

	MODULE_SYMBOL("enchant_dict_add_to_personal", enchant_dict_add_to_personal)
	MODULE_SYMBOL("enchant_dict_add_to_session", enchant_dict_add_to_session)
	MODULE_SYMBOL("enchant_dict_check", enchant_dict_check)
	MODULE_SYMBOL("enchant_dict_describe", enchant_dict_describe)
	MODULE_SYMBOL("enchant_dict_free_suggestions",
				  enchant_dict_free_suggestions)
	MODULE_SYMBOL("enchant_dict_store_replacement",
				  enchant_dict_store_replacement)
	MODULE_SYMBOL("enchant_dict_suggest", enchant_dict_suggest)

#undef MODULE_SYMBOL
}

static void
sexy_spell_entry_class_init(SexySpellEntryClass *klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkEntryClass *entry_class;

	initialize_enchant();

	parent_class = g_type_class_peek_parent(klass);

	gobject_class = G_OBJECT_CLASS(klass);
	object_class  = GTK_OBJECT_CLASS(klass);
	widget_class  = GTK_WIDGET_CLASS(klass);
	entry_class   = GTK_ENTRY_CLASS(klass);

	if (have_enchant)
		klass->word_check = default_word_check;

	gobject_class->finalize = sexy_spell_entry_finalize;

	object_class->destroy = sexy_spell_entry_destroy;

	widget_class->expose_event = sexy_spell_entry_expose;
	widget_class->button_press_event = sexy_spell_entry_button_press;

	/**
	 * SexySpellEntry::word-check:
	 * @entry: The entry on which the signal is emitted.
	 * @word: The word to check.
	 *
	 * The ::word-check signal is emitted whenever the entry has to check
	 * a word.  This allows the application to mark words as correct even
	 * if none of the active dictionaries contain it, such as nicknames in
	 * a chat client.
	 *
	 * Returns: %FALSE to indicate that the word should be marked as
	 * correct.
	 */
/*	signals[WORD_CHECK] = g_signal_new("word_check",
					   G_TYPE_FROM_CLASS(object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET(SexySpellEntryClass, word_check),
					   (GSignalAccumulator) spell_accumulator, NULL,
					   sexy_marshal_BOOLEAN__STRING,
					   G_TYPE_BOOLEAN,
					   1, G_TYPE_STRING);*/
}

static void
sexy_spell_entry_editable_init (GtkEditableClass *iface)
{
}

static gint
gtk_entry_find_position (GtkEntry *entry, gint x)
{
	PangoLayout *layout;
	PangoLayoutLine *line;
	const gchar *text;
	gint cursor_index;
	gint index;
	gint pos;
	gboolean trailing;

	x = x + entry->scroll_offset;

	layout = gtk_entry_get_layout(entry);
	text = pango_layout_get_text(layout);
	cursor_index = g_utf8_offset_to_pointer(text, entry->current_pos) - text;

	line = pango_layout_get_lines(layout)->data;
	pango_layout_line_x_to_index(line, x * PANGO_SCALE, &index, &trailing);

	if (index >= cursor_index && entry->preedit_length) {
		if (index >= cursor_index + entry->preedit_length) {
			index -= entry->preedit_length;
		} else {
			index = cursor_index;
			trailing = FALSE;
		}
	}

	pos = g_utf8_pointer_to_offset (text, text + index);
	pos += trailing;

	return pos;
}

static void
insert_underline(SexySpellEntry *entry, guint start, guint end)
{
	PangoAttribute *ucolor = pango_attr_underline_color_new (65535, 0, 0);
	PangoAttribute *unline = pango_attr_underline_new (PANGO_UNDERLINE_ERROR);

	ucolor->start_index = start;
	unline->start_index = start;

	ucolor->end_index = end;
	unline->end_index = end;

	pango_attr_list_insert (entry->priv->attr_list, ucolor);
	pango_attr_list_insert (entry->priv->attr_list, unline);
}

static void
get_word_extents_from_position(SexySpellEntry *entry, gint *start, gint *end, guint position)
{
	const gchar *text;
	gint i, bytes_pos;

	*start = -1;
	*end = -1;

	if (entry->priv->words == NULL)
		return;

	text = gtk_entry_get_text(GTK_ENTRY(entry));
	bytes_pos = (gint) (g_utf8_offset_to_pointer(text, position) - text);

	for (i = 0; entry->priv->words[i]; i++) {
		if (bytes_pos >= entry->priv->word_starts[i] &&
		    bytes_pos <= entry->priv->word_ends[i]) {
			*start = entry->priv->word_starts[i];
			*end   = entry->priv->word_ends[i];
			return;
		}
	}
}

static void
add_to_dictionary(GtkWidget *menuitem, SexySpellEntry *entry)
{
	char *word;
	gint start, end;
	struct EnchantDict *dict;

	if (!have_enchant)
		return;

	get_word_extents_from_position(entry, &start, &end, entry->priv->mark_character);
	word = gtk_editable_get_chars(GTK_EDITABLE(entry), start, end);

	dict = (struct EnchantDict *) g_object_get_data(G_OBJECT(menuitem), "gtkspell-dict");
	if (dict)
		enchant_dict_add_to_personal(dict, word, g_utf8_strlen(word, -1));

	g_free(word);

	if (entry->priv->words) {
		g_strfreev(entry->priv->words);
		g_free(entry->priv->word_starts);
		g_free(entry->priv->word_ends);
	}
	entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
}

static void
ignore_all(GtkWidget *menuitem, SexySpellEntry *entry)
{
	char *word;
	gint start, end;
	GSList *li;

	if (!have_enchant)
		return;

	get_word_extents_from_position(entry, &start, &end, entry->priv->mark_character);
	word = gtk_editable_get_chars(GTK_EDITABLE(entry), start, end);

	for (li = entry->priv->dict_list; li; li = g_slist_next (li)) {
		struct EnchantDict *dict = (struct EnchantDict *) li->data;
		enchant_dict_add_to_session(dict, word, g_utf8_strlen(word, -1));
	}

	g_free(word);

	if (entry->priv->words) {
		g_strfreev(entry->priv->words);
		g_free(entry->priv->word_starts);
		g_free(entry->priv->word_ends);
	}
	entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
}

static void
replace_word(GtkWidget *menuitem, SexySpellEntry *entry)
{
	char *oldword;
	const char *newword;
	gint start, end;
	gint cursor;
	struct EnchantDict *dict;

	if (!have_enchant)
		return;

	get_word_extents_from_position(entry, &start, &end, entry->priv->mark_character);
	oldword = gtk_editable_get_chars(GTK_EDITABLE(entry), start, end);
	newword = gtk_label_get_text(GTK_LABEL(GTK_BIN(menuitem)->child));

	cursor = gtk_editable_get_position(GTK_EDITABLE(entry));
	/* is the cursor at the end? If so, restore it there */
	if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(entry)), -1) == cursor)
		cursor = -1;
	else if(cursor > start && cursor <= end)
		cursor = start;

	gtk_editable_delete_text(GTK_EDITABLE(entry), start, end);
	gtk_editable_set_position(GTK_EDITABLE(entry), start);
	gtk_editable_insert_text(GTK_EDITABLE(entry), newword, strlen(newword),
							 &start);
	gtk_editable_set_position(GTK_EDITABLE(entry), cursor);

	dict = (struct EnchantDict *) g_object_get_data(G_OBJECT(menuitem), "gtkspell-dict");

        if (dict)
		enchant_dict_store_replacement(dict,
					       oldword, g_utf8_strlen(oldword, -1),
					       newword, g_utf8_strlen(newword, -1));

	g_free(oldword);
}

static void
build_suggestion_menu(SexySpellEntry *entry, GtkWidget *menu, struct EnchantDict *dict, const gchar *word)
{
	GtkWidget *mi;
	gchar **suggestions;
	size_t n_suggestions, i;

	if (!have_enchant)
		return;

	suggestions = enchant_dict_suggest(dict, word, g_utf8_strlen(word, -1), &n_suggestions);

	if (suggestions == NULL || n_suggestions == 0) {
		/* no suggestions.  put something in the menu anyway... */
		GtkWidget *label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label), _("<i>(no suggestions)</i>"));

		mi = gtk_separator_menu_item_new();
		gtk_container_add(GTK_CONTAINER(mi), label);
		gtk_widget_show_all(mi);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), mi);
	} else {
		/* build a set of menus with suggestions */
		for (i = 0; i < n_suggestions; i++) {
			if ((i != 0) && (i % 10 == 0)) {
				mi = gtk_separator_menu_item_new();
				gtk_widget_show(mi);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

				mi = gtk_menu_item_new_with_label(_("More..."));
				gtk_widget_show(mi);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

				menu = gtk_menu_new();
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), menu);
			}

			mi = gtk_menu_item_new_with_label(suggestions[i]);
			g_object_set_data(G_OBJECT(mi), "gtkspell-dict", dict);
			g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(replace_word), entry);
			gtk_widget_show(mi);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
		}
	}

	enchant_dict_free_suggestions(dict, suggestions);
}

static GtkWidget *
build_spelling_menu(SexySpellEntry *entry, const gchar *word)
{
	struct EnchantDict *dict;
	GtkWidget *topmenu, *mi;
	gchar *label;

	if (!have_enchant)
		return NULL;

	topmenu = gtk_menu_new();

	if (entry->priv->dict_list == NULL)
		return topmenu;

#if 1
	dict = (struct EnchantDict *) entry->priv->dict_list->data;
	build_suggestion_menu(entry, topmenu, dict, word);
#else
	/* Suggestions */
	if (g_slist_length(entry->priv->dict_list) == 1) {
		dict = (struct EnchantDict *) entry->priv->dict_list->data;
		build_suggestion_menu(entry, topmenu, dict, word);
	} else {
		GSList *li;
		GtkWidget *menu;
		gchar *lang, *lang_name;

		for (li = entry->priv->dict_list; li; li = g_slist_next (li)) {
			dict = (struct EnchantDict *) li->data;
			lang = get_lang_from_dict(dict);
			lang_name = gtkspell_iso_codes_lookup_name_for_code(lang);
			if (lang_name) {
				mi = gtk_menu_item_new_with_label(lang_name);
				g_free(lang_name);
			} else {
				mi = gtk_menu_item_new_with_label(lang);
			}
			g_free(lang);

			gtk_widget_show(mi);
			gtk_menu_shell_append(GTK_MENU_SHELL(topmenu), mi);
			menu = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), menu);
			build_suggestion_menu(entry, menu, dict, word);
		}
	}
#endif

	/* Separator */
	mi = gtk_separator_menu_item_new ();
	gtk_widget_show(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(topmenu), mi);

	/* + Add to Dictionary */
	label = g_strdup_printf(_("Add \"%s\" to Dictionary"), word);
	mi = gtk_image_menu_item_new_with_label(label);
	g_free(label);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));

#if 1
	dict = (struct EnchantDict *) entry->priv->dict_list->data;
	g_object_set_data(G_OBJECT(mi), "gtkspell-dict", dict);
	g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(add_to_dictionary), entry);
#else
	if (g_slist_length(entry->priv->dict_list) == 1) {
		dict = (struct EnchantDict *) entry->priv->dict_list->data;
		g_object_set_data(G_OBJECT(mi), "gtkspell-dict", dict);
		g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(add_to_dictionary), entry);
	} else {
		GSList *li;
		GtkWidget *menu, *submi;
		gchar *lang, *lang_name;

		menu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), menu);

		for (li = entry->priv->dict_list; li; li = g_slist_next(li)) {
			dict = (struct EnchantDict *)li->data;
			lang = get_lang_from_dict(dict);
			lang_name = gtkspell_iso_codes_lookup_name_for_code(lang);
			if (lang_name) {
				submi = gtk_menu_item_new_with_label(lang_name);
				g_free(lang_name);
			} else {
				submi = gtk_menu_item_new_with_label(lang);
			}
			g_free(lang);
			g_object_set_data(G_OBJECT(submi), "gtkspell-dict", dict);

			g_signal_connect(G_OBJECT(submi), "activate", G_CALLBACK(add_to_dictionary), entry);

			gtk_widget_show(submi);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), submi);
		}
	}
#endif

	gtk_widget_show_all(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(topmenu), mi);

	/* - Ignore All */
	mi = gtk_image_menu_item_new_with_label(_("Ignore All"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(ignore_all), entry);
	gtk_widget_show_all(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(topmenu), mi);

	return topmenu;
}

static void
sexy_spell_entry_populate_popup(SexySpellEntry *entry, GtkMenu *menu, gpointer data)
{
	GtkWidget *icon, *mi;
	gint start, end;
	gchar *word;

	if ((have_enchant == FALSE) || (entry->priv->checked == FALSE))
		return;

	if (g_slist_length(entry->priv->dict_list) == 0)
		return;

	get_word_extents_from_position(entry, &start, &end, entry->priv->mark_character);
	if (start == end)
		return;
	if (!word_misspelled(entry, start, end))
		return;

	/* separator */
	mi = gtk_separator_menu_item_new();
	gtk_widget_show(mi);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), mi);

	/* Above the separator, show the suggestions menu */
	icon = gtk_image_new_from_stock(GTK_STOCK_SPELL_CHECK, GTK_ICON_SIZE_MENU);
	mi = gtk_image_menu_item_new_with_label(_("Spelling Suggestions"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi), icon);

	word = gtk_editable_get_chars(GTK_EDITABLE(entry), start, end);
	g_assert(word != NULL);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), build_spelling_menu(entry, word));
	g_free(word);

	gtk_widget_show_all(mi);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), mi);
}

static void
sexy_spell_entry_init(SexySpellEntry *entry)
{
	entry->priv = g_new0(SexySpellEntryPriv, 1);

	entry->priv->dict_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	if (have_enchant)
		sexy_spell_entry_activate_default_languages(entry);

	entry->priv->attr_list = pango_attr_list_new();

	entry->priv->checked = TRUE;

	g_signal_connect(G_OBJECT(entry), "popup-menu", G_CALLBACK(sexy_spell_entry_popup_menu), entry);
	g_signal_connect(G_OBJECT(entry), "populate-popup", G_CALLBACK(sexy_spell_entry_populate_popup), NULL);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(sexy_spell_entry_changed), NULL);
}

static void
sexy_spell_entry_finalize(GObject *obj)
{
	SexySpellEntry *entry;

	g_return_if_fail(obj != NULL);
	g_return_if_fail(SEXY_IS_SPELL_ENTRY(obj));

	entry = SEXY_SPELL_ENTRY(obj);

	if (entry->priv->attr_list)
		pango_attr_list_unref(entry->priv->attr_list);
	if (entry->priv->dict_hash)
		g_hash_table_destroy(entry->priv->dict_hash);
	if (entry->priv->words)
		g_strfreev(entry->priv->words);
	if (entry->priv->word_starts)
		g_free(entry->priv->word_starts);
	if (entry->priv->word_ends)
		g_free(entry->priv->word_ends);

	if (have_enchant) {
		if (entry->priv->broker) {
			GSList *li;
			for (li = entry->priv->dict_list; li; li = g_slist_next(li)) {
				struct EnchantDict *dict = (struct EnchantDict*) li->data;
				enchant_broker_free_dict (entry->priv->broker, dict);
			}
			g_slist_free (entry->priv->dict_list);

			enchant_broker_free(entry->priv->broker);
		}
	}

	g_free(entry->priv);

	if (G_OBJECT_CLASS(parent_class)->finalize)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void
sexy_spell_entry_destroy(GtkObject *obj)
{
	SexySpellEntry *entry;

	entry = SEXY_SPELL_ENTRY(obj);

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		GTK_OBJECT_CLASS(parent_class)->destroy(obj);
}

/**
 * sexy_spell_entry_new
 *
 * Creates a new SexySpellEntry widget.
 *
 * Returns: a new #SexySpellEntry.
 */
GtkWidget *
sexy_spell_entry_new(void)
{
	return GTK_WIDGET(g_object_new(SEXY_TYPE_SPELL_ENTRY, NULL));
}

GQuark
sexy_spell_error_quark(void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string("sexy-spell-error-quark");
	return q;
}

static gboolean
default_word_check(SexySpellEntry *entry, const gchar *word)
{
	gboolean result = TRUE;
	GSList *li;

	if (!have_enchant)
		return result;

	if (g_unichar_isalpha(*word) == FALSE) {
		/* We only want to check words */
		return FALSE;
	}
	for (li = entry->priv->dict_list; li; li = g_slist_next (li)) {
		struct EnchantDict *dict = (struct EnchantDict *) li->data;
		if (enchant_dict_check(dict, word, strlen(word)) == 0) {
			result = FALSE;
			break;
		}
	}
	return result;
}

static gboolean
word_misspelled(SexySpellEntry *entry, int start, int end)
{
	const gchar *text;
	gchar *word;
	gboolean ret;

	if (start == end)
		return FALSE;
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	word = g_new0(gchar, end - start + 2);

	g_strlcpy(word, text + start, end - start + 1);

#if 0
	g_signal_emit(entry, signals[WORD_CHECK], 0, word, &ret);
#else
	ret = default_word_check (entry, word);
#endif

	g_free(word);
	return ret;
}

static void
check_word(SexySpellEntry *entry, int start, int end)
{
	PangoAttrIterator *it;

	/* Check to see if we've got any attributes at this position.
	 * If so, free them, since we'll readd it if the word is misspelled */
	it = pango_attr_list_get_iterator(entry->priv->attr_list);
	if (it == NULL)
		return;
	do {
		gint s, e;
		pango_attr_iterator_range(it, &s, &e);
		if (s == start) {
			GSList *attrs = pango_attr_iterator_get_attrs(it);
			g_slist_foreach(attrs, (GFunc) pango_attribute_destroy, NULL);
			g_slist_free(attrs);
		}
	} while (pango_attr_iterator_next(it));
	pango_attr_iterator_destroy(it);

	if (word_misspelled(entry, start, end))
		insert_underline(entry, start, end);
}

static void
sexy_spell_entry_recheck_all(SexySpellEntry *entry)
{
	GdkRectangle rect;
	GtkWidget *widget = GTK_WIDGET(entry);
	PangoLayout *layout;
	int length, i;

	if ((have_enchant == FALSE) || (entry->priv->checked == FALSE))
		return;

	if (g_slist_length(entry->priv->dict_list) == 0)
		return;

	/* Remove all existing pango attributes.  These will get readded as we check */
	pango_attr_list_unref(entry->priv->attr_list);
	entry->priv->attr_list = pango_attr_list_new();

	/* Loop through words */
	for (i = 0; entry->priv->words[i]; i++) {
		length = strlen(entry->priv->words[i]);
		if (length == 0)
			continue;
		check_word(entry, entry->priv->word_starts[i], entry->priv->word_ends[i]);
	}

	layout = gtk_entry_get_layout(GTK_ENTRY(entry));
	pango_layout_set_attributes(layout, entry->priv->attr_list);

	if (GTK_WIDGET_REALIZED(GTK_WIDGET(entry))) {
		rect.x = 0; rect.y = 0;
		rect.width  = widget->allocation.width;
		rect.height = widget->allocation.height;
		gdk_window_invalidate_rect(widget->window, &rect, TRUE);
	}
}

static gint
sexy_spell_entry_expose(GtkWidget *widget, GdkEventExpose *event)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(widget);
	GtkEntry *gtk_entry = GTK_ENTRY(widget);
	PangoLayout *layout;

	if (entry->priv->checked) {
		layout = gtk_entry_get_layout(gtk_entry);
		pango_layout_set_attributes(layout, entry->priv->attr_list);
	}

	return GTK_WIDGET_CLASS(parent_class)->expose_event (widget, event);
}

static gint
sexy_spell_entry_button_press(GtkWidget *widget, GdkEventButton *event)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(widget);
	GtkEntry *gtk_entry = GTK_ENTRY(widget);
	gint pos;

	pos = gtk_entry_find_position(gtk_entry, event->x);
	entry->priv->mark_character = pos;

	return GTK_WIDGET_CLASS(parent_class)->button_press_event (widget, event);
}

static gboolean
sexy_spell_entry_popup_menu(GtkWidget *widget, SexySpellEntry *entry)
{
	/* Menu popped up from a keybinding (menu key or <shift>+F10). Use
	 * the cursor position as the mark position */
	entry->priv->mark_character = gtk_editable_get_position (GTK_EDITABLE (entry));
	return FALSE;
}

static void
entry_strsplit_utf8(GtkEntry *entry, gchar ***set, gint **starts, gint **ends)
{
	PangoLayout   *layout;
	PangoLogAttr  *log_attrs;
	const gchar   *text;
	gint           n_attrs, n_strings, i, j;

	layout = gtk_entry_get_layout(GTK_ENTRY(entry));
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	pango_layout_get_log_attrs(layout, &log_attrs, &n_attrs);

	/* Find how many words we have */
	n_strings = 0;
	for (i = 0; i < n_attrs; i++)
		if (log_attrs[i].is_word_start)
			n_strings++;

	*set    = g_new0(gchar *, n_strings + 1);
	*starts = g_new0(gint, n_strings);
	*ends   = g_new0(gint, n_strings);

	/* Copy out strings */
	for (i = 0, j = 0; i < n_attrs; i++) {
		if (log_attrs[i].is_word_start) {
			gint cend, bytes;
			gchar *start;

			/* Find the end of this string */
			cend = i;
			while (!(log_attrs[cend].is_word_end))
				cend++;

			/* Copy sub-string */
			start = g_utf8_offset_to_pointer(text, i);
			bytes = (gint) (g_utf8_offset_to_pointer(text, cend) - start);
			(*set)[j]    = g_new0(gchar, bytes + 1);
			(*starts)[j] = (gint) (start - text);
			(*ends)[j]   = (gint) (start - text + bytes);
			g_utf8_strncpy((*set)[j], start, cend - i);

			/* Move on to the next word */
			j++;
		}
	}

	g_free (log_attrs);
}

static void
sexy_spell_entry_changed(GtkEditable *editable, gpointer data)
{
	SexySpellEntry *entry = SEXY_SPELL_ENTRY(editable);
	if (entry->priv->checked == FALSE)
		return;
	if (g_slist_length(entry->priv->dict_list) == 0)
		return;

	if (entry->priv->words) {
		g_strfreev(entry->priv->words);
		g_free(entry->priv->word_starts);
		g_free(entry->priv->word_ends);
	}
	entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
}

static gboolean
enchant_has_lang(const gchar *lang, GSList *langs) {
	GSList *i;
	for (i = langs; i; i = g_slist_next(i)) {
		if (strcmp(lang, i->data) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * sexy_spell_entry_activate_default_languages:
 * @entry: A #SexySpellEntry.
 *
 * Activate spell checking for languages specified in the $LANG
 * or $LANGUAGE environment variables.  These languages are
 * activated by default, so this function need only be called
 * if they were previously deactivated.
 */
void
sexy_spell_entry_activate_default_languages(SexySpellEntry *entry)
{
#if GLIB_CHECK_VERSION (2, 6, 0)
	const gchar* const *langs;
	int i;
	gchar *lastprefix = NULL;
	GSList *enchant_langs;

	if (!have_enchant)
		return;

	if (!entry->priv->broker)
		entry->priv->broker = enchant_broker_init();


	langs = g_get_language_names ();

	if (langs == NULL)
		return;

	enchant_langs = sexy_spell_entry_get_languages(entry);

	for (i = 0; langs[i]; i++) {
		if ((g_strncasecmp(langs[i], "C", 1) != 0) &&
		    (strlen(langs[i]) >= 2) &&
		    enchant_has_lang(langs[i], enchant_langs)) {
			if ((lastprefix == NULL) || (g_str_has_prefix(langs[i], lastprefix) == FALSE))
				sexy_spell_entry_activate_language_internal(entry, langs[i], NULL);
			if (lastprefix != NULL)
				g_free(lastprefix);
			lastprefix = g_strndup(langs[i], 2);
		}
	}
	if (lastprefix != NULL)
		g_free(lastprefix);

	g_slist_foreach(enchant_langs, (GFunc) g_free, NULL);
	g_slist_free(enchant_langs);

	/* If we don't have any languages activated, use "en" */
	if (entry->priv->dict_list == NULL)
		sexy_spell_entry_activate_language_internal(entry, "en", NULL);
#else
	gchar *lang;

	if (!have_enchant)
		return;

	lang = (gchar *) g_getenv("LANG");

	if (lang != NULL) {
		if (g_strncasecmp(lang, "C", 1) == 0)
			lang = NULL;
		else if (lang[0] == '\0')
			lang = NULL;
	}

	if (lang == NULL)
		lang = "en";

	sexy_spell_entry_activate_language_internal(entry, lang, NULL);
#endif
}

static void
get_lang_from_dict_cb(const char * const lang_tag,
		      const char * const provider_name,
		      const char * const provider_desc,
		      const char * const provider_file,
		      void * user_data) {
	gchar **lang = (gchar **)user_data;
	*lang = g_strdup(lang_tag);
}

static gchar *
get_lang_from_dict(struct EnchantDict *dict)
{
	gchar *lang;

	if (!have_enchant)
		return NULL;

	enchant_dict_describe(dict, get_lang_from_dict_cb, &lang);
	return lang;
}

static gboolean
sexy_spell_entry_activate_language_internal(SexySpellEntry *entry, const gchar *lang, GError **error)
{
	struct EnchantDict *dict;

	if (!have_enchant)
		return FALSE;

	if (!entry->priv->broker)
		entry->priv->broker = enchant_broker_init();

	if (g_hash_table_lookup(entry->priv->dict_hash, lang))
		return TRUE;

	dict = enchant_broker_request_dict(entry->priv->broker, lang);

	if (!dict) {
		g_set_error(error, SEXY_SPELL_ERROR, SEXY_SPELL_ERROR_BACKEND, _("enchant error for language: %s"), lang);
		return FALSE;
	}

	entry->priv->dict_list = g_slist_append(entry->priv->dict_list, (gpointer) dict);
	g_hash_table_insert(entry->priv->dict_hash, get_lang_from_dict(dict), (gpointer) dict);

	return TRUE;
}

static void
dict_describe_cb(const char * const lang_tag,
		 const char * const provider_name,
		 const char * const provider_desc,
		 const char * const provider_file,
		 void * user_data)
{
	GSList **langs = (GSList **)user_data;

	*langs = g_slist_append(*langs, (gpointer)g_strdup(lang_tag));
}

/**
 * sexy_spell_entry_get_languages:
 * @entry: A #SexySpellEntry.
 *
 * Retrieve a list of language codes for which dictionaries are available.
 *
 * Returns: a new #GList object, or %NULL on error.
 */
GSList *
sexy_spell_entry_get_languages(const SexySpellEntry *entry)
{
	GSList *langs = NULL;

	g_return_val_if_fail(entry != NULL, NULL);
	g_return_val_if_fail(SEXY_IS_SPELL_ENTRY(entry), NULL);

	if (enchant_broker_list_dicts == NULL)
		return NULL;

	if (!entry->priv->broker)
		return NULL;

	enchant_broker_list_dicts(entry->priv->broker, dict_describe_cb, &langs);

	return langs;
}

/**
 * sexy_spell_entry_get_language_name:
 * @entry: A #SexySpellEntry.
 * @lang: The language code to lookup a friendly name for.
 *
 * Get a friendly name for a given locale.
 *
 * Returns: The name of the locale. Should be freed with g_free()
 */
gchar *
sexy_spell_entry_get_language_name(const SexySpellEntry *entry,
								   const gchar *lang)
{
	/*if (have_enchant)
		return gtkspell_iso_codes_lookup_name_for_code(lang);*/
	return NULL;
}

/**
 * sexy_spell_entry_language_is_active:
 * @entry: A #SexySpellEntry.
 * @lang: The language to use, in a form enchant understands.
 *
 * Determine if a given language is currently active.
 *
 * Returns: TRUE if the language is active.
 */
gboolean
sexy_spell_entry_language_is_active(const SexySpellEntry *entry,
									const gchar *lang)
{
	return (g_hash_table_lookup(entry->priv->dict_hash, lang) != NULL);
}

/**
 * sexy_spell_entry_activate_language:
 * @entry: A #SexySpellEntry
 * @lang: The language to use in a form Enchant understands. Typically either
 *        a two letter language code or a locale code in the form xx_XX.
 * @error: Return location for error.
 *
 * Activate spell checking for the language specifed.
 *
 * Returns: FALSE if there was an error.
 */
gboolean
sexy_spell_entry_activate_language(SexySpellEntry *entry, const gchar *lang, GError **error)
{
	gboolean ret;

	g_return_val_if_fail(entry != NULL, FALSE);
	g_return_val_if_fail(SEXY_IS_SPELL_ENTRY(entry), FALSE);
	g_return_val_if_fail(lang != NULL && lang != '\0', FALSE);

	if (!have_enchant)
		return FALSE;

	if (error)
		g_return_val_if_fail(*error == NULL, FALSE);

	ret = sexy_spell_entry_activate_language_internal(entry, lang, error);

	if (ret) {
		if (entry->priv->words) {
			g_strfreev(entry->priv->words);
			g_free(entry->priv->word_starts);
			g_free(entry->priv->word_ends);
		}
		entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
		sexy_spell_entry_recheck_all(entry);
	}

	return ret;
}

/**
 * sexy_spell_entry_deactivate_language:
 * @entry: A #SexySpellEntry.
 * @lang: The language in a form Enchant understands. Typically either
 *        a two letter language code or a locale code in the form xx_XX.
 *
 * Deactivate spell checking for the language specifed.
 */
void
sexy_spell_entry_deactivate_language(SexySpellEntry *entry, const gchar *lang)
{
	g_return_if_fail(entry != NULL);
	g_return_if_fail(SEXY_IS_SPELL_ENTRY(entry));

	if (!have_enchant)
		return;

	if (!entry->priv->dict_list)
		return;

	if (lang) {
		struct EnchantDict *dict;

		dict = g_hash_table_lookup(entry->priv->dict_hash, lang);
		if (!dict)
			return;
		enchant_broker_free_dict(entry->priv->broker, dict);
		entry->priv->dict_list = g_slist_remove(entry->priv->dict_list, dict);
		g_hash_table_remove (entry->priv->dict_hash, lang);
	} else {
		/* deactivate all */
		GSList *li;
		struct EnchantDict *dict;

		for (li = entry->priv->dict_list; li; li = g_slist_next(li)) {
			dict = (struct EnchantDict *)li->data;
			enchant_broker_free_dict(entry->priv->broker, dict);
		}

		g_slist_free (entry->priv->dict_list);
		g_hash_table_destroy (entry->priv->dict_hash);
		entry->priv->dict_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		entry->priv->dict_list = NULL;
	}

	if (entry->priv->words) {
		g_strfreev(entry->priv->words);
		g_free(entry->priv->word_starts);
		g_free(entry->priv->word_ends);
	}
	entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
}

/**
 * sexy_spell_entry_set_active_languages:
 * @entry: A #SexySpellEntry
 * @langs: A list of language codes to activate, in a form Enchant understands.
 *         Typically either a two letter language code or a locale code in the
 *         form xx_XX.
 * @error: Return location for error.
 *
 * Activate spell checking for only the languages specified.
 *
 * Returns: FALSE if there was an error.
 */
gboolean
sexy_spell_entry_set_active_languages(SexySpellEntry *entry, GSList *langs, GError **error)
{
	GSList *li;

	g_return_val_if_fail(entry != NULL, FALSE);
	g_return_val_if_fail(SEXY_IS_SPELL_ENTRY(entry), FALSE);
	g_return_val_if_fail(langs != NULL, FALSE);

	if (!have_enchant)
		return FALSE;

	/* deactivate all languages first */
	sexy_spell_entry_deactivate_language(entry, NULL);

	for (li = langs; li; li = g_slist_next(li)) {
		if (sexy_spell_entry_activate_language_internal(entry,
		    (const gchar *) li->data, error) == FALSE)
			return FALSE;
	}
	if (entry->priv->words) {
		g_strfreev(entry->priv->words);
		g_free(entry->priv->word_starts);
		g_free(entry->priv->word_ends);
	}
	entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
	sexy_spell_entry_recheck_all(entry);
	return TRUE;
}

/**
 * sexy_spell_entry_get_active_languages:
 * @entry: A #SexySpellEntry
 *
 * Retrieve a list of the currently active languages.
 *
 * Returns: A GSList of char* values with language codes (en, fr, etc).  Both
 *          the data and the list must be freed by the user.
 */
GSList *
sexy_spell_entry_get_active_languages(SexySpellEntry *entry)
{
	GSList *ret = NULL, *li;
	struct EnchantDict *dict;
	gchar *lang;

	g_return_val_if_fail(entry != NULL, NULL);
	g_return_val_if_fail(SEXY_IS_SPELL_ENTRY(entry), NULL);

	if (!have_enchant)
		return NULL;

	for (li = entry->priv->dict_list; li; li = g_slist_next(li)) {
		dict = (struct EnchantDict *) li->data;
		lang = get_lang_from_dict(dict);
		ret = g_slist_append(ret, lang);
	}
	return ret;
}

/**
 * sexy_spell_entry_is_checked:
 * @entry: A #SexySpellEntry.
 *
 * Queries a #SexySpellEntry and returns whether the entry has spell-checking enabled.
 *
 * Returns: TRUE if the entry has spell-checking enabled.
 */
gboolean
sexy_spell_entry_is_checked(SexySpellEntry *entry)
{
	return entry->priv->checked;
}

/**
 * sexy_spell_entry_set_checked:
 * @entry: A #SexySpellEntry.
 * @checked: Whether to enable spell-checking
 *
 * Sets whether the entry has spell-checking enabled.
 */
void
sexy_spell_entry_set_checked(SexySpellEntry *entry, gboolean checked)
{
	GtkWidget *widget;

	if (entry->priv->checked == checked)
		return;

	entry->priv->checked = checked;
	widget = GTK_WIDGET(entry);

	if (checked == FALSE && GTK_WIDGET_REALIZED(widget)) {
		PangoLayout *layout;
		GdkRectangle rect;

		pango_attr_list_unref(entry->priv->attr_list);
		entry->priv->attr_list = pango_attr_list_new();

		layout = gtk_entry_get_layout(GTK_ENTRY(entry));
		pango_layout_set_attributes(layout, entry->priv->attr_list);

		rect.x = 0; rect.y = 0;
		rect.width  = widget->allocation.width;
		rect.height = widget->allocation.height;
		gdk_window_invalidate_rect(widget->window, &rect, TRUE);
	} else {
		if (entry->priv->words) {
			g_strfreev(entry->priv->words);
			g_free(entry->priv->word_starts);
			g_free(entry->priv->word_ends);
		}
		entry_strsplit_utf8(GTK_ENTRY(entry), &entry->priv->words, &entry->priv->word_starts, &entry->priv->word_ends);
		sexy_spell_entry_recheck_all(entry);
	}
}
