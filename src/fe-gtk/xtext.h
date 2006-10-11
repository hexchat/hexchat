#ifndef __XTEXT_H__
#define __XTEXT_H__

#include <gtk/gtkadjustment.h>
#ifdef USE_XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef USE_SHM
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

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

typedef struct _GtkXText GtkXText;
typedef struct _GtkXTextClass GtkXTextClass;
typedef struct textentry textentry;

typedef struct {
	GtkXText *xtext;					/* attached to this widget */

	gfloat old_value;					/* last known adj->value */
	textentry *text_first;
	textentry *text_last;
	guint16 grid_offset[256];

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

	int window_width;				/* window size when last rendered. */
	int window_height;

	unsigned int time_stamp:1;
	unsigned int scrollbar_down:1;
	unsigned int needs_recalc:1;
	unsigned int grid_dirty:1;
	unsigned int marker_seen:1;
	unsigned int reset_marker_pos:1;
} xtext_buffer;

struct _GtkXText
{
	GtkWidget widget;

	xtext_buffer *buffer;
	xtext_buffer *orig_buffer;
	xtext_buffer *selection_buffer;

#ifdef USE_SHM
	XShmSegmentInfo shminfo;
#endif

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

	int tint_red;
	int tint_green;
	int tint_blue;

	GdkGC *bgc;						  /* backing pixmap */
	GdkGC *fgc;						  /* text foreground color */
	GdkGC *light_gc;				  /* sep bar */
	GdkGC *dark_gc;
	GdkGC *thin_gc;
	GdkGC *marker_gc;
	gulong palette[XTEXT_COLS];

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

#ifdef USE_XFT
	XftColor color[XTEXT_COLS];
	XftColor *xft_fg;
	XftColor *xft_bg;				/* both point into color[20] */
	XftDraw *xftdraw;
	XftFont *font;
	XftFont *ifont;				/* italics */
#else
	struct pangofont
	{
		PangoFontDescription *font;
		PangoFontDescription *ifont;	/* italics */
		int ascent;
		int descent;
	} *font, pango_font;
	PangoLayout *layout;
#endif

	int fontsize;
	int space_width;				  /* width (pixels) of the space " " character */
	int stamp_width;				  /* width of "[88:88:88]" */
	int max_auto_indent;

	unsigned char scratch_buffer[4096];

	void (*error_function) (int type);
	int (*urlcheck_function) (GtkWidget * xtext, char *word, int len);

	int jump_out_offset;	/* point at which to stop rendering */
	int jump_in_offset;	/* "" start rendering */

	int ts_x;			/* ts origin for ->bgc GC */
	int ts_y;

	int clip_x;			/* clipping (x directions) */
	int clip_x2;		/* from x to x2 */

	int clip_y;			/* clipping (y directions) */
	int clip_y2;		/* from y to y2 */

	/* current text states */
	unsigned int bold:1;
	unsigned int underline:1;
	unsigned int italics:1;
	unsigned int hidden:1;

	/* text parsing states */
	unsigned int parsing_backcolor:1;
	unsigned int parsing_color:1;
	unsigned int backcolor:1;

	/* various state information */
	unsigned int moving_separator:1;
	unsigned int word_or_line_select:1;
	unsigned int button_down:1;
	unsigned int hilighting:1;
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
	unsigned int avoid_trans:1;
	unsigned int indent_changed:1;
	unsigned int shm:1;

	/* settings/prefs */
	unsigned int auto_indent:1;
	unsigned int color_paste:1;
	unsigned int thinline:1;
	unsigned int transparent:1;
	unsigned int shaded:1;
	unsigned int marker:1;
	unsigned int separator:1;
	unsigned int wordwrap:1;
	unsigned int overdraw:1;
	unsigned int ignore_hidden:1;	/* rawlog uses this */
};

struct _GtkXTextClass
{
	GtkWidgetClass parent_class;
	void (*word_click) (GtkXText * xtext, char *word, GdkEventButton * event);
};

GtkWidget *gtk_xtext_new (GdkColor palette[], int separator);
void gtk_xtext_append (xtext_buffer *buf, unsigned char *text, int len);
void gtk_xtext_append_indent (xtext_buffer *buf,
										unsigned char *left_text, int left_len,
										unsigned char *right_text, int right_len);
int gtk_xtext_set_font (GtkXText *xtext, char *name);
void gtk_xtext_set_background (GtkXText * xtext, GdkPixmap * pixmap, gboolean trans);
void gtk_xtext_set_palette (GtkXText * xtext, GdkColor palette[]);
void gtk_xtext_clear (xtext_buffer *buf);
void gtk_xtext_save (GtkXText * xtext, int fh);
void gtk_xtext_refresh (GtkXText * xtext, int do_trans);
int gtk_xtext_lastlog (xtext_buffer *out, xtext_buffer *search_area, int (*cmp_func) (char *, void *userdata), void *userdata);
textentry *gtk_xtext_search (GtkXText * xtext, const gchar *text, textentry *start, gboolean case_match, gboolean backward);
void gtk_xtext_reset_marker_pos (GtkXText *xtext);
void gtk_xtext_check_marker_visibility(GtkXText *xtext);

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
void gtk_xtext_set_tint (GtkXText *xtext, int tint_red, int tint_green, int tint_blue);
void gtk_xtext_set_urlcheck_function (GtkXText *xtext, int (*urlcheck_function) (GtkWidget *, char *, int));
void gtk_xtext_set_wordwrap (GtkXText *xtext, gboolean word_wrap);

xtext_buffer *gtk_xtext_buffer_new (GtkXText *xtext);
void gtk_xtext_buffer_free (xtext_buffer *buf);
void gtk_xtext_buffer_show (GtkXText *xtext, xtext_buffer *buf, int render);
GtkType gtk_xtext_get_type (void);

#endif
