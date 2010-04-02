#include <stdlib.h>

#define USE_PTHREAD

#ifdef WIN32

#include <windows.h>
#define pthread_t DWORD
#define pipe(a) _pipe(a,4096,_O_BINARY)

#else
#ifdef USE_PTHREAD

#include <pthread.h>

#else

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#define pthread_t int

#endif
#endif


typedef struct
{
	pthread_t threadid;
	int pipe_fd[2];
	void *userdata;
} thread;

thread *
thread_new (void)
{
	thread *th;

	th = calloc (1, sizeof (*th));
	if (!th)
		return NULL;

	if (pipe (th->pipe_fd) == -1)
	{
		free (th);
		return NULL;
	}

#ifdef __EMX__ /* os/2 */
	setmode (pipe_fd[0], O_BINARY);
	setmode (pipe_fd[1], O_BINARY);
#endif

	return th;
}

int
thread_start (thread *th, void *(*start_routine)(void *), void *arg)
{
	pthread_t id;

#ifdef WIN32
	CloseHandle (CreateThread (NULL, 0,
										(LPTHREAD_START_ROUTINE)start_routine,
										arg, 0, (DWORD *)&id));
#else
#ifdef USE_PTHREAD
	if (pthread_create (&id, NULL, start_routine, arg) != 0)
		return 0;
#else
	switch (id = fork ())
	{
	case -1:
		return 0;
	case 0:
		/* this is the child */
		setuid (getuid ());
		start_routine (arg);
		_exit (0);
	}
#endif
#endif

	th->threadid = id;

	return 1;
}

/*void
thread_kill (thread *th)
{
#ifdef WIN32
	PostThreadMessage (th->threadid, WM_QUIT, 0, 0);
#else
#ifdef USE_PTHREAD
	pthread_cancel (th->threadid);
	pthread_join (th->threadid, (void *)&th);
#else
	kill (th->threadid, SIGKILL);
	waitpid (th->threadid, NULL, 0);
#endif
#endif
}

void
thread_free (thread *th)
{
	close (th->pipe_fd[0]);
	close (th->pipe_fd[1]);
	free (th);
}*/
