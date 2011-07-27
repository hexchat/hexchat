#include <windows.h>

typedef struct
{
	DWORD threadid;
	int pipe_fd[2];
} thread;

thread *thread_new (void);
int thread_start (thread *th, void *(*start_routine)(void *), void *arg);
