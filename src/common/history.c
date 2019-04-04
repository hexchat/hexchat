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

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include "history.h"

void
history_add (struct history *his, char *text)
{
	g_free (his->lines[his->realpos]);
	his->lines[his->realpos] = g_strdup (text);
	his->realpos++;
	if (his->realpos == HISTORY_SIZE)
		his->realpos = 0;
	his->pos = his->realpos;
}

void
history_free (struct history *his)
{
	int i;
	for (i = 0; i < HISTORY_SIZE; i++)
	{
		if (his->lines[i])
		{
			g_free (his->lines[i]);
			his->lines[i] = 0;
		}
	}
}

char *
history_down (struct history *his)
{
	int next;

	if (his->pos == his->realpos)	/* allow down only after up */
		return NULL;
	if (his->realpos == 0)
	{
		if (his->pos == HISTORY_SIZE - 1)
		{
			his->pos = 0;
			return "";
		}
	} else
	{
		if (his->pos == his->realpos - 1)
		{
			his->pos++;
			return "";
		}
	}

	next = 0;
	if (his->pos < HISTORY_SIZE - 1)
		next = his->pos + 1;

	if (his->lines[next])
	{
		his->pos = next;
		return his->lines[his->pos];
	}

	return NULL;
}

char *
history_up (struct history *his, char *current_text)
{
	int next;

	if (his->realpos == HISTORY_SIZE - 1)
	{
		if (his->pos == 0)
			return NULL;
	} else
	{
		if (his->pos == his->realpos + 1)
			return NULL;
	}

	next = HISTORY_SIZE - 1;
	if (his->pos != 0)
		next = his->pos - 1;

	if (his->lines[next])
	{
		if
		(
			current_text[0] && strcmp(current_text, his->lines[next]) &&
			(!his->lines[his->pos] || strcmp(current_text, his->lines[his->pos])) &&
			(!his->lines[his->realpos] || strcmp(current_text, his->lines[his->pos]))
		)
		{
			history_add (his, current_text);
		}
		
		his->pos = next;
		return his->lines[his->pos];
	}

	return NULL;
}
