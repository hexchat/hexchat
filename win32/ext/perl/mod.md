 * Download [Perl](http://www.perl.org/get.html)
 * Extract to `C:\mozilla-build\hexchat`
 * Open `win32\Makefile` with a text editor
 * Replace `INST_TOP = $(INST_DRV)\perl` with `INST_TOP = $(INST_DRV)\mozilla-build\hexchat\perl-5.16\Win32`
 * Comment out `CCTYPE = MSVC60`
 * Uncomment `#CCTYPE = MSVC100`
 * Uncomment `#WIN64 = undef`
 * Open VS x86 command prompt
 * Go to win32
 * `nmake -f Makefile`
 * `nmake -f Makefile install`
 * Copy `Copying` to `C:mozilla-build\hexchat\perl-5.16\Win32`
