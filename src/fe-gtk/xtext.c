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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 * =========================================================================
 *
 * xtext, the text widget used by X-Chat.
 * By Peter Zelezny <zed@xchat.org>.
 *
 */

#define XCHAT							/* using xchat */
#define TINT_VALUE 195				/* 195/255 of the brightness. */
#define MOTION_MONITOR				/* URL hilights. */
#define SMOOTH_SCROLL				/* line-by-line or pixel scroll? */
#define SCROLL_HACK					/* use gdk_window_scroll? */
#undef COLOR_HILIGHT				/* Color instead of underline? */
#define USE_GDK_PIXBUF

#define MARGIN 2						/* dont touch. */
#define REFRESH_TIMEOUT 20
#define WORDWRAP_LIMIT 24

#ifdef XCHAT
#include "../../config.h"			/* can define USE_XLIB here */
#else
#define USE_XLIB
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkselection.h>

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "xtext.h"

#ifdef USE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#define charlen(str) g_utf8_skip[*(guchar *)(str)]

#ifdef WIN32
#include <windows.h>
#include <gdk/win32/gdkwin32.h>
#endif

/* is delimiter */
#define is_del(c) \
	(c == ' ' || c == '\n' || c == ')' || c == '(' || \
	 c == '>' || c == '<' || c == ATTR_RESET || c == ATTR_BOLD || c == 0)

#ifdef SCROLL_HACK
/* force scrolling off */
#define dontscroll(buf) (buf)->last_pixel_pos = 2147483647
#else
#define dontscroll(buf)
#endif

static GtkWidgetClass *parent_class = NULL;

struct textentry
{
	struct textentry *next;
	struct textentry *prev;
	unsigned char *str;
	time_t stamp;
	gint16 str_width;
	gint16 str_len;
	gint16 mark_start;
	gint16 mark_end;
	gint16 indent;
	gint16 left_len;
	gint16 lines_taken;
	unsigned int mb:1;	/* is multibyte? */
	unsigned int new:1;	/* new and hasn't been drawn yet? */
};

enum
{
	WORD_CLICK,
	LAST_SIGNAL
};

/* values for selection info */
enum
{
	TARGET_UTF8_STRING,
	TARGET_STRING,
	TARGET_TEXT,
	TARGET_COMPOUND_TEXT
};

static guint xtext_signals[LAST_SIGNAL];

#ifdef XCHAT
char *nocasestrstr (const char *text, const char *tofind);	/* util.c */
int get_stamp_str (time_t, char *, int);
#endif
static void gtk_xtext_render_page (GtkXText * xtext);
static void gtk_xtext_calc_lines (xtext_buffer *buf, int);
#if defined(USE_XLIB) || defined(WIN32)
static void gtk_xtext_load_trans (GtkXText * xtext);
static void gtk_xtext_free_trans (GtkXText * xtext);
#endif
static textentry *gtk_xtext_nth (GtkXText *xtext, int line, int *subline);
static void gtk_xtext_adjustment_changed (GtkAdjustment * adj,
														GtkXText * xtext);
static void gtk_xtext_render_ents (GtkXText * xtext, textentry *, textentry *);
static void gtk_xtext_recalc_widths (xtext_buffer *buf, int);
static void gtk_xtext_fix_indent (xtext_buffer *buf);
static char *gtk_xtext_conv_color (unsigned char *text, int len, int *newlen);
static unsigned char *
gtk_xtext_strip_color (unsigned char *text, int len, unsigned char *outbuf,
							  int *newlen, int *mb_ret);
static GtkType gtk_xtext_get_type (void);

/* some utility functions first */

#ifndef XCHAT	/* xchat has this in util.c */

static char *
nocasestrstr (const char *s, const char *tofind)
{
   register const size_t len = strlen (tofind);

   if (len == 0)
     return (char *)s;
   while (toupper(*s) != toupper(*tofind) || strncasecmp (s, tofind, len))
     if (*s++ == '\0')
       return (char *)NULL;
   return (char *)s;   
}

#endif

/* gives width of a 8bit string - with no mIRC codes in it */

static int
gtk_xtext_text_width_8bit (GtkXText *xtext, unsigned char *str, int len)
{
	int width = 0;

	while (len)
	{
		width += xtext->fontwidth[*str];
		str++;
		len--;
	}

	return width;
}

#ifdef WIN32

static void
win32_draw_bg (GtkXText *xtext, int x, int y, int width, int height)
{
	HDC hdc;
	HWND hwnd;
	HRGN rgn;

	if (xtext->shaded)
	{
		gdk_draw_drawable (xtext->draw_buf, xtext->bgc, xtext->pixmap,
								 x, y, x, y, width, height);
	} else
	{
		hwnd = GDK_WINDOW_HWND (xtext->draw_buf);
		hdc = GetDC (hwnd);

		rgn = CreateRectRgn (x, y, x + width, y + height);
		SelectClipRgn (hdc, rgn);

		PaintDesktop (hdc);

		ReleaseDC (hwnd, hdc);
		DeleteObject (rgn);
	}
}

static void
xtext_draw_bg (GtkXText *xtext, int x, int y, int width, int height)
{
	if (xtext->transparent)
		win32_draw_bg (xtext, x, y, width, height);
	else
		gdk_draw_rectangle (xtext->draw_buf, xtext->bgc, 1, x, y, width, height);
}

#else

#define xtext_draw_bg(xt,x,y,w,h) gdk_draw_rectangle(xt->draw_buf, xt->bgc, \
																	  1,x,y,w,h);

#endif

/* ========================================= */
/* ========== XFT 1 and 2 BACKEND ========== */
/* ========================================= */

#ifdef USE_XFT

#define backend_font_close(f) XftFontClose(GDK_DISPLAY(),f)

static void
backend_init (GtkXText *xtext)
{
	if (xtext->xftdraw == NULL)
		xtext->xftdraw = XftDrawCreate (GDK_DISPLAY (),
			GDK_WINDOW_XWINDOW (xtext->draw_buf),
			GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (xtext->draw_buf)),
			GDK_COLORMAP_XCOLORMAP (gdk_drawable_get_colormap (xtext->draw_buf)));
}

static void
backend_deinit (GtkXText *xtext)
{
	if (xtext->xftdraw)
	{
		XftDrawDestroy (xtext->xftdraw);
		xtext->xftdraw = NULL;
	}
}

static void
backend_font_open (GtkXText *xtext, char *name)
{
	XftFont *font = NULL;
	PangoFontDescription *fontd;
	Display *xdisplay = GDK_DISPLAY ();
	int weight, slant, screen = DefaultScreen (xdisplay);

	if (*name == '-')
	{
		xtext->font = XftFontOpenXlfd (xdisplay, screen, name);
		if (xtext->font)
			return;
	}

	fontd = pango_font_description_from_string (name);

	if (pango_font_description_get_size (fontd) != 0)
	{
		weight = pango_font_description_get_weight (fontd);
		/* from pangoft2-fontmap.c */
		if (weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_LIGHT) / 2)
			weight = XFT_WEIGHT_LIGHT;
		else if (weight < (PANGO_WEIGHT_NORMAL + 600) / 2)
			weight = XFT_WEIGHT_MEDIUM;
		else if (weight < (600 + PANGO_WEIGHT_BOLD) / 2)
			weight = XFT_WEIGHT_DEMIBOLD;
		else if (weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
			weight = XFT_WEIGHT_BOLD;
		else
			weight = XFT_WEIGHT_BLACK;

		slant = pango_font_description_get_style (fontd);
		if (slant == PANGO_STYLE_ITALIC)
			slant = XFT_SLANT_ITALIC;
		else if (slant == PANGO_STYLE_OBLIQUE)
			slant = XFT_SLANT_OBLIQUE;
		else
			slant = XFT_SLANT_ROMAN;

		font = XftFontOpen (xdisplay, screen,
						XFT_FAMILY, XftTypeString, pango_font_description_get_family (fontd),
						/*XFT_ENCODING, XftTypeString, "glyphs-fontspecific",*/
						XFT_CORE, XftTypeBool, False,
						XFT_SIZE, XftTypeDouble, (double)pango_font_description_get_size (fontd)/PANGO_SCALE,
						XFT_WEIGHT, XftTypeInteger, weight,
						XFT_SLANT, XftTypeInteger, slant,
						NULL);
	}
	pango_font_description_free (fontd);

	if (font == NULL)
	{
		font = XftFontOpenName (xdisplay, screen, name);
		if (font == NULL)
			font = XftFontOpenName (xdisplay, screen, "sans-12");
	}

	xtext->font = font;
}

inline static int
backend_get_char_width (GtkXText *xtext, unsigned char *str, int *mbl_ret)
{
	XGlyphInfo ext;

	if (*str < 128)
	{
		*mbl_ret = 1;
		return xtext->fontwidth[(int)*str];
	}

	*mbl_ret = charlen (str);
	XftTextExtentsUtf8 (GDK_DISPLAY (), xtext->font, str, *mbl_ret, &ext);

	return ext.xOff;
}

static int
backend_get_text_width (GtkXText *xtext, char *str, int len, int is_mb)
{
	XGlyphInfo ext;

	if (!is_mb)
		return gtk_xtext_text_width_8bit (xtext, str, len);

	XftTextExtentsUtf8 (GDK_DISPLAY (), xtext->font, str, len, &ext);
	return ext.xOff;
}

static void
backend_draw_text (GtkXText *xtext, int dofill, GdkGC *gc, int x, int y,
						 char *str, int len, int str_width, int is_mb)
{
	/*Display *xdisplay = GDK_WINDOW_XDISPLAY (xtext->draw_buf);*/
	void (*draw_func) (XftDraw *, XftColor *, XftFont *, int, int, XftChar8 *, int) = XftDrawString8;

	/* if all ascii, use String8 to avoid the conversion penalty */
	if (is_mb)
		draw_func = XftDrawStringUtf8;

	if (dofill)
	{
/*		register GC xgc = GDK_GC_XGC (gc);
		XSetForeground (xdisplay, xgc, xtext->xft_bg->pixel);
		XFillRectangle (xdisplay, GDK_WINDOW_XWINDOW (xtext->draw_buf), xgc, x,
							 y - xtext->font->ascent, str_width, xtext->fontsize);*/
		XftDrawRect (xtext->xftdraw, xtext->xft_bg, x,
						 y - xtext->font->ascent, str_width, xtext->fontsize);
	}

	draw_func (xtext->xftdraw, xtext->xft_fg, xtext->font, x, y, str, len);
	/* if (~black_background) */
/*	if (xtext->xft_bg->color.red < 0x2000 ||
		 xtext->xft_bg->color.green < 0x2000 ||
		 xtext->xft_bg->color.blue < 0x2000)
		draw_func (xtext->xftdraw, xtext->xft_fg, xtext->font, x, y, str, len);*/

	if (xtext->bold)
		draw_func (xtext->xftdraw, xtext->xft_fg, xtext->font, x + 1, y, str, len);
}

static void
backend_set_clip (GtkXText *xtext, GdkRectangle *area)
{
	Region reg;
	XRectangle rect;

	rect.x = area->x;
	rect.y = area->y;
	rect.width = area->width;
	rect.height = area->height;

	reg = XCreateRegion ();
	XUnionRectWithRegion (&rect, reg, reg);
	XftDrawSetClip (xtext->xftdraw, reg);
	XDestroyRegion (reg);

	gdk_gc_set_clip_rectangle (xtext->fgc, area);
}

static void
backend_clear_clip (GtkXText *xtext)
{
	XftDrawSetClip (xtext->xftdraw, NULL);
	gdk_gc_set_clip_rectangle (xtext->fgc, NULL);
}

#else	/* !USE_XFT */

/* ======================================= */
/* ============ PANGO BACKEND ============ */
/* ======================================= */

#define backend_font_close(f) pango_font_description_free(f->font)

static void
backend_init (GtkXText *xtext)
{
	if (xtext->layout == NULL)
	{
		xtext->layout = gtk_widget_create_pango_layout (GTK_WIDGET (xtext), 0); 
		pango_layout_set_font_description (xtext->layout, xtext->font->font);
	}
}

static void
backend_deinit (GtkXText *xtext)
{
	if (xtext->layout)
	{
		g_object_unref (xtext->layout);
		xtext->layout = NULL;
	}
}

static void
backend_font_open (GtkXText *xtext, char *name)
{
	PangoLanguage *lang;
	PangoContext *context;
	PangoFontMetrics *metrics;

	xtext->font = &xtext->pango_font;

	xtext->font->font = pango_font_description_from_string (name);
	if (!xtext->font->font)
		xtext->font->font = pango_font_description_from_string ("sans 11");

	if (xtext->font->font)
	{
		if (pango_font_description_get_size (xtext->font->font) == 0)
		{
			pango_font_description_free (xtext->font->font);
			xtext->font->font = pango_font_description_from_string ("sans 11");
		}
	}

	if (!xtext->font->font)
	{
		xtext->font = NULL;
		return;
	}

	backend_init (xtext);
	pango_layout_set_font_description (xtext->layout, xtext->font->font);

	/* vte does it this way */
	context = gtk_widget_get_pango_context (GTK_WIDGET (xtext));
	lang = pango_context_get_language (context);
	metrics = pango_context_get_metrics (context, xtext->font->font, lang);
	xtext->font->ascent = pango_font_metrics_get_ascent (metrics) / PANGO_SCALE;
	xtext->font->descent = pango_font_metrics_get_descent (metrics) / PANGO_SCALE;
	pango_font_metrics_unref (metrics);
}

static int
backend_get_text_width (GtkXText *xtext, char *str, int len, int is_mb)
{
	int width;

	if (!is_mb)
		return gtk_xtext_text_width_8bit (xtext, str, len);

	if (*str == 0)
		return 0;

	pango_layout_set_text (xtext->layout, str, len);
	pango_layout_get_pixel_size (xtext->layout, &width, NULL);

	return width;
}

inline static int
backend_get_char_width (GtkXText *xtext, unsigned char *str, int *mbl_ret)
{
	int width;

	if (*str < 128)
	{
		*mbl_ret = 1;
		return xtext->fontwidth[(int)*str];
	}

	*mbl_ret = charlen (str);
	pango_layout_set_text (xtext->layout, str, *mbl_ret);
	pango_layout_get_pixel_size (xtext->layout, &width, NULL);

	return width;
}

static void
backend_draw_text (GtkXText *xtext, int dofill, GdkGC *gc, int x, int y,
						 char *str, int len, int str_width, int is_mb)
{
	GdkGCValues val;
	GdkColor col;

	/*backend_init (xtext);*/
	pango_layout_set_text (xtext->layout, str, len);

	y -= xtext->font->ascent;

	if (dofill)
	{
#ifdef WIN32
		if (xtext->transparent && !xtext->backcolor)
			win32_draw_bg (xtext, x, y - xtext->font->ascent, str_width,
								xtext->fontsize);
		else
#endif
		{
			gdk_gc_get_values (gc, &val);
			col.pixel = val.background.pixel;
			gdk_gc_set_foreground (gc, &col);
			gdk_draw_rectangle (xtext->draw_buf, gc, 1, x, y, str_width,
									  xtext->fontsize);
			col.pixel = val.foreground.pixel;
			gdk_gc_set_foreground (gc, &col);
		}
	}

	gdk_draw_layout (xtext->draw_buf, gc, x, y, xtext->layout);

	if (xtext->bold)
		gdk_draw_layout (xtext->draw_buf, gc, x + 1, y, xtext->layout);
}

static void
backend_set_clip (GtkXText *xtext, GdkRectangle *area)
{
	gdk_gc_set_clip_rectangle (xtext->fgc, area);
	gdk_gc_set_clip_rectangle (xtext->bgc, area);
}

static void
backend_clear_clip (GtkXText *xtext)
{
	gdk_gc_set_clip_rectangle (xtext->fgc, NULL);
	gdk_gc_set_clip_rectangle (xtext->bgc, NULL);
}

#endif /* !USE_PANGO */

static void
xtext_set_fg (GtkXText *xtext, GdkGC *gc, int index)
{
	GdkColor col;

	col.pixel = xtext->palette[index];
	gdk_gc_set_foreground (gc, &col);

#ifdef USE_XFT
	if (gc == xtext->fgc)
		xtext->xft_fg = &xtext->color[index];
	else
		xtext->xft_bg = &xtext->color[index];
#endif
}

#ifdef USE_XFT

#define xtext_set_bg(xt,gc,index) xt->xft_bg = &xt->color[index]

#else

static void
xtext_set_bg (GtkXText *xtext, GdkGC *gc, int index)
{
	GdkColor col;

	col.pixel = xtext->palette[index];
	gdk_gc_set_background (gc, &col);
}

#endif

static void
gtk_xtext_init (GtkXText * xtext)
{
	xtext->pixmap = NULL;
	xtext->io_tag = 0;
	xtext->add_io_tag = 0;
	xtext->scroll_tag = 0;
	xtext->max_lines = 0;
	xtext->col_back = 19;
	xtext->col_fore = 18;
	xtext->nc = 0;
	xtext->pixel_offset = 0;
	xtext->bold = FALSE;
	xtext->underline = FALSE;
	xtext->font = NULL;
#ifdef USE_XFT
	xtext->xftdraw = NULL;
#else
	xtext->layout = NULL;
#endif
	xtext->jump_out_offset = 0;
	xtext->jump_in_offset = 0;
	xtext->error_function = NULL;
	xtext->urlcheck_function = NULL;
	xtext->color_paste = FALSE;
	xtext->skip_fills = FALSE;
	xtext->skip_border_fills = FALSE;
	xtext->skip_stamp = FALSE;
	xtext->render_hilights_only = FALSE;
	xtext->un_hilight = FALSE;
	xtext->recycle = FALSE;
	xtext->dont_render = FALSE;
	xtext->dont_render2 = FALSE;
	xtext->tint_red = xtext->tint_green = xtext->tint_blue = TINT_VALUE;

	xtext->adj = (GtkAdjustment *) gtk_adjustment_new (0, 0, 1, 1, 1, 1);
	g_object_ref (G_OBJECT (xtext->adj));
	gtk_object_sink ((GtkObject *) xtext->adj);

	g_signal_connect (G_OBJECT (xtext->adj), "value_changed",
							G_CALLBACK (gtk_xtext_adjustment_changed), xtext);
	{
		static const GtkTargetEntry targets[] = {
			{ "UTF8_STRING", 0, TARGET_UTF8_STRING },
			{ "STRING", 0, TARGET_STRING },
			{ "TEXT",   0, TARGET_TEXT }, 
			{ "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT }
		};
		static const gint n_targets = sizeof (targets) / sizeof (targets[0]);

		gtk_selection_add_targets (GTK_WIDGET (xtext), GDK_SELECTION_PRIMARY,
											targets, n_targets);
#ifdef WIN32
		gtk_selection_add_targets (GTK_WIDGET (xtext),
											gdk_atom_intern ("CLIPBOARD", FALSE),
											targets, n_targets);
#endif
	}
}

static void
gtk_xtext_adjustment_set (xtext_buffer *buf, int fire_signal)
{
	GtkAdjustment *adj = buf->xtext->adj;

	if (buf->xtext->buffer == buf)
	{
		adj->lower = 0;
		adj->upper = buf->num_lines;

		if (adj->upper == 0)
			adj->upper = 1;

		adj->page_size =
			(GTK_WIDGET (buf->xtext)->allocation.height -
			 buf->xtext->font->descent) / buf->xtext->fontsize;
		adj->page_increment = adj->page_size;

		if (adj->value > adj->upper - adj->page_size)
			adj->value = adj->upper - adj->page_size;

		if (fire_signal)
			gtk_adjustment_changed (adj);
	}
}

static gint
gtk_xtext_adjustment_timeout (GtkXText * xtext)
{
	gtk_xtext_render_page (xtext);
	xtext->io_tag = 0;
	return 0;
}

static void
gtk_xtext_adjustment_changed (GtkAdjustment * adj, GtkXText * xtext)
{
#ifdef SMOOTH_SCROLL
	if (xtext->buffer->old_value != xtext->adj->value)
#else
	if ((int) xtext->buffer->old_value != (int) xtext->adj->value)
#endif
	{
		if (xtext->adj->value >= xtext->adj->upper - xtext->adj->page_size)
			xtext->buffer->scrollbar_down = TRUE;
		else
			xtext->buffer->scrollbar_down = FALSE;

		if (xtext->adj->value + 1 == xtext->buffer->old_value ||
			 xtext->adj->value - 1 == xtext->buffer->old_value)	/* clicked an arrow? */
		{
			if (xtext->io_tag)
			{
				g_source_remove (xtext->io_tag);
				xtext->io_tag = 0;
			}
			gtk_xtext_render_page (xtext);
		} else
		{
			if (!xtext->io_tag)
				xtext->io_tag = g_timeout_add (REFRESH_TIMEOUT,
															(GSourceFunc)
															gtk_xtext_adjustment_timeout,
															xtext);
		}
	}
	xtext->buffer->old_value = adj->value;
}

GtkWidget *
gtk_xtext_new (GdkColor palette[], int separator)
{
	GtkXText *xtext;

	xtext = g_object_new (gtk_xtext_get_type (), NULL);
	xtext->separator = separator;
	xtext->wordwrap = TRUE;
	xtext->buffer = gtk_xtext_buffer_new (xtext);
	xtext->orig_buffer = xtext->buffer;

	gtk_widget_set_double_buffered (GTK_WIDGET (xtext), FALSE);
	gtk_xtext_set_palette (xtext, palette);

	return GTK_WIDGET (xtext);
}

static void
gtk_xtext_destroy (GtkObject * object)
{
	GtkXText *xtext = GTK_XTEXT (object);

	if (xtext->add_io_tag)
	{
		g_source_remove (xtext->add_io_tag);
		xtext->add_io_tag = 0;
	}

	if (xtext->scroll_tag)
	{
		g_source_remove (xtext->scroll_tag);
		xtext->scroll_tag = 0;
	}

	if (xtext->io_tag)
	{
		g_source_remove (xtext->io_tag);
		xtext->io_tag = 0;
	}

	if (xtext->pixmap)
	{
#if defined(USE_XLIB) || defined(WIN32)
		if (xtext->transparent)
			gtk_xtext_free_trans (xtext);
		else
#endif
			g_object_unref (xtext->pixmap);
		xtext->pixmap = NULL;
	}

	if (xtext->font)
	{
		backend_font_close (xtext->font);
		xtext->font = NULL;
	}

	if (xtext->adj)
	{
		g_signal_handlers_disconnect_matched (G_OBJECT (xtext->adj),
					G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, xtext);
	/*	gtk_signal_disconnect_by_data (GTK_OBJECT (xtext->adj), xtext);*/
		g_object_unref (G_OBJECT (xtext->adj));
		xtext->adj = NULL;
	}

	if (xtext->bgc)
	{
		g_object_unref (xtext->bgc);
		xtext->bgc = NULL;
	}

	if (xtext->fgc)
	{
		g_object_unref (xtext->fgc);
		xtext->fgc = NULL;
	}

	if (xtext->light_gc)
	{
		g_object_unref (xtext->light_gc);
		xtext->light_gc = NULL;
	}

	if (xtext->dark_gc)
	{
		g_object_unref (xtext->dark_gc);
		xtext->dark_gc = NULL;
	}

	if (xtext->thin_gc)
	{
		g_object_unref (xtext->thin_gc);
		xtext->thin_gc = NULL;
	}

	if (xtext->hand_cursor)
	{
		gdk_cursor_unref (xtext->hand_cursor);
		xtext->hand_cursor = NULL;
	}

	if (xtext->orig_buffer)
	{
		gtk_xtext_buffer_free (xtext->orig_buffer);
		xtext->orig_buffer = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_xtext_unrealize (GtkWidget * widget)
{
	backend_deinit (GTK_XTEXT (widget));

	if (parent_class->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gtk_xtext_realize (GtkWidget * widget)
{
	GtkXText *xtext;
	GdkWindowAttr attributes;
	GdkGCValues val;
	GdkColor col;
	GdkColormap *cmap;

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	xtext = GTK_XTEXT (widget);

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events (widget) |
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
#ifdef MOTION_MONITOR
		| GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK;
#else
		| GDK_POINTER_MOTION_MASK;
#endif

	cmap = gtk_widget_get_colormap (widget);
	attributes.colormap = cmap;
	attributes.visual = gtk_widget_get_visual (widget);

	widget->window = gdk_window_new (widget->parent->window, &attributes,
												GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL |
												GDK_WA_COLORMAP);

	gdk_window_set_user_data (widget->window, widget);

	xtext->depth = gdk_drawable_get_visual (widget->window)->depth;

	val.subwindow_mode = GDK_INCLUDE_INFERIORS;
	val.graphics_exposures = 0;

	xtext->bgc = gdk_gc_new_with_values (widget->window, &val,
													 GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->fgc = gdk_gc_new_with_values (widget->window, &val,
													 GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->light_gc = gdk_gc_new_with_values (widget->window, &val,
											GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->dark_gc = gdk_gc_new_with_values (widget->window, &val,
											GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
	xtext->thin_gc = gdk_gc_new_with_values (widget->window, &val,
											GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);

	/* for the separator bar (light) */
	col.red = 0xffff; col.green = 0xffff; col.blue = 0xffff;
	gdk_colormap_alloc_color (cmap, &col, TRUE, TRUE);
	gdk_gc_set_foreground (xtext->light_gc, &col);

	/* for the separator bar (dark) */
	col.red = 0x1111; col.green = 0x1111; col.blue = 0x1111;
	gdk_colormap_alloc_color (cmap, &col, TRUE, TRUE);
	gdk_gc_set_foreground (xtext->dark_gc, &col);

	/* for the separator bar (thinline) */
	col.red = 0x8e38; col.green = 0x8e38; col.blue = 0x9f38;
	gdk_colormap_alloc_color (cmap, &col, TRUE, TRUE);
	gdk_gc_set_foreground (xtext->thin_gc, &col);

	xtext_set_fg (xtext, xtext->fgc, 18);
	xtext_set_bg (xtext, xtext->fgc, 19);
	xtext_set_fg (xtext, xtext->bgc, 19);

#if defined(USE_XLIB) || defined(WIN32)
	if (xtext->transparent)
	{
		gtk_xtext_load_trans (xtext);
	} else
#endif
	if (xtext->pixmap)
	{
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
		gdk_gc_set_fill (xtext->bgc, GDK_TILED);
	}

	xtext->hand_cursor = gdk_cursor_new (GDK_HAND1);

	gdk_window_set_back_pixmap (widget->window, NULL, FALSE);

	/* draw directly to window */
	xtext->draw_buf = widget->window;

	backend_init (xtext);
}

static void
gtk_xtext_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
	requisition->width = 200;
	requisition->height = 90;
}

static void
gtk_xtext_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	int height_only = FALSE;
	int do_trans = TRUE;

	if (allocation->width == xtext->buffer->window_width)
		height_only = TRUE;

	if (allocation->x == widget->allocation.x &&
		 allocation->y == widget->allocation.y && xtext->avoid_trans)
		do_trans = FALSE;

	xtext->avoid_trans = FALSE;

	widget->allocation = *allocation;
	if (GTK_WIDGET_REALIZED (widget))
	{
		xtext->buffer->window_width = allocation->width;
		xtext->buffer->window_height = allocation->height;

		gdk_window_move_resize (widget->window, allocation->x, allocation->y,
										allocation->width, allocation->height);
		dontscroll (xtext->buffer);	/* force scrolling off */
		if (!height_only)
			gtk_xtext_calc_lines (xtext->buffer, FALSE);
		else
		{
			xtext->buffer->pagetop_ent = NULL;
			gtk_xtext_adjustment_set (xtext->buffer, FALSE);
		}
#if defined(USE_XLIB) || defined(WIN32)
		if (do_trans && xtext->transparent && xtext->shaded)
		{
			gtk_xtext_free_trans (xtext);
			gtk_xtext_load_trans (xtext);
		}
#endif
		if (xtext->buffer->scrollbar_down)
			gtk_adjustment_set_value (xtext->adj, xtext->adj->upper -
											  xtext->adj->page_size);
	}
}

static int
gtk_xtext_selection_clear (xtext_buffer *buf)
{
	textentry *ent;
	int ret = 0;

	ent = buf->last_ent_start;
	while (ent)
	{
		if (ent->mark_start != -1)
			ret = 1;
		ent->mark_start = -1;
		ent->mark_end = -1;
		if (ent == buf->last_ent_end)
			break;
		ent = ent->next;
	}

	return ret;
}

static int
find_x (GtkXText *xtext, textentry *ent, unsigned char *text, int x, int indent)
{
	int xx = indent;
	int i = 0;
	int col = FALSE;
	int nc = 0;
	unsigned char *orig = text;
	int mbl;

	while (*text)
	{
		mbl = 1;
		if ((col && isdigit (*text) && nc < 2) ||
			 (col && *text == ',' && isdigit (*(text+1)) && nc < 3))
		{
			nc++;
			if (*text == ',')
				nc = 0;
			text++;
		} else
		{
			col = FALSE;
			switch (*text)
			{
			case ATTR_COLOR:
				col = TRUE;
				nc = 0;
			case ATTR_BEEP:
			case ATTR_RESET:
			case ATTR_REVERSE:
			case ATTR_BOLD:
			case ATTR_UNDERLINE:
				text++;
				break;
			default:
				xx += backend_get_char_width (xtext, text, &mbl);
				text += mbl;
				if (xx >= x)
					return i + (orig - ent->str);
			}
		}

		i += mbl;
		if (text - orig >= ent->str_len)
			return ent->str_len;
	}

	return ent->str_len;
}

static int
gtk_xtext_find_x (GtkXText * xtext, int x, textentry * ent, int subline,
						int line, int *out_of_bounds)
{
	int indent;
	unsigned char *str;

	if (subline < 1)
		indent = ent->indent;
	else
		indent = xtext->buffer->indent;

	if (line > xtext->adj->page_size || line < 0)
		return 0;

	if (xtext->buffer->grid_offset[line] > ent->str_len)
		return 0;

	if (xtext->buffer->grid_offset[line] < 0)
		return 0;

	str = ent->str + xtext->buffer->grid_offset[line];

	if (x < indent)
	{
		*out_of_bounds = 1;
		return (str - ent->str);
	}

	*out_of_bounds = 0;

	return find_x (xtext, ent, str, x, indent);
}

static textentry *
gtk_xtext_find_char (GtkXText * xtext, int x, int y, int *off,
							int *out_of_bounds)
{
	textentry *ent;
	int line;
	int subline;

	line = (y + xtext->pixel_offset) / xtext->fontsize;
	ent = gtk_xtext_nth (xtext, line + (int)xtext->adj->value, &subline);
	if (!ent)
		return 0;

	if (off)
		*off = gtk_xtext_find_x (xtext, x, ent, subline, line, out_of_bounds);

	return ent;
}

static void
gtk_xtext_draw_sep (GtkXText * xtext, int y)
{
	int x, height;
	GdkGC *light, *dark;

	if (y == -1)
	{
		y = 0;
		height = GTK_WIDGET (xtext)->allocation.height;
	} else
	{
		height = xtext->fontsize;
	}

	/* draw the separator line */
	if (xtext->separator && xtext->buffer->indent)
	{
		light = xtext->light_gc;
		dark = xtext->dark_gc;

		x = xtext->buffer->indent - ((xtext->space_width + 1) / 2);
		if (x < 1)
			return;

		if (xtext->thinline)
		{
			if (xtext->moving_separator)
				gdk_draw_line (xtext->draw_buf, light, x, y, x, y + height);
			else
				gdk_draw_line (xtext->draw_buf, xtext->thin_gc, x, y, x, y + height);
		} else
		{
			if (xtext->moving_separator)
			{
				gdk_draw_line (xtext->draw_buf, light, x - 1, y, x - 1, y + height);
				gdk_draw_line (xtext->draw_buf, dark, x, y, x, y + height);
			} else
			{
				gdk_draw_line (xtext->draw_buf, dark, x - 1, y, x - 1, y + height);
				gdk_draw_line (xtext->draw_buf, light, x, y, x, y + height);
			}
		}
	}
}

static void
gtk_xtext_paint (GtkWidget *widget, GdkRectangle *area)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	textentry *ent_start, *ent_end;
	int x;
#if defined(USE_XLIB) || defined(WIN32)
	int y;

	if (xtext->transparent)
	{
		gdk_window_get_origin (widget->window, &x, &y);
		/* update transparency only if it moved */
		if (xtext->last_win_x != x || xtext->last_win_y != y)
		{
			xtext->last_win_x = x;
			xtext->last_win_y = y;
			if (xtext->shaded)
			{
				xtext->recycle = TRUE;
				gtk_xtext_load_trans (xtext);
				xtext->recycle = FALSE;
			} else
			{
				gtk_xtext_free_trans (xtext);
				gtk_xtext_load_trans (xtext);
			}
		}
	}
#endif

	if (area->x == 0 && area->y == 0 &&
		 area->height == widget->allocation.height &&
		 area->width == widget->allocation.width)
	{
		dontscroll (xtext->buffer);	/* force scrolling off */
		gtk_xtext_render_page (xtext);
		return;
	}

	xtext_draw_bg (xtext, area->x, area->y, area->width, area->height);

	ent_start = gtk_xtext_find_char (xtext, area->x, area->y, NULL, NULL);
	if (!ent_start)
		goto xit;
	ent_end = gtk_xtext_find_char (xtext, area->x + area->width,
											 area->y + area->height, NULL, NULL);
	if (!ent_end)
		ent_end = xtext->buffer->text_last;

	/* can't over-write the same text with xft, or the AA will change */
	backend_set_clip (xtext, area);

	xtext->skip_fills = TRUE;
	xtext->skip_border_fills = TRUE;

	gtk_xtext_render_ents (xtext, ent_start, ent_end);

	xtext->skip_fills = FALSE;
	xtext->skip_border_fills = FALSE;

	backend_clear_clip (xtext);

xit:
	x = xtext->buffer->indent - ((xtext->space_width + 1) / 2);
	if (area->x <= x)
		gtk_xtext_draw_sep (xtext, -1);
}

static gboolean
gtk_xtext_expose (GtkWidget * widget, GdkEventExpose * event)
{
	gtk_xtext_paint (widget, &event->area);
	return FALSE;
}

/* render a selection that has extended or contracted upward */

static void
gtk_xtext_selection_up (GtkXText *xtext, textentry *start, textentry *end,
								int start_offset)
{
	/* render all the complete lines */
	if (start->next == end)
		gtk_xtext_render_ents (xtext, end, NULL);
	else
		gtk_xtext_render_ents (xtext, start->next, end);

	/* now the incomplete upper line */
	if (start == xtext->buffer->last_ent_start)
		xtext->jump_in_offset = xtext->buffer->last_offset_start;
	else
		xtext->jump_in_offset = start_offset;
	gtk_xtext_render_ents (xtext, start, NULL);
	xtext->jump_in_offset = 0;
}

/* render a selection that has extended or contracted downward */

static void
gtk_xtext_selection_down (GtkXText *xtext, textentry *start, textentry *end,
								  int end_offset)
{
	/* render all the complete lines */
	if (end->prev == start)
		gtk_xtext_render_ents (xtext, start, NULL);
	else
		gtk_xtext_render_ents (xtext, start, end->prev);

	/* now the incomplete bottom line */
	if (end == xtext->buffer->last_ent_end)
		xtext->jump_out_offset = xtext->buffer->last_offset_end;
	else
		xtext->jump_out_offset = end_offset;
	gtk_xtext_render_ents (xtext, end, NULL);
	xtext->jump_out_offset = 0;
}

static void
gtk_xtext_selection_render (GtkXText *xtext,
									 textentry *start_ent, int start_offset,
									 textentry *end_ent, int end_offset)
{
	textentry *ent;
	int start, end;

	xtext->skip_border_fills = TRUE;
	xtext->skip_stamp = TRUE;

	/* force an optimized render if there was no previous selection */
	if (xtext->buffer->last_ent_start == NULL && start_ent == end_ent)
	{
		xtext->buffer->last_offset_start = start_offset;
		xtext->buffer->last_offset_end = end_offset;
		goto lamejump;
	}

	/* mark changed within 1 ent only? */
	if (xtext->buffer->last_ent_start == start_ent &&
		 xtext->buffer->last_ent_end == end_ent)
	{
		/* when only 1 end of the selection is changed, we can really
			save on rendering */
		if (xtext->buffer->last_offset_start == start_offset ||
			 xtext->buffer->last_offset_end == end_offset)
		{
lamejump:
			ent = end_ent;
			/* figure out where to start and end the rendering */
			if (end_offset > xtext->buffer->last_offset_end)
			{
				end = end_offset;
				start = xtext->buffer->last_offset_end;
			} else if (end_offset < xtext->buffer->last_offset_end)
			{
				end = xtext->buffer->last_offset_end;
				start = end_offset;
			} else if (start_offset < xtext->buffer->last_offset_start)
			{
				end = xtext->buffer->last_offset_start;
				start = start_offset;
				ent = start_ent;
			} else if (start_offset > xtext->buffer->last_offset_start)
			{
				end = start_offset;
				start = xtext->buffer->last_offset_start;
				ent = start_ent;
			} else
			{	/* WORD selects end up here */
				end = end_offset;
				start = start_offset;
			}
		} else
		{
			/* LINE selects end up here */
			/* so which ent actually changed? */
			ent = start_ent;
			if (xtext->buffer->last_offset_start == start_offset)
				ent = end_ent;

			end = MAX (xtext->buffer->last_offset_end, end_offset);
			start = MIN (xtext->buffer->last_offset_start, start_offset);
		}

		xtext->jump_out_offset = end;
		xtext->jump_in_offset = start;
		gtk_xtext_render_ents (xtext, ent, NULL);
		xtext->jump_out_offset = 0;
		xtext->jump_in_offset = 0;
	}
	/* marking downward? */
	else if (xtext->buffer->last_ent_start == start_ent &&
				xtext->buffer->last_offset_start == start_offset)
	{
		/* find the range that covers both old and new selection */
		ent = start_ent;
		while (ent)
		{
			if (ent == xtext->buffer->last_ent_end)
			{
				gtk_xtext_selection_down (xtext, ent, end_ent, end_offset);
				/*gtk_xtext_render_ents (xtext, ent, end_ent);*/
				break;
			}
			if (ent == end_ent)
			{
				gtk_xtext_selection_down (xtext, ent, xtext->buffer->last_ent_end, end_offset);
				/*gtk_xtext_render_ents (xtext, ent, xtext->buffer->last_ent_end);*/
				break;
			}
			ent = ent->next;
		}
	}
	/* marking upward? */
	else if (xtext->buffer->last_ent_end == end_ent &&
				xtext->buffer->last_offset_end == end_offset)
	{
		ent = end_ent;
		while (ent)
		{
			if (ent == start_ent)
			{
				gtk_xtext_selection_up (xtext, xtext->buffer->last_ent_start, ent, start_offset);
				/*gtk_xtext_render_ents (xtext, xtext->buffer->last_ent_start, ent);*/
				break;
			}
			if (ent == xtext->buffer->last_ent_start)
			{
				gtk_xtext_selection_up (xtext, start_ent, ent, start_offset);
				/*gtk_xtext_render_ents (xtext, start_ent, ent);*/
				break;
			}
			ent = ent->prev;
		}
	}
	else	/* cross-over mark (stretched or shrunk at both ends) */
	{
		/* unrender the old mark */
		gtk_xtext_render_ents (xtext, xtext->buffer->last_ent_start, xtext->buffer->last_ent_end);
		/* now render the new mark, but skip overlaps */
		if (start_ent == xtext->buffer->last_ent_start)
			gtk_xtext_render_ents (xtext, start_ent->next, end_ent);
		else if (end_ent == xtext->buffer->last_ent_end)
			gtk_xtext_render_ents (xtext, start_ent, end_ent->prev);
		else
			gtk_xtext_render_ents (xtext, start_ent, end_ent);
	}

	xtext->buffer->last_ent_start = start_ent;
	xtext->buffer->last_ent_end = end_ent;
	xtext->buffer->last_offset_start = start_offset;
	xtext->buffer->last_offset_end = end_offset;

	xtext->skip_border_fills = FALSE;
	xtext->skip_stamp = FALSE;
}

static void
gtk_xtext_selection_draw (GtkXText * xtext, GdkEventMotion * event)
{
	textentry *ent;
	textentry *ent_end;
	textentry *ent_start;
	int offset_start;
	int offset_end;
	int low_x;
	int low_y;
	int high_x;
	int high_y;
	int tmp;

	if (xtext->select_start_y > xtext->select_end_y)
	{
		low_x = xtext->select_end_x;
		low_y = xtext->select_end_y;
		high_x = xtext->select_start_x;
		high_y = xtext->select_start_y;
	} else
	{
		low_x = xtext->select_start_x;
		low_y = xtext->select_start_y;
		high_x = xtext->select_end_x;
		high_y = xtext->select_end_y;
	}

	ent_start = gtk_xtext_find_char (xtext, low_x, low_y, &offset_start, &tmp);
	if (!ent_start)
	{
		if (xtext->adj->value != xtext->buffer->old_value)
			gtk_xtext_render_page (xtext);
		return;
	}

	ent_end = gtk_xtext_find_char (xtext, high_x, high_y, &offset_end, &tmp);
	if (!ent_end)
	{
		ent_end = xtext->buffer->text_last;
		if (!ent_end)
		{
			if (xtext->adj->value != xtext->buffer->old_value)
				gtk_xtext_render_page (xtext);
			return;
		}
		offset_end = ent_end->str_len;
	}

	/* marking less than a complete line? */
	/* make sure "start" is smaller than "end" (swap them if need be) */
	if (ent_start == ent_end && offset_start > offset_end)
	{
		tmp = offset_start;
		offset_start = offset_end;
		offset_end = tmp;
	}

	/* has the selection changed? Dont render unless necessary */
	if (xtext->buffer->last_ent_start == ent_start &&
		 xtext->buffer->last_ent_end == ent_end &&
		 xtext->buffer->last_offset_start == offset_start &&
		 xtext->buffer->last_offset_end == offset_end)
		return;

	/* set all the old mark_ fields to -1 */
	gtk_xtext_selection_clear (xtext->buffer);

	ent_start->mark_start = offset_start;
	ent_start->mark_end = offset_end;

	if (ent_start != ent_end)
	{
		ent_start->mark_end = ent_start->str_len;
		if (offset_end != 0)
		{
			ent_end->mark_start = 0;
			ent_end->mark_end = offset_end;
		}

		/* set all the mark_ fields of the ents within the selection */
		ent = ent_start->next;
		while (ent && ent != ent_end)
		{
			ent->mark_start = 0;
			ent->mark_end = ent->str_len;
			ent = ent->next;
		}
	}

	gtk_xtext_selection_render (xtext, ent_start, offset_start, ent_end, offset_end);
}

static gint
gtk_xtext_scrolldown_timeout (GtkXText * xtext)
{
	int p_y, win_height;

	gdk_window_get_pointer (GTK_WIDGET (xtext)->window, 0, &p_y, 0);
	gdk_drawable_get_size (GTK_WIDGET (xtext)->window, 0, &win_height);

	if (p_y > win_height &&
		 xtext->adj->value < (xtext->adj->upper - xtext->adj->page_size))
	{
		xtext->adj->value++;
		gtk_adjustment_changed (xtext->adj);
		gtk_xtext_render_page (xtext);
		return 1;
	}

	xtext->scroll_tag = 0;
	return 0;
}

static gint
gtk_xtext_scrollup_timeout (GtkXText * xtext)
{
	int p_y;

	gdk_window_get_pointer (GTK_WIDGET (xtext)->window, 0, &p_y, 0);

	if (p_y < 0 && xtext->adj->value > 0.0)
	{
		xtext->adj->value--;
		gtk_adjustment_changed (xtext->adj);
		gtk_xtext_render_page (xtext);
		return 1;
	}

	xtext->scroll_tag = 0;
	return 0;
}

static void
gtk_xtext_selection_update (GtkXText * xtext, GdkEventMotion * event, int p_y)
{
	int win_height;
	int moved;

	gdk_drawable_get_size (GTK_WIDGET (xtext)->window, 0, &win_height);

	/* selecting past top of window, scroll up! */
	if (p_y < 0 && xtext->adj->value >= 0)
	{
		if (!xtext->scroll_tag)
			xtext->scroll_tag = g_timeout_add (100,
															(GSourceFunc)
														 	gtk_xtext_scrollup_timeout,
															xtext);
		return;
	}

	/* selecting past bottom of window, scroll down! */
	if (p_y > win_height &&
		 xtext->adj->value < (xtext->adj->upper - xtext->adj->page_size))
	{
		if (!xtext->scroll_tag)
			xtext->scroll_tag = g_timeout_add (100,
															(GSourceFunc)
															gtk_xtext_scrolldown_timeout,
															xtext);
		return;
	}

	moved = (int)xtext->adj->value - xtext->select_start_adj;
	xtext->select_start_y -= (moved * xtext->fontsize);
	xtext->select_start_adj = xtext->adj->value;
	gtk_xtext_selection_draw (xtext, event);
}

static char *
gtk_xtext_get_word (GtkXText * xtext, int x, int y, textentry ** ret_ent,
						  int *ret_off, int *ret_len)
{
	textentry *ent;
	int offset;
	unsigned char *str;
	unsigned char *word;
	int len;
	int out_of_bounds;

	ent = gtk_xtext_find_char (xtext, x, y, &offset, &out_of_bounds);
	if (!ent)
		return 0;

	if (out_of_bounds)
		return 0;

	if (offset == ent->str_len)
		return 0;

	if (offset < 1)
		return 0;

	/*offset--;*/	/* FIXME: not all chars are 1 byte */

	str = ent->str + offset;

	while (!is_del (*str) && str != ent->str)
		str--;
	word = str + 1;

	len = 0;
	str = word;
	while (!is_del (*str) && len != ent->str_len)
	{
		str++;
		len++;
	}

	if (ret_ent)
		*ret_ent = ent;
	if (ret_off)
		*ret_off = word - ent->str;
	if (ret_len)
		*ret_len = str - word;

	return gtk_xtext_strip_color (word, len, xtext->scratch_buffer, NULL, NULL);
}

#ifdef MOTION_MONITOR

static void
gtk_xtext_unrender_hilight (GtkXText *xtext)
{
	xtext->render_hilights_only = TRUE;
	xtext->skip_border_fills = TRUE;
	xtext->skip_stamp = TRUE;
	xtext->un_hilight = TRUE;

	gtk_xtext_render_ents (xtext, xtext->hilight_ent, NULL);

	xtext->render_hilights_only = FALSE;
	xtext->skip_border_fills = FALSE;
	xtext->skip_stamp = FALSE;
	xtext->un_hilight = FALSE;
}

static gboolean
gtk_xtext_leave_notify (GtkWidget * widget, GdkEventCrossing * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);

	if (xtext->cursor_hand)
	{
		gtk_xtext_unrender_hilight (xtext);
		xtext->hilight_start = -1;
		xtext->hilight_end = -1;
		xtext->cursor_hand = FALSE;
		gdk_window_set_cursor (widget->window, 0);
		xtext->hilight_ent = NULL;
	}
	return FALSE;
}

#endif

static gboolean
gtk_xtext_motion_notify (GtkWidget * widget, GdkEventMotion * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	int tmp, x, y, offset, len;
	unsigned char *word;
	textentry *word_ent;

	gdk_window_get_pointer (widget->window, &x, &y, 0);

	if (xtext->moving_separator)
	{
		if (x < (3 * widget->allocation.width) / 5 && x > 15)
		{
			tmp = xtext->buffer->indent;
			xtext->buffer->indent = x;
			gtk_xtext_fix_indent (xtext->buffer);
			if (tmp != xtext->buffer->indent)
			{
				gtk_xtext_recalc_widths (xtext->buffer, FALSE);
				if (xtext->buffer->scrollbar_down)
					gtk_adjustment_set_value (xtext->adj, xtext->adj->upper -
													  xtext->adj->page_size);
				if (!xtext->io_tag)
					xtext->io_tag = g_timeout_add (REFRESH_TIMEOUT,
																(GSourceFunc)
																gtk_xtext_adjustment_timeout,
																xtext);
			}
		}
		return FALSE;
	}

	if (xtext->button_down)
	{
		gtk_grab_add (widget);
		/*gdk_pointer_grab (widget->window, TRUE,
									GDK_BUTTON_RELEASE_MASK |
									GDK_BUTTON_MOTION_MASK, NULL, NULL, 0);*/
		xtext->select_end_x = x;
		xtext->select_end_y = y;
		gtk_xtext_selection_update (xtext, event, y);
		return FALSE;
	}
#ifdef MOTION_MONITOR

	if (xtext->urlcheck_function == NULL)
		return FALSE;

	word = gtk_xtext_get_word (xtext, x, y, &word_ent, &offset, &len);
	if (word)
	{
		if (xtext->urlcheck_function (GTK_WIDGET (xtext), word) > 0)
		{
			if (!xtext->cursor_hand ||
				 xtext->hilight_ent != word_ent ||
				 xtext->hilight_start != offset ||
				 xtext->hilight_end != offset + len)
			{
				if (!xtext->cursor_hand)
				{
					gdk_window_set_cursor (GTK_WIDGET (xtext)->window,
											  		xtext->hand_cursor);
					xtext->cursor_hand = TRUE;
				}

				/* un-render the old hilight */
				if (xtext->hilight_ent)
					gtk_xtext_unrender_hilight (xtext);

				xtext->hilight_ent = word_ent;
				xtext->hilight_start = offset;
				xtext->hilight_end = offset + len;

				xtext->skip_border_fills = TRUE;
				xtext->render_hilights_only = TRUE;
				xtext->skip_stamp = TRUE;

				gtk_xtext_render_ents (xtext, word_ent, NULL);

				xtext->skip_border_fills = FALSE;
				xtext->render_hilights_only = FALSE;
				xtext->skip_stamp = FALSE;
			}
			return FALSE;
		}
	}

	gtk_xtext_leave_notify (widget, NULL);

#endif

	return FALSE;
}

static void
gtk_xtext_set_clip_owner (GtkWidget * xtext, GdkEventButton * event)
{
#ifdef WIN32
	gtk_selection_owner_set (xtext, gdk_atom_intern ("CLIPBOARD", FALSE),
									 event->time);
#endif
	gtk_selection_owner_set (xtext, GDK_SELECTION_PRIMARY, event->time);

	if (GTK_XTEXT (xtext)->selection_buffer &&
		GTK_XTEXT (xtext)->selection_buffer != GTK_XTEXT (xtext)->buffer)
		gtk_xtext_selection_clear (GTK_XTEXT (xtext)->selection_buffer);

	GTK_XTEXT (xtext)->selection_buffer = GTK_XTEXT (xtext)->buffer;
}

static gboolean
gtk_xtext_button_release (GtkWidget * widget, GdkEventButton * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	unsigned char *word;
	int old;

	if (xtext->moving_separator)
	{
		xtext->moving_separator = FALSE;
		old = xtext->buffer->indent;
		if (event->x < (4 * widget->allocation.width) / 5 && event->x > 15)
			xtext->buffer->indent = event->x;
		gtk_xtext_fix_indent (xtext->buffer);
		if (xtext->buffer->indent != old)
		{
			gtk_xtext_recalc_widths (xtext->buffer, FALSE);
			gtk_xtext_adjustment_set (xtext->buffer, TRUE);
			gtk_xtext_render_page (xtext);
		} else
			gtk_xtext_draw_sep (xtext, -1);
		return FALSE;
	}

	if (xtext->word_or_line_select)
	{
		xtext->word_or_line_select = FALSE;
		xtext->button_down = FALSE;
		return FALSE;
	}

	if (event->button == 1)
	{
		xtext->button_down = FALSE;

		gtk_grab_remove (widget);
		/*gdk_pointer_ungrab (0);*/
		if (xtext->buffer->last_ent_start)
			gtk_xtext_set_clip_owner (GTK_WIDGET (xtext), event);

		if (xtext->select_start_x == event->x &&
			 xtext->select_start_y == event->y)
		{
			if (gtk_xtext_selection_clear (xtext->buffer))
			{
				xtext->skip_border_fills = TRUE;
				xtext->skip_stamp = TRUE;
				gtk_xtext_render_ents (xtext, xtext->buffer->last_ent_start,
											  xtext->buffer->last_ent_end);
				xtext->skip_border_fills = FALSE;
				xtext->skip_stamp = FALSE;

				xtext->buffer->last_ent_start = NULL;
				xtext->buffer->last_ent_end = NULL;
			}
		} else
		{
			word = gtk_xtext_get_word (xtext, event->x, event->y, 0, 0, 0);
			if (word)
			{
				g_signal_emit (G_OBJECT (xtext), xtext_signals[WORD_CLICK], 0,
									word, event);
				return FALSE;
			}
		}
	}

	return FALSE;
}

static gboolean
gtk_xtext_button_press (GtkWidget * widget, GdkEventButton * event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	textentry *ent;
	unsigned char *word;
	int line_x, x, y, offset, len;

	gdk_window_get_pointer (widget->window, &x, &y, 0);

	if (event->button == 3)		  /* right click */
	{
		word = gtk_xtext_get_word (xtext, x, y, 0, 0, 0);
		if (word)
		{
			g_signal_emit (G_OBJECT (xtext), xtext_signals[WORD_CLICK], 0,
								word, event);
		} else
			g_signal_emit (G_OBJECT (xtext), xtext_signals[WORD_CLICK], 0,
								"", event);
		return FALSE;
	}

	if (event->button == 2)
	{
		g_signal_emit (G_OBJECT (xtext), xtext_signals[WORD_CLICK], 0, "", event);
		return FALSE;
	}

	if (event->button != 1)		  /* we only want left button */
		return FALSE;

	if (event->type == GDK_2BUTTON_PRESS)	/* WORD select */
	{
		if (gtk_xtext_get_word (xtext, x, y, &ent, &offset, &len))
		{
			if (len == 0)
				return FALSE;
			gtk_xtext_selection_clear (xtext->buffer);
			ent->mark_start = offset;
			ent->mark_end = offset + len;
			gtk_xtext_selection_render (xtext, ent, offset, ent, offset + len);
			xtext->word_or_line_select = TRUE;
			gtk_xtext_set_clip_owner (GTK_WIDGET (xtext), event);
		}

		return FALSE;
	}

	if (event->type == GDK_3BUTTON_PRESS)	/* LINE select */
	{
		if (gtk_xtext_get_word (xtext, x, y, &ent, 0, 0))
		{
			gtk_xtext_selection_clear (xtext->buffer);
			ent->mark_start = 0;
			ent->mark_end = ent->str_len;
			gtk_xtext_selection_render (xtext, ent, 0, ent, ent->str_len);
			xtext->word_or_line_select = TRUE;
			gtk_xtext_set_clip_owner (GTK_WIDGET (xtext), event);
		}

		return FALSE;
	}

	/* check if it was a separator-bar click */
	if (xtext->separator && xtext->buffer->indent)
	{
		line_x = xtext->buffer->indent - ((xtext->space_width + 1) / 2);
		if (line_x == x || line_x == x + 1 || line_x == x - 1)
		{
			xtext->moving_separator = TRUE;
			/* draw the separator line */
			gtk_xtext_draw_sep (xtext, -1);
			return FALSE;
		}
	}

	xtext->button_down = TRUE;
	xtext->select_start_x = x;
	xtext->select_start_y = y;
	xtext->select_start_adj = xtext->adj->value;

	return FALSE;
}

/* another program has claimed the selection */

static gboolean
gtk_xtext_selection_kill (GtkXText *xtext, GdkEventSelection *event)
{
#ifndef WIN32
	if (gtk_xtext_selection_clear (xtext->buffer))
	{
		xtext->skip_border_fills = TRUE;
		xtext->skip_stamp = TRUE;
		gtk_xtext_render_ents (xtext, xtext->buffer->last_ent_start,
									  xtext->buffer->last_ent_end);
		xtext->skip_border_fills = FALSE;
		xtext->skip_stamp = FALSE;

		xtext->buffer->last_ent_start = NULL;
		xtext->buffer->last_ent_end = NULL;
	}
#endif
	return TRUE;
}

/* another program is asking for our selection */

static void
gtk_xtext_selection_get (GtkWidget * widget,
								 GtkSelectionData * selection_data_ptr,
								 guint info, guint time)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	textentry *ent;
	char *txt;
	char *pos;
	char *stripped;
	guchar *new_text;
	int len;
	int first = TRUE;
	xtext_buffer *buf;

	buf = xtext->selection_buffer;
	if (!buf)
		return;

	/* first find out how much we need to malloc ... */
	len = 0;
	ent = buf->last_ent_start;
	while (ent)
	{
		if (ent->mark_start != -1)
		{
			if (ent->mark_end - ent->mark_start > 0)
				len += (ent->mark_end - ent->mark_start) + 1;
			else
				len++;
		}
		if (ent == buf->last_ent_end)
			break;
		ent = ent->next;
	}

	if (len < 1)
		return;

	/* now allocate mem and copy buffer */
	pos = txt = malloc (len);
	ent = buf->last_ent_start;
	while (ent)
	{
		if (ent->mark_start != -1)
		{
			if (!first)
			{
				*pos = '\n';
				pos++;
			}
			first = FALSE;
			if (ent->mark_end - ent->mark_start > 0)
			{
				memcpy (pos, ent->str + ent->mark_start,
						  ent->mark_end - ent->mark_start);
				pos += ent->mark_end - ent->mark_start;
			}
		}
		if (ent == buf->last_ent_end)
			break;
		ent = ent->next;
	}
	*pos = 0;

	if (xtext->color_paste)
		stripped = gtk_xtext_conv_color (txt, strlen (txt), &len);
	else
		stripped = gtk_xtext_strip_color (txt, strlen (txt), NULL, &len, 0);
	free (txt);

	switch (info)
	{
	case TARGET_UTF8_STRING:
		/* it's already in utf8 */
		gtk_selection_data_set_text (selection_data_ptr, stripped, len);
		break;
	case TARGET_TEXT:
	case TARGET_COMPOUND_TEXT:
		{
			GdkAtom encoding;
			gint format;
			gint new_length;

			gdk_string_to_compound_text (stripped, &encoding, &format, &new_text,
												  &new_length);
			gtk_selection_data_set (selection_data_ptr, encoding, format,
											new_text, new_length);
			gdk_free_compound_text (new_text);
		}
		break;
	default:
		new_text = g_locale_from_utf8 (stripped, len, NULL, &len, NULL);
		gtk_selection_data_set (selection_data_ptr, GDK_SELECTION_TYPE_STRING,
										8, new_text, len);
		g_free (new_text);
	}

	free (stripped);
}

static gboolean
gtk_xtext_scroll (GtkWidget *widget, GdkEventScroll *event)
{
	GtkXText *xtext = GTK_XTEXT (widget);
	int new_value;

	if (event->direction == GDK_SCROLL_UP)		/* mouse wheel pageUp */
	{
		new_value = xtext->adj->value - (xtext->adj->page_increment / 10);
		if (new_value < xtext->adj->lower)
			new_value = xtext->adj->lower;
		gtk_adjustment_set_value (xtext->adj, new_value);
	}
	else if (event->direction == GDK_SCROLL_DOWN)	/* mouse wheel pageDn */
	{
		new_value = xtext->adj->value + (xtext->adj->page_increment / 10);
		if (new_value > (xtext->adj->upper - xtext->adj->page_size))
			new_value = xtext->adj->upper - xtext->adj->page_size;
		gtk_adjustment_set_value (xtext->adj, new_value);
	}

	return FALSE;
}

static void
gtk_xtext_class_init (GtkXTextClass * class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkXTextClass *xtext_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	xtext_class = (GtkXTextClass *) class;

	parent_class = gtk_type_class (gtk_widget_get_type ());

	xtext_signals[WORD_CLICK] =
		g_signal_new ("word_click",
							G_TYPE_FROM_CLASS (object_class),
							G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
							G_STRUCT_OFFSET (GtkXTextClass, word_click),
							NULL, NULL,
							gtk_marshal_VOID__POINTER_POINTER,
							G_TYPE_NONE,
							2, G_TYPE_POINTER, G_TYPE_POINTER);
	object_class->destroy = gtk_xtext_destroy;

	widget_class->realize = gtk_xtext_realize;
	widget_class->unrealize = gtk_xtext_unrealize;
	widget_class->size_request = gtk_xtext_size_request;
	widget_class->size_allocate = gtk_xtext_size_allocate;
	widget_class->button_press_event = gtk_xtext_button_press;
	widget_class->button_release_event = gtk_xtext_button_release;
	widget_class->motion_notify_event = gtk_xtext_motion_notify;
	widget_class->selection_clear_event = (void *)gtk_xtext_selection_kill;
	widget_class->selection_get = gtk_xtext_selection_get;
	widget_class->expose_event = gtk_xtext_expose;
	widget_class->scroll_event = gtk_xtext_scroll;
#ifdef MOTION_MONITOR
	widget_class->leave_notify_event = gtk_xtext_leave_notify;
#endif

	xtext_class->word_click = NULL;
}

static GtkType
gtk_xtext_get_type (void)
{
	static GtkType xtext_type = 0;

	if (!xtext_type)
	{
		static const GTypeInfo xtext_info =
		{
			sizeof (GtkXTextClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gtk_xtext_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GtkXText),
			0,		/* n_preallocs */
			(GInstanceInitFunc) gtk_xtext_init,
		};

		xtext_type = g_type_register_static (GTK_TYPE_WIDGET, "GtkXText",
														 &xtext_info, 0);
	}

	return xtext_type;
}

/* strip MIRC colors and other attribs. */

static unsigned char *
gtk_xtext_strip_color (unsigned char *text, int len, unsigned char *outbuf,
							  int *newlen, int *mb_ret)
{
	int nc = 0;
	int i = 0;
	int col = FALSE;
	unsigned char *new_str;
	int mbl;
	int mb = FALSE;

	if (outbuf == NULL)
		new_str = malloc (len + 2);
	else
		new_str = outbuf;

	while (len > 0)
	{
		if (*text >= 128)
			mb = TRUE;

		if ((col && isdigit (*text) && nc < 2) ||
			 (col && *text == ',' && isdigit (*(text+1)) && nc < 3))
		{
			nc++;
			if (*text == ',')
				nc = 0;
		} else
		{
			col = FALSE;
			switch (*text)
			{
			case ATTR_COLOR:
				col = TRUE;
				nc = 0;
				break;
			case ATTR_BEEP:
			case ATTR_RESET:
			case ATTR_REVERSE:
			case ATTR_BOLD:
			case ATTR_UNDERLINE:
				break;
			default:
				mbl = charlen (text);
				if (mbl == 1)
				{
					new_str[i] = *text;
					i++;
					text++;
					len--;
				} else
				{
					mb = TRUE;

					len -= mbl;
					/* safe-guard against invalid utf8 */
					if (len < 0)
						/* avoid memcpy beyond buffer */
						mbl += len;
					memcpy (&new_str[i], text, mbl);
					i += mbl;
					text += mbl;
				}
				continue;
			}
		}
		text++;
		len--;
	}

	new_str[i] = 0;

	if (newlen != NULL)
		*newlen = i;

	if (mb_ret != NULL)
		*mb_ret = mb;

	return new_str;
}

/* GeEkMaN: converts mIRC control codes to literal control codes */

static char *
gtk_xtext_conv_color (unsigned char *text, int len, int *newlen)
{
	int i, j = 2;
	char cchar = 0;
	char *new_str;
	int mbl;

	for (i = 0; i < len;)
	{
		switch (text[i])
		{
		case ATTR_COLOR:
		case ATTR_RESET:
		case ATTR_REVERSE:
		case ATTR_BOLD:
		case ATTR_UNDERLINE:
			j += 3;
			i++;
			break;
		default:
			mbl = charlen (text + i);
			j += mbl;
			i += mbl;
		}
	}

	new_str = malloc (j);
	j = 0;

	for (i = 0; i < len;)
	{
		switch (text[i])
		{
		case ATTR_COLOR:
			cchar = 'C';
			break;
		case ATTR_RESET:
			cchar = 'O';
			break;
		case ATTR_REVERSE:
			cchar = 'R';
			break;
		case ATTR_BOLD:
			cchar = 'B';
			break;
		case ATTR_UNDERLINE:
			cchar = 'U';
			break;
		case ATTR_BEEP:
			break;
		default:
			mbl = charlen (text + i);
			if (mbl == 1)
			{
				new_str[j] = text[i];
				j++;
				i++;
			} else
			{
				/* invalid utf8 safe guard */
				if (i + mbl > len)
					mbl = len - i;
				memcpy (new_str + j, text + i, mbl);
				j += mbl;
				i += mbl;
			}
		}
		if (cchar != 0)
		{
			new_str[j++] = '%';
			new_str[j++] = cchar;
			cchar = 0;
			i++;
		}
	}

	new_str[j] = 0;
	*newlen = j;

	return new_str;
}

/* gives width of a string, excluding the mIRC codes */

static int
gtk_xtext_text_width (GtkXText *xtext, unsigned char *text, int len,
							 int *mb_ret)
{
	unsigned char *new_buf;
	int new_len, mb;

	new_buf = gtk_xtext_strip_color (text, len, xtext->scratch_buffer,
												&new_len, &mb);

	if (mb_ret)
		*mb_ret = mb;

	return backend_get_text_width (xtext, new_buf, new_len, mb);
}

/* actually draw text to screen (one run with the same color/attribs) */

static int
gtk_xtext_render_flush (GtkXText * xtext, int x, int y, unsigned char *str,
								int len, GdkGC *gc, int is_mb)
{
	int str_width, dofill;

	if (xtext->dont_render || len < 1)
		return 0;

	str_width = backend_get_text_width (xtext, str, len, is_mb);

	if (xtext->dont_render2)
		return str_width;

	if (xtext->render_hilights_only)
	{
		if (!xtext->in_hilight)	/* is it a hilight prefix? */
			return str_width;
#ifndef COLOR_HILIGHT
		if (!xtext->un_hilight)	/* doing a hilight? no need to draw the text */
			goto dounder;
#endif
	}

	dofill = TRUE;

	/* backcolor is always handled by XDrawImageString */
	if (!xtext->backcolor)
	{
		dofill = !xtext->skip_fills;
		if (dofill && xtext->pixmap)
		{
	/* draw the background pixmap behind the text - CAUSES FLICKER HERE!! */
			xtext_draw_bg (xtext, x, y - xtext->font->ascent, str_width,
								xtext->fontsize);
			dofill = FALSE;	/* already drawn the background */
		}
	}

	backend_draw_text (xtext, dofill, gc, x, y, str, len, str_width, is_mb);

	if (xtext->underline)
	{
#ifdef USE_XFT
		GdkColor col;
#endif

#ifndef COLOR_HILIGHT
dounder:
#endif

#ifdef USE_XFT
		col.pixel = xtext->xft_fg->pixel;
		gdk_gc_set_foreground (gc, &col);
#endif

		gdk_draw_line (xtext->draw_buf, gc, x, y + 1, x + str_width - 1, y + 1);
	}

	return str_width;
}

static void
gtk_xtext_reset (GtkXText * xtext, int mark, int attribs)
{
	if (attribs)
	{
		xtext->underline = FALSE;
		xtext->bold = FALSE;
	}
	if (!mark)
	{
		xtext->backcolor = FALSE;
		if (xtext->col_fore != 18)
			xtext_set_fg (xtext, xtext->fgc, 18);
		if (xtext->col_back != 19)
			xtext_set_bg (xtext, xtext->fgc, 19);
	}
	xtext->col_fore = 18;
	xtext->col_back = 19;
	xtext->parsing_color = FALSE;
	xtext->parsing_backcolor = FALSE;
	xtext->nc = 0;
}

/* render a single line, which WONT wrap, and parse mIRC colors */

static int
gtk_xtext_render_str (GtkXText * xtext, int y, textentry * ent, unsigned char *str,
							 int len, int win_width, int indent, int line)
{
	GdkGC *gc;
	int i = 0, x = indent, j = 0;
	unsigned char *pstr = str;
	int col_num, tmp;
	int offset;
	int mark = FALSE;
	int ret = 1;

	xtext->in_hilight = FALSE;

	offset = str - ent->str;

	if (line < 255 && line >= 0)
		xtext->buffer->grid_offset[line] = offset;

	gc = xtext->fgc;				  /* our foreground GC */

	if (ent->mark_start != -1 &&
		 ent->mark_start <= i + offset && ent->mark_end > i + offset)
	{
		xtext_set_bg (xtext, gc, 16);
		xtext_set_fg (xtext, gc, 17);
		xtext->backcolor = TRUE;
		mark = TRUE;
	}
#ifdef MOTION_MONITOR
	if (xtext->hilight_ent == ent &&
		 xtext->hilight_start <= i + offset && xtext->hilight_end > i + offset)
	{
		if (!xtext->un_hilight)
		{
#ifdef COLOR_HILIGHT
			xtext_set_bg (xtext, gc, 2);
#else
			xtext->underline = TRUE;
#endif
		}
		xtext->in_hilight = TRUE;
	}
#endif

	if (!xtext->skip_border_fills)
	{
		/* draw background to the left of the text */
		if (str == ent->str && indent && xtext->buffer->time_stamp)
		{
			/* don't overwrite the timestamp */
			if (indent > xtext->stamp_width)
			{
				xtext_draw_bg (xtext, xtext->stamp_width, y - xtext->font->ascent,
									indent - xtext->stamp_width, xtext->fontsize);
			}
		} else
		{
			/* fill the indent area with background gc */
			xtext_draw_bg (xtext, 0, y - xtext->font->ascent, indent,
								xtext->fontsize);
		}
	}

	if (xtext->jump_in_offset > 0 && offset < xtext->jump_in_offset)
		xtext->dont_render2 = TRUE;

	while (i < len)
	{

#ifdef MOTION_MONITOR
		if (xtext->hilight_ent == ent && xtext->hilight_start == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			if (!xtext->un_hilight)
			{
#ifdef COLOR_HILIGHT
				xtext_set_bg (xtext, gc, 2);
#else
				xtext->underline = TRUE;
#endif
			}

			xtext->in_hilight = TRUE;
		}
#endif

		if ((xtext->parsing_color && isdigit (str[i]) && xtext->nc < 2) ||
			 (xtext->parsing_color && str[i] == ',' && isdigit (str[i+1]) && xtext->nc < 3))
		{
			pstr++;
			if (str[i] == ',')
			{
				xtext->parsing_backcolor = TRUE;
				if (xtext->nc)
				{
					xtext->num[xtext->nc] = 0;
					xtext->nc = 0;
					col_num = atoi (xtext->num);
					if (col_num == 99)	/* mIRC lameness */
						col_num = 18;
					else
						col_num = col_num % 16;
					xtext->col_fore = col_num;
					if (!mark)
						xtext_set_fg (xtext, gc, col_num);
				}
			} else
			{
				xtext->num[xtext->nc] = str[i];
				if (xtext->nc < 7)
					xtext->nc++;
			}
		} else
		{
			if (xtext->parsing_color)
			{
				xtext->parsing_color = FALSE;
				if (xtext->nc)
				{
					xtext->num[xtext->nc] = 0;
					xtext->nc = 0;
					col_num = atoi (xtext->num);
					if (xtext->parsing_backcolor)
					{
						if (col_num == 99)	/* mIRC lameness */
							col_num = 19;
						else
							col_num = col_num % 16;
						if (col_num == 19)
							xtext->backcolor = FALSE;
						else
							xtext->backcolor = TRUE;
						if (!mark)
							xtext_set_bg (xtext, gc, col_num);
						xtext->col_back = col_num;
					} else
					{
						if (col_num == 99)	/* mIRC lameness */
							col_num = 18;
						else
							col_num = col_num % 16;
						if (!mark)
							xtext_set_fg (xtext, gc, col_num);
						xtext->col_fore = col_num;
					}
					xtext->parsing_backcolor = FALSE;
				} else
				{
					/* got a \003<non-digit>... i.e. reset colors */
					x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
					pstr += j;
					j = 0;
					gtk_xtext_reset (xtext, mark, FALSE);
				}
			}

			switch (str[i])
			{
			/* FIXME for non-fixed width fonts. \t may not match ' ' width */
			case '\t':
				str[i] = ' ';
				j++;
				break;
			case '\n':
			/*case ATTR_BEEP:*/
				break;
			case ATTR_REVERSE:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				pstr += j + 1;
				j = 0;
				tmp = xtext->col_fore;
				xtext->col_fore = xtext->col_back;
				xtext->col_back = tmp;
				if (!mark)
				{
					xtext_set_fg (xtext, gc, xtext->col_fore);
					xtext_set_bg (xtext, gc, xtext->col_back);
				}
				if (xtext->col_back != 19)
					xtext->backcolor = TRUE;
				else
					xtext->backcolor = FALSE;
				break;
			case ATTR_BOLD:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				xtext->bold = !xtext->bold;
				pstr += j + 1;
				j = 0;
				break;
			case ATTR_UNDERLINE:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				xtext->underline = !xtext->underline;
				pstr += j + 1;
				j = 0;
				break;
			case ATTR_RESET:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				pstr += j + 1;
				j = 0;
				gtk_xtext_reset (xtext, mark, !xtext->in_hilight);
				break;
			case ATTR_COLOR:
				x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
				xtext->parsing_color = TRUE;
				pstr += j + 1;
				j = 0;
				break;
			default:
				tmp = charlen (str + i);
				/* invalid utf8 safe guard */
				if (tmp + i > len)
					tmp = len - i;
				j += tmp;	/* move to the next utf8 char */
			}
		}
		i += charlen (str + i);	/* move to the next utf8 char */
		/* invalid utf8 safe guard */
		if (i > len)
			i = len;

		/* have we been told to stop rendering at this point? */
		if (xtext->jump_out_offset > 0 && xtext->jump_out_offset <= (i + offset))
		{
			gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			ret = 0;	/* skip the rest of the lines, we're done. */
			j = 0;
			break;
		}

		if (xtext->jump_in_offset > 0 && xtext->jump_in_offset == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext->dont_render2 = FALSE;
		}

#ifdef MOTION_MONITOR
		if (xtext->hilight_ent == ent && xtext->hilight_end == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
#ifdef COLOR_HILIGHT
			if (mark)
			{
				xtext_set_bg (xtext, gc, 16);
				xtext->backcolor = TRUE;
			} else
			{
				xtext_set_bg (xtext, gc, xtext->col_back);
				if (xtext->col_back != 19)
					xtext->backcolor = TRUE;
				else
					xtext->backcolor = FALSE;
			}
#else
			xtext->underline = FALSE;
#endif
			xtext->in_hilight = FALSE;
			if (xtext->render_hilights_only)
			{
				/* stop drawing this ent */
				ret = 0;
				break;
			}
		}
#endif

		if (!mark && ent->mark_start == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext_set_bg (xtext, gc, 16);
			xtext_set_fg (xtext, gc, 17);
			xtext->backcolor = TRUE;
			mark = TRUE;
		}

		if (mark && ent->mark_end == (i + offset))
		{
			x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);
			pstr += j;
			j = 0;
			xtext_set_bg (xtext, gc, xtext->col_back);
			xtext_set_fg (xtext, gc, xtext->col_fore);
			if (xtext->col_back != 19)
				xtext->backcolor = TRUE;
			else
				xtext->backcolor = FALSE;
			mark = FALSE;
		}

	}

	if (j)
		x += gtk_xtext_render_flush (xtext, x, y, pstr, j, gc, ent->mb);

	if (mark)
	{
		xtext_set_bg (xtext, gc, xtext->col_back);
		xtext_set_fg (xtext, gc, xtext->col_fore);
		if (xtext->col_back != 19)
			xtext->backcolor = TRUE;
		else
			xtext->backcolor = FALSE;
	}

	/* draw separator now so it doesn't appear to flicker */
	gtk_xtext_draw_sep (xtext, y - xtext->font->ascent);
	/* draw background to the right of the text */
	if (!xtext->skip_border_fills)
		xtext_draw_bg (xtext, x, y - xtext->font->ascent,
							(win_width + MARGIN) - x, xtext->fontsize);

	xtext->dont_render2 = FALSE;

	return ret;
}

#ifdef USE_XLIB

/* get the desktop/root window */

static Window desktop_window = None;

static Window
get_desktop_window (Window the_window)
{
	Atom prop, type;
	int format;
	unsigned long length, after;
	unsigned char *data;
	unsigned int nchildren;
	Window w, root, *children, parent;

	prop = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", True);
	if (prop == None)
	{
		prop = XInternAtom (GDK_DISPLAY (), "_XROOTCOLOR_PIXEL", True);
		if (prop == None)
			return None;
	}

	for (w = the_window; w; w = parent)
	{
		if ((XQueryTree (GDK_DISPLAY (), w, &root, &parent, &children,
				&nchildren)) == False)
			return None;

		if (nchildren)
			XFree (children);

		XGetWindowProperty (GDK_DISPLAY (), w, prop, 0L, 1L, False,
								  AnyPropertyType, &type, &format, &length, &after,
								  &data);
		if (data)
			XFree (data);

		if (type != None)
			return (desktop_window = w);
	}

	return (desktop_window = None);
}

/* find the root window (backdrop) Pixmap */

static Pixmap
get_pixmap_prop (Window the_window)
{
	Atom type;
	int format;
	unsigned long length, after;
	unsigned char *data;
	Pixmap pix = None;
	static Atom prop = None;

	if (desktop_window == None)
		desktop_window = get_desktop_window (the_window);
	if (desktop_window == None)
		desktop_window = GDK_ROOT_WINDOW ();

	if (prop == None)
		prop = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", True);
	if (prop == None)
		return None;

	XGetWindowProperty (GDK_DISPLAY (), desktop_window, prop, 0L, 1L, False,
							  AnyPropertyType, &type, &format, &length, &after,
							  &data);
	if (data)
	{
		if (type == XA_PIXMAP)
			pix = *((Pixmap *) data);

		XFree (data);
	}

	return pix;
}

#ifdef USE_MMX

#include "mmx_cmod.h"

static GdkPixmap *
shade_pixmap_mmx (GtkXText * xtext, Pixmap p, int x, int y, int w, int h)
{
	int dummy, width, height, depth;
	GdkPixmap *shaded_pix;
	Window root;
	Pixmap tmp;
	XImage *ximg;
	XGCValues gcv;
	GC tgc;

	XGetGeometry (GDK_DISPLAY (), p, &root, &dummy, &dummy, &width, &height,
					  &dummy, &depth);

	if (width < x + w || height < y + h || x < 0 || y < 0)
	{
		gcv.subwindow_mode = IncludeInferiors;
		gcv.graphics_exposures = False;
		tgc = XCreateGC (GDK_DISPLAY (), p, GCGraphicsExposures|GCSubwindowMode,
							  &gcv);
		tmp = XCreatePixmap (GDK_DISPLAY (), p, w, h, depth);
		XSetTile (GDK_DISPLAY (), tgc, p);
		XSetFillStyle (GDK_DISPLAY (), tgc, FillTiled);
		XSetTSOrigin (GDK_DISPLAY (), tgc, -x, -y);
		XFillRectangle (GDK_DISPLAY (), tmp, tgc, 0, 0, w, h);
		XFreeGC (GDK_DISPLAY (), tgc);

		ximg = XGetImage (GDK_DISPLAY (), tmp, 0, 0, w, h, -1, ZPixmap);
		XFreePixmap (GDK_DISPLAY (), tmp);
	} else
	{
		ximg = XGetImage (GDK_DISPLAY (), p, x, y, w, h, -1, ZPixmap);
	}

	switch (depth)
	{
	/* 1 and 8 not supported */
	case 15:
		shade_ximage_15_mmx (ximg->data, ximg->bytes_per_line, w, h,
						xtext->tint_red, xtext->tint_green, xtext->tint_blue);
		break;
	case 16:
		shade_ximage_16_mmx (ximg->data, ximg->bytes_per_line, w, h,
						xtext->tint_red, xtext->tint_green, xtext->tint_blue);
		break;
	case 24:
		if (ximg->bits_per_pixel != 32)
			break;
	case 32:
		shade_ximage_32_mmx (ximg->data, ximg->bytes_per_line, w, h,
						xtext->tint_red, xtext->tint_green, xtext->tint_blue);
		break;
	}

	if (xtext->recycle)
		shaded_pix = xtext->pixmap;
	else
		shaded_pix = gdk_pixmap_new (GTK_WIDGET (xtext)->window, w, h, depth);

	XPutImage (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (shaded_pix),
				  GDK_GC_XGC (xtext->fgc), ximg, 0, 0, 0, 0, w, h);

	XDestroyImage (ximg);

	return shaded_pix;
}

#endif	/* !USE_MMX */

#ifdef USE_GDK_PIXBUF

static GdkPixmap *
shade_pixmap_gdk (GtkXText * xtext, Pixmap p, int x, int y, int w, int h)
{
	GdkPixmap *pp, *tmp, *shaded_pixmap;
	GdkPixbuf *pixbuf;
	GdkColormap *cmap;
	GdkGC *tgc;
	unsigned char *buf;
	unsigned char *pbuf;
	int width, height, depth;
	int rowstride;
	int pbwidth;
	int pbheight;
	int i, j;
	int offset;
	int r, g, b, a;

	pp = gdk_pixmap_foreign_new (p);
	cmap = gtk_widget_get_colormap (GTK_WIDGET (xtext));
	gdk_drawable_get_size (pp, &width, &height);
	depth = gdk_drawable_get_depth (pp);

	if (width < x + w || height < y + h || x < 0 || y < 0)
	{
		tgc = gdk_gc_new (pp);
		tmp = gdk_pixmap_new (pp, w, h, depth);
		gdk_gc_set_tile (tgc, pp);
		gdk_gc_set_fill (tgc, GDK_TILED);
		gdk_gc_set_ts_origin (tgc, -x, -y);
		gdk_draw_rectangle (tmp, tgc, TRUE, 0, 0, w, h);
		g_object_unref (tgc);

		pixbuf = gdk_pixbuf_get_from_drawable (NULL, tmp, cmap,
															0, 0, 0, 0, w, h);
		g_object_unref (tmp);
	} else
	{
		pixbuf = gdk_pixbuf_get_from_drawable (NULL, pp, cmap,
															x, y, 0, 0, w, h);
	}
	g_object_unref (pp);

	if (!pixbuf)
		return NULL;

	buf = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pbwidth = gdk_pixbuf_get_width (pixbuf);
	pbheight = gdk_pixbuf_get_height (pixbuf);

	a = 128;	/* alpha */
	r = xtext->tint_red;
	g = xtext->tint_green;
	b = xtext->tint_blue;

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		offset = 4;
	else
		offset = 3;

	for (i=0;i<pbheight;i++)
	{
		pbuf = buf;
		for (j=0;j<pbwidth;j++)
		{
			pbuf[0] = ((pbuf[0] * r) >> 8);
			pbuf[1] = ((pbuf[1] * g) >> 8);
			pbuf[2] = ((pbuf[2] * b) >> 8);
			pbuf+=offset;
		}
		buf+=rowstride;
	}

	/* reuse the same pixmap to save a few cycles */
	if (xtext->recycle)
	{
		shaded_pixmap = xtext->pixmap;
		gdk_pixbuf_render_to_drawable (pixbuf, shaded_pixmap, xtext->fgc, 0, 0,
												 0, 0, w, h, GDK_RGB_DITHER_NORMAL, 0, 0);
	} else
	{
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &shaded_pixmap, NULL, 0);
	}
	g_object_unref (pixbuf);

	return shaded_pixmap;
}

#endif /* !USE_GDK_PIXBUF */

#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)

static GdkPixmap *
shade_pixmap (GtkXText * xtext, Pixmap p, int x, int y, int w, int h)
{
#ifdef USE_MMX

	if (have_mmx () && xtext->depth != 8)
		return shade_pixmap_mmx (xtext, p, x, y, w, h);

#ifdef USE_GDK_PIXBUF
	return shade_pixmap_gdk (xtext, p, x, y, w, h);
#else
	return NULL;
#endif

#else

	return shade_pixmap_gdk (xtext, p, x, y, w, h);

#endif /* !USE_MMX */
}

#endif /* !USE_TINT */
#endif /* !USE_XLIB */

/* free transparency xtext->pixmap */

static void
gtk_xtext_free_trans (GtkXText * xtext)
{
	if (xtext->pixmap)
	{
		g_object_unref (xtext->pixmap);
		xtext->pixmap = NULL;
	}
}

#ifdef WIN32

static GdkPixmap *
win32_tint (GtkXText *xtext, GdkImage *img, int width, int height)
{
	int x, y;
	GdkPixmap *pix;
	GdkVisual *visual = img->visual;
	gulong pixel;
	int r, g, b;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pixel = gdk_image_get_pixel (img, x, y);

			r = (pixel & visual->red_mask) >> visual->red_shift;
			g = (pixel & visual->green_mask) >> visual->green_shift;
			b = (pixel & visual->blue_mask) >> visual->blue_shift;

			gdk_image_put_pixel (img, x, y,
						((r * xtext->tint_red) >> 8) << visual->red_shift |
						((g * xtext->tint_green) >> 8) << visual->green_shift |
						((b * xtext->tint_blue) >> 8) << visual->blue_shift);
		}
	}

	if (xtext->recycle)
		pix = xtext->pixmap;
	else
		pix = gdk_pixmap_new (GTK_WIDGET (xtext)->window, width, height,
									 xtext->depth);
	gdk_draw_image (pix, xtext->bgc, img, 0, 0, 0, 0, width, height);

	return pix;
}

#endif /* !WIN32 */

/* grab pixmap from root window and set xtext->pixmap */

static void
gtk_xtext_load_trans (GtkXText * xtext)
{
#ifdef WIN32
	GdkImage *img;
	int width, height;
	HDC hdc;
	HWND hwnd;

	/* if not shaded, we paint directly with PaintDesktop() */
	if (!xtext->shaded)
		return;

	hwnd = GDK_WINDOW_HWND (GTK_WIDGET (xtext)->window);
	hdc = GetDC (hwnd);
	PaintDesktop (hdc);
	ReleaseDC (hwnd, hdc);

	gdk_window_get_size (GTK_WIDGET (xtext)->window, &width, &height);
	width += 105;
	img = gdk_image_get (GTK_WIDGET (xtext)->window, 0, 0, width, height);
	xtext->pixmap = win32_tint (xtext, img, width, height);
	gdk_image_destroy (img);

#else

	Pixmap rootpix;
	GtkWidget *widget = GTK_WIDGET (xtext);
	int x, y;

	rootpix = get_pixmap_prop (GDK_WINDOW_XWINDOW (widget->window));
	if (rootpix == None)
	{
		if (xtext->error_function)
			xtext->error_function ("Unable to get root window pixmap!\n\n"
										  "You may need to use Esetroot or Gnome\n"
										  "control-center to set your background.\n");
		xtext->transparent = FALSE;
		return;
	}

	gdk_window_get_origin (widget->window, &x, &y);

#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
	if (xtext->shaded)
	{
		int width, height;
		gdk_drawable_get_size (GTK_WIDGET (xtext)->window, &width, &height);
		xtext->pixmap = shade_pixmap (xtext, rootpix, x, y, width+105, height);
		if (xtext->pixmap == NULL)
		{
			xtext->shaded = 0;
			goto noshade;
		}
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
	} else
	{
noshade:
#endif
		xtext->pixmap = gdk_pixmap_foreign_new (rootpix);
		gdk_gc_set_tile (xtext->bgc, xtext->pixmap);
		gdk_gc_set_ts_origin (xtext->bgc, -x, -y);
#if defined(USE_GDK_PIXBUF) || defined(USE_MMX)
	}
#endif
	gdk_gc_set_fill (xtext->bgc, GDK_TILED);
#endif /* !WIN32 */
}

/* walk through str until this line doesn't fit anymore */

static int
find_next_wrap (GtkXText * xtext, textentry * ent, unsigned char *str,
					 int win_width, int indent)
{
	unsigned char *last_space = str;
	unsigned char *orig_str = str;
	int str_width = indent;
	int col = FALSE;
	int nc = 0;
	int mbl;
	int ret;
	int limit_offset = 0;

	/* single liners */
	if (win_width >= ent->str_width + ent->indent)
		return ent->str_len;

	/* it does happen! */
	if (win_width < 1)
	{
		ret = ent->str_len - (str - ent->str);
		goto done;
	}

	while (1)
	{
		if ((col && isdigit (*str) && nc < 2) ||
			 (col && *str == ',' && isdigit (*(str+1)) && nc < 3))
		{
			nc++;
			if (*str == ',')
				nc = 0;
			limit_offset++;
			str++;
		} else
		{
			col = FALSE;
			switch (*str)
			{
			case ATTR_COLOR:
				col = TRUE;
				nc = 0;
			case ATTR_BEEP:
			case ATTR_RESET:
			case ATTR_REVERSE:
			case ATTR_BOLD:
			case ATTR_UNDERLINE:
				limit_offset++;
				str++;
				break;
			default:
				str_width += backend_get_char_width (xtext, str, &mbl);
				if (str_width > win_width)
				{
					if (xtext->wordwrap)
					{
						if (str - last_space > WORDWRAP_LIMIT + limit_offset)
							ret = str - orig_str; /* fall back to character wrap */
						else
						{
							if (*last_space == ' ')
								last_space++;
							ret = last_space - orig_str;
							if (ret == 0) /* fall back to character wrap */
								ret = str - orig_str;
						}
						goto done;
					}
					ret = str - orig_str;
					goto done;
				}

				/* keep a record of the last space, for wordwrapping */
				if (is_del (*str))
				{
					last_space = str;
					limit_offset = 0;
				}

				/* progress to the next char */
				str += mbl;

			}
		}

		if (str >= ent->str + ent->str_len)
		{
			ret = str - orig_str;
			goto done;
		}
	}

done:

	/* must make progress */
	if (ret < 1)
		ret = 1;

	return ret;
}

/* render a single line, which may wrap to more lines */

static int
gtk_xtext_render_line (GtkXText * xtext, textentry * ent, int line,
							  int lines_max, int subline, int win_width)
{
	unsigned char *str;
	int indent, taken, len, y;

	taken = 0;
	str = ent->str;
	indent = ent->indent;
	ent->new = 0;

#ifdef XCHAT
	/* draw the timestamp */
	if (xtext->buffer->time_stamp && !xtext->skip_stamp)
	{
		char time_str[64];
		int stamp_size = get_stamp_str (ent->stamp, time_str, sizeof (time_str));
		y = (xtext->fontsize * line) + xtext->font->ascent - xtext->pixel_offset;
		gtk_xtext_render_str (xtext, y, ent, time_str, stamp_size,
									 win_width, 2, line);
	}
#endif

	/* draw each line one by one */
	do
	{
		len = find_next_wrap (xtext, ent, str, win_width, indent);

		y = (xtext->fontsize * line) + xtext->font->ascent - xtext->pixel_offset;
		if (!subline)
		{
			if (!gtk_xtext_render_str (xtext, y, ent, str, len, win_width,
												indent, line))
			{
				/* small optimization */
				return ent->lines_taken - subline;
			}
		} else
		{
			xtext->dont_render = TRUE;
			gtk_xtext_render_str (xtext, y, ent, str, len, win_width,
										 indent, line);
			xtext->dont_render = FALSE;
			subline--;
			line--;
			taken--;
		}

		indent = xtext->buffer->indent;
		line++;
		taken++;
		str += len;

		if (line >= lines_max)
			break;

	}
	while (str < ent->str + ent->str_len);

	return taken;
}

void
gtk_xtext_set_palette (GtkXText * xtext, GdkColor palette[])
{
	int i;

	for (i = 19; i >= 0; i--)
	{
#ifdef USE_XFT
		xtext->color[i].color.red = palette[i].red;
		xtext->color[i].color.green = palette[i].green;
		xtext->color[i].color.blue = palette[i].blue;
		xtext->color[i].color.alpha = 0xffff;
		xtext->color[i].pixel = palette[i].pixel;
#endif
		xtext->palette[i] = palette[i].pixel;
	}

	if (GTK_WIDGET_REALIZED (xtext))
	{
		xtext_set_fg (xtext, xtext->fgc, 18);
		xtext_set_bg (xtext, xtext->fgc, 19);
		xtext_set_fg (xtext, xtext->bgc, 19);
	}
	xtext->col_fore = 18;
	xtext->col_back = 19;
}

static void
gtk_xtext_fix_indent (xtext_buffer *buf)
{
	int j;

	/* make indent a multiple of the space width */
	if (buf->indent && buf->xtext->space_width)
	{
		j = 0;
		while (j < buf->indent)
		{
			j += buf->xtext->space_width;
		}
		buf->indent = j;
	}

	dontscroll (buf);	/* force scrolling off */
}

static void
gtk_xtext_recalc_widths (xtext_buffer *buf, int do_str_width)
{
	textentry *ent;

	/* since we have a new font, we have to recalc the text widths */
	ent = buf->text_first;
	while (ent)
	{
		if (do_str_width)
		{
			ent->str_width = gtk_xtext_text_width (buf->xtext, ent->str,
														ent->str_len, NULL);
		}
		if (ent->left_len != -1)
		{
			ent->indent =
				(buf->indent -
				 gtk_xtext_text_width (buf->xtext, ent->str,
										ent->left_len, NULL)) - buf->xtext->space_width;
			if (ent->indent < MARGIN)
				ent->indent = MARGIN;
		}
		ent = ent->next;
	}

	gtk_xtext_calc_lines (buf, FALSE);
}

int
gtk_xtext_set_font (GtkXText *xtext, char *name)
{
	int i;
	unsigned char c;

	if (xtext->font)
		backend_font_close (xtext->font);

	backend_font_open (xtext, name);
	if (xtext->font == NULL)
		return FALSE;

	/* measure the width of every char;  only the ASCII ones for XFT */
	for (i = 0; i < sizeof(xtext->fontwidth)/sizeof(xtext->fontwidth[0]); i++)
	{
		c = i;
		xtext->fontwidth[i] = backend_get_text_width (xtext, &c, 1, TRUE);
	}
	xtext->space_width = xtext->fontwidth[' '];
	xtext->fontsize = xtext->font->ascent + xtext->font->descent;

#ifdef XCHAT
	{
		char time_str[64];
		int stamp_size = get_stamp_str (time(0), time_str, sizeof (time_str));
		xtext->stamp_width =
			gtk_xtext_text_width (xtext, time_str, stamp_size, NULL) + MARGIN;
	}
#endif

	gtk_xtext_fix_indent (xtext->buffer);

	if (GTK_WIDGET_REALIZED (xtext))
		gtk_xtext_recalc_widths (xtext->buffer, TRUE);

	return TRUE;
}

void
gtk_xtext_set_background (GtkXText * xtext, GdkPixmap * pixmap, int trans,
								  int shaded)
{
	GdkGCValues val;

#if !defined(USE_GDK_PIXBUF) && !defined(USE_MMX) && !defined(WIN32)
	shaded = FALSE;
#endif

#if !defined(USE_XLIB) && !defined(WIN32)
	shaded = FALSE;
	trans = FALSE;
#endif

	if (xtext->pixmap)
	{
#if defined(USE_XLIB) || defined(WIN32)
		if (xtext->transparent)
			gtk_xtext_free_trans (xtext);
		else
#endif
			g_object_unref (xtext->pixmap);
		xtext->pixmap = NULL;
	}

	xtext->transparent = trans;

#if defined(USE_XLIB) || defined(WIN32)
	if (trans)
	{
		xtext->shaded = shaded;
		if (GTK_WIDGET_REALIZED (xtext))
			gtk_xtext_load_trans (xtext);
		return;
	}
#endif

	dontscroll (xtext->buffer);
	xtext->pixmap = pixmap;

	if (pixmap != 0)
	{
		g_object_ref (pixmap);
		if (GTK_WIDGET_REALIZED (xtext))
		{
			gdk_gc_set_tile (xtext->bgc, pixmap);
			gdk_gc_set_ts_origin (xtext->bgc, 0, 0);
			gdk_gc_set_fill (xtext->bgc, GDK_TILED);
		}
	} else if (GTK_WIDGET_REALIZED (xtext))
	{
		g_object_unref (xtext->bgc);
		val.subwindow_mode = GDK_INCLUDE_INFERIORS;
		val.graphics_exposures = 0;
		xtext->bgc = gdk_gc_new_with_values (GTK_WIDGET (xtext)->window,
								&val, GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);
		xtext_set_fg (xtext, xtext->bgc, 19);
	}
}

void
gtk_xtext_save (GtkXText * xtext, int fh)
{
	textentry *ent;
	int newlen;
	char *buf;

	ent = xtext->buffer->text_first;
	while (ent)
	{
		buf = gtk_xtext_strip_color (ent->str, ent->str_len, NULL,
											  &newlen, NULL);
		write (fh, buf, newlen);
		write (fh, "\n", 1);
		free (buf);
		ent = ent->next;
	}
}

/* count how many lines 'ent' will take (with wraps) */

static int
gtk_xtext_lines_taken (xtext_buffer *buf, textentry * ent)
{
	unsigned char *str;
	int indent, taken, len;
	int win_width;

	win_width = buf->window_width - MARGIN;

	if (ent->str_width + ent->indent < win_width)
		return 1;

	indent = ent->indent;
	str = ent->str;
	taken = 0;

	do
	{
		len = find_next_wrap (buf->xtext, ent, str, win_width, indent);
		indent = buf->indent;
		taken++;
		str += len;
	}
	while (str < ent->str + ent->str_len);

	return taken;
}

/* Calculate number of actual lines (with wraps), to set adj->lower. *
 * This should only be called when the window resizes.               */

static void
gtk_xtext_calc_lines (xtext_buffer *buf, int fire_signal)
{
	textentry *ent;
	int width;
	int height;
	int lines;

	gdk_drawable_get_size (GTK_WIDGET (buf->xtext)->window, &width, &height);
	width -= MARGIN;

	if (width < 30 || height < buf->xtext->fontsize || width < buf->indent + 30)
		return;

	lines = 0;
	ent = buf->text_first;
	while (ent)
	{
		ent->lines_taken = gtk_xtext_lines_taken (buf, ent);
		lines += ent->lines_taken;
		ent = ent->next;
	}

	buf->pagetop_ent = NULL;
	buf->num_lines = lines;
	gtk_xtext_adjustment_set (buf, fire_signal);
}

/* find the n-th line in the linked list, this includes wrap calculations */

static textentry *
gtk_xtext_nth (GtkXText *xtext, int line, int *subline)
{
	int lines = 0;
	textentry *ent;

	ent = xtext->buffer->text_first;

	/* -- optimization -- try to make a short-cut using the pagetop ent */
	if (xtext->buffer->pagetop_ent)
	{
		if (line == xtext->buffer->pagetop_line)
		{
			*subline = xtext->buffer->pagetop_subline;
			return xtext->buffer->pagetop_ent;
		}
		if (line > xtext->buffer->pagetop_line)
		{
			/* lets start from the pagetop instead of the absolute beginning */
			ent = xtext->buffer->pagetop_ent;
			lines = xtext->buffer->pagetop_line - xtext->buffer->pagetop_subline;
		}
		else if (line > xtext->buffer->pagetop_line - line)
		{
			/* move backwards from pagetop */
			ent = xtext->buffer->pagetop_ent;
			lines = xtext->buffer->pagetop_line - xtext->buffer->pagetop_subline;
			while (1)
			{
				if (lines <= line)
				{
					*subline = line - lines;
					return ent;
				}
				ent = ent->prev;
				if (!ent)
					break;
				lines -= ent->lines_taken;
			}
			return 0;
		}
	}
	/* -- end of optimization -- */

	while (ent)
	{
		lines += ent->lines_taken;
		if (lines > line)
		{
			*subline = ent->lines_taken - (lines - line);
			return ent;
		}
		ent = ent->next;
	}
	return 0;
}

/* render enta (or an inclusive range enta->entb) */

static void
gtk_xtext_render_ents (GtkXText * xtext, textentry * enta, textentry * entb)
{
	textentry *ent, *orig_ent, *tmp_ent;
	int line;
	int lines_max;
	int width;
	int height;
	int subline;
	int drawing = FALSE;

	if (xtext->buffer->indent < MARGIN)
		xtext->buffer->indent = MARGIN;	  /* 2 pixels is our left margin */

	gdk_drawable_get_size (GTK_WIDGET (xtext)->window, &width, &height);
	width -= MARGIN;

	if (width < 32 || height < xtext->fontsize || width < xtext->buffer->indent + 30)
		return;

	lines_max = ((height + xtext->pixel_offset) / xtext->fontsize) + 1;
	line = 0;
	orig_ent = xtext->buffer->pagetop_ent;
	subline = xtext->buffer->pagetop_subline;

	/* check if enta is before the start of this page */
	if (entb)
	{
		tmp_ent = orig_ent;
		while (tmp_ent)
		{
			if (tmp_ent == enta)
				break;
			if (tmp_ent == entb)
			{
				drawing = TRUE;
				break;
			}
			tmp_ent = tmp_ent->next;
		}
	}

	ent = orig_ent;
	while (ent)
	{
		if (entb && ent == enta)
			drawing = TRUE;

		if (drawing || ent == entb || ent == enta)
		{
			gtk_xtext_reset (xtext, FALSE, TRUE);
			line += gtk_xtext_render_line (xtext, ent, line, lines_max,
													 subline, width);
			subline = 0;
		} else
		{
			if (ent == orig_ent)
			{
				line -= subline;
				subline = 0;
			}
			line += ent->lines_taken;
		}

		if (ent == entb)
			break;

		if (line >= lines_max)
			break;

		ent = ent->next;
	}
}

#ifdef SCROLL_HACK

static int
gtk_xtext_draw_new_lines (GtkXText *xtext)
{
	textentry *ent;
	int ret = 0;

	ent = xtext->buffer->text_last;
	while (ent)
	{
		if (!ent->new)
			break;
		ent = ent->prev;
	}

	if (ent && ent->next)
	{
		gtk_xtext_render_ents (xtext, ent->next, xtext->buffer->text_last);
		ret = 1;
	}

	return ret;
}

#endif

/* render a whole page/window, starting from 'startline' */

static void
gtk_xtext_render_page (GtkXText * xtext)
{
	textentry *ent;
	int line;
	int lines_max;
	int width;
	int height;
	int subline;
	int startline = xtext->adj->value;

	if(!GTK_WIDGET_REALIZED(xtext))
	  return;

	if (xtext->buffer->indent < MARGIN)
		xtext->buffer->indent = MARGIN;	  /* 2 pixels is our left margin */

	gdk_drawable_get_size (GTK_WIDGET (xtext)->window, &width, &height);

	if (width < 34 || height < xtext->fontsize || width < xtext->buffer->indent + 32)
		return;

#ifdef SMOOTH_SCROLL
	xtext->pixel_offset = ((float)((float)xtext->adj->value - (float)startline) * xtext->fontsize);
#else
	xtext->pixel_offset = 0;
#endif

	lines_max = ((height + xtext->pixel_offset) / xtext->fontsize) + 1;
	subline = line = 0;
	ent = xtext->buffer->text_first;

	if (startline > 0)
		ent = gtk_xtext_nth (xtext, startline, &subline);

	xtext->buffer->pagetop_ent = ent;
	xtext->buffer->pagetop_subline = subline;
	xtext->buffer->pagetop_line = startline;

#ifdef SCROLL_HACK
{
	int pos, ended = 0, overlap;
	GdkRectangle area;

	pos = xtext->adj->value * xtext->fontsize;
	overlap = xtext->buffer->last_pixel_pos - pos;
	xtext->buffer->last_pixel_pos = pos;

	if (!xtext->pixmap && abs (overlap) < height - (3*xtext->fontsize))
	{
		/* so the obscured regions are exposed */
		gdk_gc_set_exposures (xtext->fgc, TRUE);
		if (overlap < 0)	/* DOWN */
		{
			gdk_draw_drawable (xtext->draw_buf, xtext->fgc, xtext->draw_buf,
									 0, -overlap, 0, 0, width, height + overlap);
			area.y = height + overlap;
			area.height = -overlap;
			ended = gtk_xtext_draw_new_lines (xtext);
		} else
		{
			gdk_draw_drawable (xtext->draw_buf, xtext->fgc, xtext->draw_buf,
									 0, 0, 0, overlap, width, height - overlap);
			area.y = 0;
			area.height = overlap;
		}
		gdk_gc_set_exposures (xtext->fgc, FALSE);

		if (!ended && area.height > 0)
		{
			area.x = 0;
			area.width = width;
			gtk_xtext_paint (GTK_WIDGET (xtext), &area);
		}

		return;
	}
}
#endif

	width -= MARGIN;
	while (ent)
	{
		gtk_xtext_reset (xtext, FALSE, TRUE);
		line += gtk_xtext_render_line (xtext, ent, line, lines_max,
												 subline, width);
		subline = 0;

		if (line >= lines_max)
			break;

		ent = ent->next;
	}

	line = (xtext->fontsize * line) - xtext->pixel_offset;
	/* fill any space below the last line with our background GC */
	xtext_draw_bg (xtext, 0, line, width + MARGIN, height - line);

	/* draw the separator line */
	gtk_xtext_draw_sep (xtext, -1);
}

void
gtk_xtext_refresh (GtkXText * xtext, int do_trans)
{
	if (GTK_WIDGET_REALIZED (GTK_WIDGET (xtext)))
	{
#if defined(USE_XLIB) || defined(WIN32)
		if (xtext->transparent && do_trans)
		{
			gtk_xtext_free_trans (xtext);
			gtk_xtext_load_trans (xtext);
		}
#endif
		gtk_xtext_render_page (xtext);
	}
}

/* remove the topline from the list */

static void
gtk_xtext_remove_top (xtext_buffer *buffer)
{
	textentry *ent;

	ent = buffer->text_first;
	if (!ent)
		return;
	buffer->num_lines -= ent->lines_taken;
	buffer->pagetop_line -= ent->lines_taken;
	buffer->last_pixel_pos -= (ent->lines_taken * buffer->xtext->fontsize);
	buffer->text_first = ent->next;
	buffer->text_first->prev = NULL;

	buffer->old_value -= ent->lines_taken;
	if (buffer->xtext->buffer == buffer)	/* is it the current buffer? */
	{
		buffer->xtext->adj->value -= ent->lines_taken;
		buffer->xtext->select_start_adj -= ent->lines_taken;
	}

	if (ent == buffer->pagetop_ent)
		buffer->pagetop_ent = NULL;

	if (ent == buffer->last_ent_start)
		buffer->last_ent_start = ent->next;

	if (ent == buffer->last_ent_end)
	{
		buffer->last_ent_start = NULL;
		buffer->last_ent_end = NULL;
	}

	free (ent);
}

void
gtk_xtext_clear (xtext_buffer *buf)
{
	textentry *next;

	buf->last_ent_start = NULL;
	buf->last_ent_end = NULL;
	dontscroll (buf);

	while (buf->text_first)
	{
		next = buf->text_first->next;
		free (buf->text_first);
		buf->text_first = next;
	}
	buf->text_last = NULL;

	if (buf->xtext->buffer == buf)
	{
		gtk_xtext_calc_lines (buf, TRUE);
		gtk_xtext_refresh (buf->xtext, 0);
	} else
	{
		gtk_xtext_calc_lines (buf, FALSE);
	}

	if (buf->xtext->auto_indent)
		buf->xtext->buffer->indent = 1;
}

void *
gtk_xtext_search (GtkXText * xtext, const unsigned char *text, void *start)
{
	textentry *ent, *fent;
	unsigned char *str;
	int line;

	gtk_xtext_selection_clear (xtext->buffer);
	xtext->buffer->last_ent_start = NULL;
	xtext->buffer->last_ent_end = NULL;

	/* validate 'start' */
	ent = xtext->buffer->text_first;
	while (ent)
	{
		if (ent == start)
			break;
		ent = ent->next;
	}
	if (!ent)
		start = NULL;

	if (start)
		ent = ((textentry *) start)->next;
	else
		ent = xtext->buffer->text_first;
	while (ent)
	{
		if ((str = nocasestrstr (ent->str, text)))
		{
			ent->mark_start = str - ent->str;
			ent->mark_end = ent->mark_start + strlen (text);
			break;
		}
		ent = ent->next;
	}

	fent = ent;
	ent = xtext->buffer->text_first;
	line = 0;
	while (ent)
	{
		line += ent->lines_taken;
		ent = ent->next;
		if (ent == fent)
			break;
	}
	while (line > xtext->adj->upper - xtext->adj->page_size)
		line--;

	if (fent != 0)
	{
		xtext->adj->value = line;
		xtext->buffer->scrollbar_down = FALSE;
		gtk_adjustment_changed (xtext->adj);
	}
	gtk_xtext_render_page (xtext);

	return fent;
}

static int
gtk_xtext_render_page_timeout (GtkXText * xtext)
{
	GtkAdjustment *adj = xtext->adj;
	gfloat val;

	if (xtext->buffer->scrollbar_down)
	{
		gtk_xtext_adjustment_set (xtext->buffer, FALSE);
		gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
	} else
	{
		val = adj->value;
		gtk_xtext_adjustment_set (xtext->buffer, TRUE);
		gtk_adjustment_set_value (adj, val);
	}

	if (adj->value >= adj->upper - adj->page_size || adj->value < 1)
		gtk_xtext_render_page (xtext);

	xtext->add_io_tag = 0;

	return 0;
}

/* append a textentry to our linked list */

static void
gtk_xtext_append_entry (xtext_buffer *buf, textentry * ent)
{
	unsigned int mb;

	ent->stamp = time (0);
	ent->str_width = gtk_xtext_text_width (buf->xtext, ent->str, ent->str_len, &mb);
	ent->mb = FALSE;
	if (mb)
		ent->mb = TRUE;
	ent->mark_start = -1;
	ent->mark_end = -1;
	ent->next = NULL;

	if (ent->indent < MARGIN)
		ent->indent = MARGIN;	  /* 2 pixels is the left margin */

	/* append to our linked list */
	if (buf->text_last)
		buf->text_last->next = ent;
	else
		buf->text_first = ent;
	ent->prev = buf->text_last;
	buf->text_last = ent;

	ent->lines_taken = gtk_xtext_lines_taken (buf, ent);
	buf->num_lines += ent->lines_taken;

	if (buf->xtext->max_lines > 2 && buf->xtext->max_lines < buf->num_lines)
	{
		gtk_xtext_remove_top (buf);
	}

	if (buf->xtext->buffer == buf)
	{
#ifdef SCROLL_HACK
		/* this could be improved */
		if (buf->num_lines <= buf->xtext->adj->page_size)
			dontscroll (buf);
#endif

		if (buf->scrollbar_down)
			ent->new = 1;

		if (!buf->xtext->add_io_tag)
		{
			buf->xtext->add_io_tag = g_timeout_add (REFRESH_TIMEOUT * 2,
															(GSourceFunc)
															gtk_xtext_render_page_timeout,
															buf->xtext);
		}
	} else if (buf->scrollbar_down)
	{
		buf->old_value = buf->num_lines - buf->xtext->adj->page_size;
/*		buf->pagetop_ent = NULL;*/
	}
}

/* the main two public functions */

void
gtk_xtext_append_indent (xtext_buffer *buf,
								 unsigned char *left_text, int left_len,
								 unsigned char *right_text, int right_len)
{
	textentry *ent;
	unsigned char *str;
	int space;
	int tempindent;
	int left_width;

	if (left_len == -1)
		left_len = strlen (left_text);

	if (right_len == -1)
		right_len = strlen (right_text);

	if (right_len >= sizeof (buf->xtext->scratch_buffer))
		right_len = sizeof (buf->xtext->scratch_buffer) - 1;

	if (right_text[right_len-1] == '\n')
		right_len--;

	ent = malloc (left_len + right_len + 2 + sizeof (textentry));
	str = (unsigned char *) ent + sizeof (textentry);

	memcpy (str, left_text, left_len);
	str[left_len] = ' ';
	memcpy (str + left_len + 1, right_text, right_len);
	str[left_len + 1 + right_len] = 0;

	left_width = gtk_xtext_text_width (buf->xtext, left_text, left_len, NULL);

	ent->left_len = left_len;
	ent->str = str;
	ent->str_len = left_len + 1 + right_len;
	ent->indent = (buf->indent - left_width) - buf->xtext->space_width;

	if (buf->time_stamp)
		space = buf->xtext->stamp_width;
	else
		space = 0;

	/* do we need to auto adjust the separator position? */
	if (buf->xtext->auto_indent && ent->indent < MARGIN + space)
	{
		tempindent = MARGIN + space + buf->xtext->space_width + left_width;

		if (tempindent > buf->indent)
			buf->indent = tempindent;

		if (buf->indent > buf->xtext->max_auto_indent)
			buf->indent = buf->xtext->max_auto_indent;

		gtk_xtext_fix_indent (buf);
		gtk_xtext_recalc_widths (buf, FALSE);

		ent->indent = (buf->indent - left_width) - buf->xtext->space_width;
	}

	gtk_xtext_append_entry (buf, ent);
}

void
gtk_xtext_append (xtext_buffer *buf, unsigned char *text, int len)
{
	textentry *ent;

	if (len == -1)
		len = strlen (text);

	if (text[len-1] == '\n')
		len--;

	if (len >= sizeof (buf->xtext->scratch_buffer))
		len = sizeof (buf->xtext->scratch_buffer) - 1;

	ent = malloc (len + 1 + sizeof (textentry));
	ent->str = (unsigned char *) ent + sizeof (textentry);
	ent->str_len = len;
	if (len)
		memcpy (ent->str, text, len);
	ent->str[len] = 0;
	ent->indent = 0;
	ent->left_len = -1;

	gtk_xtext_append_entry (buf, ent);
}

gboolean
gtk_xtext_is_empty (GtkXText * xtext)
{
	return xtext->buffer->text_first != NULL;
}

void
gtk_xtext_foreach (xtext_buffer *buf, GtkXTextForeach func, void *data)
{
	textentry *ent = buf->text_first;

	while (ent)
	{
		(*func) (buf->xtext, ent->str, data);
		ent = ent->next;
	}
}

void
gtk_xtext_set_error_function (GtkXText *xtext, 	GtkWidget *(*error_function) (char *))
{
	xtext->error_function = error_function;
}

void
gtk_xtext_set_indent (GtkXText *xtext, gboolean indent)
{
	xtext->auto_indent = indent;
}

void
gtk_xtext_set_max_indent (GtkXText *xtext, int max_auto_indent)
{
	xtext->max_auto_indent = max_auto_indent;
}

void
gtk_xtext_set_max_lines (GtkXText *xtext, int max_lines)
{
	xtext->max_lines = max_lines;
}

void
gtk_xtext_set_show_separator (GtkXText *xtext, gboolean show_separator)
{
	xtext->separator = show_separator;
}

void
gtk_xtext_set_thin_separator (GtkXText *xtext, gboolean thin_separator)
{
	xtext->thinline = thin_separator;
}

void
gtk_xtext_set_time_stamp (xtext_buffer *buf, gboolean time_stamp)
{
	buf->time_stamp = time_stamp;
}

void
gtk_xtext_set_tint (GtkXText *xtext, int tint_red, int tint_green, int tint_blue)
{
	xtext->tint_red = tint_red;
	xtext->tint_green = tint_green;
	xtext->tint_blue = tint_blue;
}

void
gtk_xtext_set_urlcheck_function (GtkXText *xtext, int (*urlcheck_function) (GtkWidget *, char *))
{
	xtext->urlcheck_function = urlcheck_function;
}

void
gtk_xtext_set_wordwrap (GtkXText *xtext, gboolean wordwrap)
{
	xtext->wordwrap = wordwrap;
}

void
gtk_xtext_buffer_show (GtkXText *xtext, xtext_buffer *buf, int render)
{
	int w, h;

	buf->xtext = xtext;

	if (xtext->buffer == buf)
		return;

/*printf("text_buffer_show: xtext=%p buffer=%p\n", xtext, buf);*/

	if (!GTK_WIDGET_REALIZED (GTK_WIDGET (xtext)))
		gtk_widget_realize (GTK_WIDGET (xtext));

	gdk_drawable_get_size (GTK_WIDGET (xtext)->window, &w, &h);

	/* after a font change */
	if (buf->needs_recalc)
	{
		buf->needs_recalc = FALSE;
		gtk_xtext_recalc_widths (buf, TRUE);
	}

	/* now change to the new buffer */
	xtext->buffer = buf;
	dontscroll (buf);	/* force scrolling off */
	xtext->adj->value = buf->old_value;
	xtext->adj->upper = buf->num_lines;
	if (xtext->adj->upper == 0)
		xtext->adj->upper = 1;
	/* sanity check */
	else if (xtext->adj->value > xtext->adj->upper - xtext->adj->page_size)
	{
		/*buf->pagetop_ent = NULL;*/
		xtext->adj->value = xtext->adj->upper - xtext->adj->page_size;
		if (xtext->adj->value < 0)
			xtext->adj->value = 0;
	}

	if (render)
	{
		/* did the window change size since this buffer was last shown? */
		if (buf->window_width != w)
		{
			buf->window_width = w;
			gtk_xtext_calc_lines (buf, FALSE);
			if (buf->scrollbar_down)
				gtk_adjustment_set_value (xtext->adj, xtext->adj->upper -
												  xtext->adj->page_size);
		} else if (buf->window_height != h)
		{
			buf->window_height = h;
			buf->pagetop_ent = NULL;
			gtk_xtext_adjustment_set (buf, FALSE);
		}

		gtk_xtext_render_page (xtext);
		gtk_adjustment_changed (xtext->adj);
	} else
	{
		/* avoid redoing the transparency */
		xtext->avoid_trans = TRUE;
	}
}

xtext_buffer *
gtk_xtext_buffer_new (GtkXText *xtext)
{
	xtext_buffer *buf;

	buf = malloc (sizeof (xtext_buffer));
	memset (buf, 0, sizeof (xtext_buffer));
	buf->old_value = -1;
	buf->xtext = xtext;
	buf->scrollbar_down = TRUE;
	buf->indent = xtext->space_width * 2;
	dontscroll (buf);

	return buf;
}

void
gtk_xtext_buffer_free (xtext_buffer *buf)
{
	textentry *ent, *next;

	if (buf->xtext->buffer == buf)
		buf->xtext->buffer = buf->xtext->orig_buffer;

	if (buf->xtext->selection_buffer == buf)
		buf->xtext->selection_buffer = NULL;

	ent = buf->text_first;
	while (ent)
	{
		next = ent->next;
		free (ent);
		ent = next;
	}

	free (buf);
}
