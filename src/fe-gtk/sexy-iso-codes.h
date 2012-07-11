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

#ifndef GTKSPELL_ISO_CODES_H
#define GTKSPELL_ISO_CODES_H

#include <glib.h>

G_BEGIN_DECLS

char *  gtkspell_iso_codes_lookup_name_for_code (const char *code);

G_END_DECLS

#endif
