# HexChat Hacking Guidelines

Just some tips if you're going to help with HexChat code (patches etc):

* Use tabs, not spaces, to indent and align code.

* Use a tab size of 4 (most editors will let you choose this).
  Type :set ts=4 in vim/gvim.

* Try to stick to the same consistant coding style (vertically aligned braces, a space after if, while, functions etc.):

	<pre>void
	routine (void)
	{
		if (function (a, b, c))
		{
			x = a + 1;
		}
	}</pre>

* Don't use "//" C++ style comments, some compilers don't like them.

* When opening a file with Unix level functions (open, read/write, close)
  as opposed to the C level functions (fopen, fwrite/fread, fclose), use
  the OFLAGS macro. This makes sure it'll work on Win32 aswell as Unix e.g.:

	<pre>fh = open ("file", OFLAGS | O_RDONLY);</pre>

* Use closesocket() for sockets, and close() for normal files.

* Don't read() from sockets, use recv() instead.

* Please provide unified format diffs (run diff -u).

* Call your patch something more meaningfull than hexchat.diff.

* To make a really nice and clean patch, do something like this:

	* Have two directories, unpacked from the original archive:

		<pre>hexchat-2.9.0/
		hexchat-2.9.0p1/</pre>

	* Then edit/compile the hexchat-2.9.0p1 directory. To create a patch:

		* Windows:

			<pre>rmdir /q /s hexchat-2.9.0p1/win32/build
			rmdir /q /s hexchat-2.9.0p1/win32/build-xp
			diff -ruN --strip-trailing-cr hexchat-2.9.0 hexchat-2.9.0p1 > hexchat-something.diff
			</pre>

		* Unix:

			<pre>diff -ruN hexchat-2.9.0 hexchat-2.9.0p1 > hexchat-something.diff</pre>


