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

#ifndef HEXCHAT_XTEXT_H
#define HEXCHAT_XTEXT_H

#include <gtk/gtk.h>

#define GTK_TYPE_XTEXT              (gtk_xtext_get_type ())
#define GTK_XTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GTK_TYPE_XTEXT, GtkXText))
#define GTK_XTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_XTEXT, GtkXTextClass))
#define GTK_IS_XTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GTK_TYPE_XTEXT))
#define GTK_IS_XTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_XTEXT))
#define GTK_XTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_XTEXT, GtkXTextClass))

#define ATTR_BOLD			'\002'
#define ATTR_COLOR		'\003'
#define ATTR_BLINK		'\006'
#define ATTR_BEEP			'\007'
#define ATTR_HIDDEN		'\010'
#define ATTR_ITALICS2	'\011'
#define ATTR_RESET		'\017'
#define ATTR_REVERSE		'\026'
#define ATTR_ITALICS		'\035'
#define ATTR_UNDERLINE	'\037'

/* these match palette.h */
#define XTEXT_MIRC_COLS 32
#define XTEXT_COLS 37		/* 32 plus 5 for extra stuff below */
#define XTEXT_MARK_FG 32	/* for marking text */
#define XTEXT_MARK_BG 33
#define XTEXT_FG 34
#define XTEXT_BG 35
#define XTEXT_MARKER 36		/* for marker line */
#define XTEXT_MAX_COLOR 41

typedef struct _GtkXText GtkXText;
typedef struct _GtkXTextClass GtkXTextClass;
typedef struct textentry textentry;

/*
 * offsets_t is used for retaining search information.
 * It is stored in the 'data' member of a GList,
 * as chained from ent->marks.  It saves starting and
 * ending+1 offset of a found occurrence.
 */
typedef union offsets_u {
	struct offsets_s {
		guint16	start;
		guint16	end;
	} o;
	guint32 u;
} offsets_t;

typedef enum marker_reset_reason_e {
	MARKER_WAS_NEVER_SET,
	MARKER_IS_SET,
	MARKER_RESET_MANUALLY,
	MARKER_RESET_BY_KILL,
	MARKER_RESET_BY_CLEAR
} marker_reset_reason;

typedef struct {
	GtkXText *xtext;					/* attached to this widget */

	gfloat old_value;					/* last known adj->value */
	textentry *text_first;
	textentry *text_last;

	textentry *last_ent_start;	  /* this basically describes the last rendered */
	textentry *last_ent_end;	  /* selection. */
	int last_offset_start;
	int last_offset_end;

	int last_pixel_pos;

	int pagetop_line;
	int pagetop_subline;
	textentry *pagetop_ent;			/* what's at xtext->adj->value */

	int num_lines;
	int indent;						  /* position of separator (pixels) from left */

	textentry *marker_pos;
	marker_reset_reason marker_state;

	int window_width;				/* window size when last rendered. */
	int window_height;

	unsigned int time_stamp:1;
	unsigned int scrollbar_down:1;
	unsigned int needs_recalc:1;
	unsigned int marker_seen:1;

	GList *search_found;		/* list of textentries where search found strings */
	gchar *search_text;		/* desired text to search for */
	gchar *search_nee;		/* prepared needle to look in haystack for */
	gint search_lnee;		/* its length */
	gtk_xtext_search_flags search_flags;	/* match, bwd, highlight */
	GList *cursearch;			/* GList whose 'data' pts to current textentry */
	GList *curmark;			/* current item in ent->marks */
	offsets_t curdata;		/* current offset info, from *curmark */
	GRegex *search_re;		/* Compiled regular expression */
	textentry *hintsearch;	/* textentry found for last search */
} xtext_buffer;

struct _GtkXText
{
	GtkWidget widget;

	xtext_buffer *buffer;
	xtext_buffer *orig_buffer;
	xtext_buffer *selection_buffer;

	GtkAdjustment *adj;
	GdkPixmap *pixmap;				/* 0 = use palette[19] */
	GdkDrawable *draw_buf;			/* points to ->window */
	GdkCursor *hand_cursor;
	GdkCursor *resize_cursor;

	int pixel_offset;					/* amount of pixels the top line is chopped by */

	int last_win_x;
	int last_win_y;
	int last_win_h;
	int last_win_w;

	GdkGC *bgc;						  /* backing pixmap */
	GdkGC *fgc;						  /* text foreground color */
	GdkGC *light_gc;				  /* sep bar */
	GdkGC *dark_gc;
	GdkGC *thin_gc;
	GdkGC *marker_gc;
	GdkColor palette[XTEXT_COLS];

	gint io_tag;					  /* for delayed refresh events */
	gint add_io_tag;				  /* "" when adding new text */
	gint scroll_tag;				  /* marking-scroll timeout */
	gulong vc_signal_tag;        /* signal handler for "value_changed" adj */

	int select_start_adj;		  /* the adj->value when the selection started */
	int select_start_x;
	int select_start_y;
	int select_end_x;
	int select_end_y;

	int max_lines;

	int col_fore;
	int col_back;

	int depth;						  /* gdk window depth */

	char num[8];					  /* for parsing mirc color */
	int nc;							  /* offset into xtext->num */

	textentry *hilight_ent;
	int hilight_start;
	int hilight_end;

	guint16 fontwidth[128];	  /* each char's width, only the ASCII ones */

	struct pangofont
	{
		PangoFontDescription *font;
		int ascent;
		int descent;
	} *font, pango_font;
	PangoLayout *layout;

	int fontsize;
	int space_width;				  /* width (pixels) of the space " " character */
	int stamp_width;				  /* width of "[88:88:88]" */
	int max_auto_indent;

	unsigned char scratch_buffer[4096];

	int (*urlcheck_function) (GtkWidget * xtext, char *word);

	int jump_out_offset;	/* point at which to stop rendering */
	int jump_in_offset;	/* "" start rendering */

	int ts_x;			/* ts origin for ->bgc GC */
	int ts_y;

	int clip_x;			/* clipping (x directions) */
	int clip_x2;		/* from x to x2 */

	int clip_y;			/* clipping (y directions) */
	int clip_y2;		/* from y to y2 */

	/* current text states */
	unsigned int underline:1;
	unsigned int hidden:1;

	/* text parsing states */
	unsigned int parsing_backcolor:1;
	unsigned int parsing_color:1;
	unsigned int backcolor:1;

	/* various state information */
	unsigned int moving_separator:1;
	unsigned int word_select:1;
	unsigned int line_select:1;
	unsigned int button_down:1;
	unsigned int dont_render:1;
	unsigned int dont_render2:1;
	unsigned int cursor_hand:1;
	unsigned int cursor_resize:1;
	unsigned int skip_border_fills:1;
	unsigned int skip_stamp:1;
	unsigned int mark_stamp:1;	/* Cut&Paste with stamps? */
	unsigned int force_stamp:1;	/* force redrawing it */
	unsigned int render_hilights_only:1;
	unsigned int in_hilight:1;
	unsigned int un_hilight:1;
	unsigned int recycle:1;
	unsigned int force_render:1;
	unsigned int color_paste:1; /* CTRL was pressed when selection finished */

	/* settings/prefs */
	unsigned int auto_indent:1;
	unsigned int thinline:1;
	unsigned int marker:1;
	unsigned int separator:1;
	unsigned int wordwrap:1;
	unsigned int ignore_hidden:1;	/* rawlog uses this */
};

struct _GtkXTextClass
{
	GtkWidgetClass parent_class;
	void (*word_click) (GtkXText * xtext, char *word, GdkEventButton * event);
	void (*set_scroll_adjustments) (GtkXText *xtext, GtkAdjustment *hadj, GtkAdjustment *vadj);
};

GtkWidget *gtk_xtext_new (GdkColor palette[], int separator);
void gtk_xtext_append (xtext_buffer *buf, unsigned char *text, int len, time_t stamp);
void gtk_xtext_append_indent (xtext_buffer *buf,
										unsigned char *left_text, int left_len,
										unsigned char *right_text, int right_len,
										time_t stamp);
int gtk_xtext_set_font (GtkXText *xtext, char *name);
void gtk_xtext_set_background (GtkXText * xtext, GdkPixmap * pixmap);
void gtk_xtext_set_palette (GtkXText * xtext, GdkColor palette[]);
void gtk_xtext_clear (xtext_buffer *buf, int lines);
void gtk_xtext_save (GtkXText * xtext, int fh);
void gtk_xtext_refresh (GtkXText * xtext);
int gtk_xtext_lastlog (xtext_buffer *out, xtext_buffer *search_area);
textentry *gtk_xtext_search (GtkXText * xtext, const gchar *text, gtk_xtext_search_flags flags, GError **err);
void gtk_xtext_reset_marker_pos (GtkXText *xtext);
int gtk_xtext_moveto_marker_pos (GtkXText *xtext);
void gtk_xtext_check_marker_visibility(GtkXText *xtext);
void gtk_xtext_set_marker_last (session *sess);

gboolean gtk_xtext_is_empty (xtext_buffer *buf);
typedef void (*GtkXTextForeach) (GtkXText *xtext, unsigned char *text, void *data);
void gtk_xtext_foreach (xtext_buffer *buf, GtkXTextForeach func, void *data);

void gtk_xtext_set_error_function (GtkXText *xtext, void (*error_function) (int));
void gtk_xtext_set_indent (GtkXText *xtext, gboolean indent);
void gtk_xtext_set_max_indent (GtkXText *xtext, int max_auto_indent);
void gtk_xtext_set_max_lines (GtkXText *xtext, int max_lines);
void gtk_xtext_set_show_marker (GtkXText *xtext, gboolean show_marker);
void gtk_xtext_set_show_separator (GtkXText *xtext, gboolean show_separator);
void gtk_xtext_set_thin_separator (GtkXText *xtext, gboolean thin_separator);
void gtk_xtext_set_time_stamp (xtext_buffer *buf, gboolean timestamp);
void gtk_xtext_set_urlcheck_function (GtkXText *xtext, int (*urlcheck_function) (GtkWidget *, char *));
void gtk_xtext_set_wordwrap (GtkXText *xtext, gboolean word_wrap);

xtext_buffer *gtk_xtext_buffer_new (GtkXText *xtext);
void gtk_xtext_buffer_free (xtext_buffer *buf);
void gtk_xtext_buffer_show (GtkXText *xtext, xtext_buffer *buf, int render);
void gtk_xtext_copy_selection (GtkXText *xtext);
GType gtk_xtext_get_type (void);

#endif
