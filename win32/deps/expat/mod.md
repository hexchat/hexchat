 * Download [Expat 2.1.0](http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download)
 * Extract to `C:\mozilla-build\hexchat`
 * Open `expat.dsw` with VS and convert
 * For each project, change _Runtime Library_ from _Multi-threaded (/MT)_ to _Multi-threaded DLL (/MD)_ under _Configuration Properties_ `->` _C/C++_ `->` _Code Generation_
 * Build in VS
 * Release with `release-x86.bat`
 * Extract package to `C:\mozilla-build\hexchat\build\Win32`
