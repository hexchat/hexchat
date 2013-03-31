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

#if 0	/* native file dialogs */
#include <fcntl.h>
#include "thread.h"

thread *
thread_new (void)
{
	thread *th;

	th = calloc (1, sizeof (*th));
	if (!th)
	{
		return NULL;
	}

	if (_pipe (th->pipe_fd, 4096, _O_BINARY) == -1)
	{
		free (th);
		return NULL;
	}

	return th;
}

int
thread_start (thread *th, void *(*start_routine)(void *), void *arg)
{
	DWORD id;

	CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, (DWORD *)&id));
	th->threadid = id;

	return 1;
}
#endif
