#include  <io.h>

int
portable_mode ()
{
	if ((_access( "portable-mode", 0 )) != -1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
