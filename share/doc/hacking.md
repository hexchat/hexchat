# HexChat Hacking Guidelines

Just some tips if you're going to help with HexChat code (patches etc):

* Use tabs, not spaces, to indent and align code.

* Use a tab size of 4 (most editors will let you choose this).
  Type :set ts=4 in vim/gvim.

* Try to stick to the same consistant coding style (vertically aligned braces, a space after if, while, functions etc.):

```C
void
routine (void)
{
	if (function (a, b, c))
	{
		x = a + 1;
	}
}
```

* Don't use "//" C++ style comments, some compilers don't like them.

* When opening a file with Unix level functions (open, read/write, close)
  as opposed to the C level functions (fopen, fwrite/fread, fclose), use
  the OFLAGS macro. This makes sure it'll work on Win32 as well as Unix e.g.:

	<pre>fh = open ("file", OFLAGS | O_RDONLY);</pre>

* Use closesocket() for sockets, and close() for normal files.

* Don't read() from sockets, use recv() instead.

* Patches are only accepted as a Github Pull request: https://help.github.com/articles/using-pull-requests

