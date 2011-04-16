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
