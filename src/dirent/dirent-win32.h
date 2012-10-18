/*
 * dirent.h - dirent API for Microsoft Visual Studio
 *
 * Copyright (C) 2006-2012 Toni Ronkko
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * ``Software''), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL TONI RONKKO BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Version 1.12.1, Oct 1 2012, Toni Ronkko
 * Bug fix: renamed wide-character DIR structure _wDIR to _WDIR (with
 * capital W) in order to maintain compatibility with MingW.
 *
 * Version 1.12, Sep 30 2012, Toni Ronkko
 * Define PATH_MAX and NAME_MAX.
 *
 * Added wide-character variants _wDIR, _wdirent, _wopendir(),
 * _wreaddir(), _wclosedir() and _wrewinddir().  Thanks to Edgar Buerkle
 * and Jan Nijtmans for ideas and code.
 *
 * Do not include windows.h.  This allows dirent.h to be integrated more
 * easily into programs using winsock.  Thanks to Fernando Azaldegui.
 *
 * Version 1.11, Mar 15, 2011, Toni Ronkko
 * Defined FILE_ATTRIBUTE_DEVICE for MSVC 6.0.
 *
 * Version 1.10, Aug 11, 2010, Toni Ronkko
 * Added d_type and d_namlen fields to dirent structure.  The former is
 * especially useful for determining whether directory entry represents a
 * file or a directory.  For more information, see
 * http://www.delorie.com/gnu/docs/glibc/libc_270.html
 *
 * Improved conformance to the standards.  For example, errno is now set
 * properly on failure and assert() is never used.  Thanks to Peter Brockam
 * for suggestions.
 *
 * Fixed a bug in rewinddir(): when using relative directory names, change
 * of working directory no longer causes rewinddir() to fail.
 *
 * Version 1.9, Dec 15, 2009, John Cunningham
 * Added rewinddir member function
 *
 * Version 1.8, Jan 18, 2008, Toni Ronkko
 * Using FindFirstFileA and WIN32_FIND_DATAA to avoid converting string
 * between multi-byte and unicode representations.  This makes the
 * code simpler and also allows the code to be compiled under MingW.  Thanks
 * to Azriel Fasten for the suggestion.
 *
 * Mar 4, 2007, Toni Ronkko
 * Bug fix: due to the strncpy_s() function this file only compiled in
 * Visual Studio 2005.  Using the new string functions only when the
 * compiler version allows.
 *
 * Nov  2, 2006, Toni Ronkko
 * Major update: removed support for Watcom C, MS-DOS and Turbo C to
 * simplify the file, updated the code to compile cleanly on Visual
 * Studio 2005 with both unicode and multi-byte character strings,
 * removed rewinddir() as it had a bug.
 *
 * Aug 20, 2006, Toni Ronkko
 * Removed all remarks about MSVC 1.0, which is antiqued now.  Simplified
 * comments by removing SGML tags.
 *
 * May 14 2002, Toni Ronkko
 * Embedded the function definitions directly to the header so that no
 * source modules need to be included in the Visual Studio project.  Removed
 * all the dependencies to other projects so that this header file can be
 * used independently.
 *
 * May 28 1998, Toni Ronkko
 * First version.
 *****************************************************************************/
#ifndef DIRENT_H
#define DIRENT_H

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#define _X86_
#endif
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wchar.h>
#include <winnls.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* Windows 8 wide-character string functions */
#if (_WIN32_WINNT >= 0x0602)
#   include <stringapiset.h>
#endif

/* Entries missing from MSVC 6.0 */
#if !defined(FILE_ATTRIBUTE_DEVICE)
# define FILE_ATTRIBUTE_DEVICE 0x40
#endif

/* File type and permission flags for stat() */
#if defined(_MSC_VER)  &&  !defined(S_IREAD)
#   define S_IFMT   _S_IFMT                     /* File type mask */
#   define S_IFDIR  _S_IFDIR                    /* Directory */
#   define S_IFCHR  _S_IFCHR                    /* Character device */
#   define S_IFFIFO _S_IFFIFO                   /* Pipe */
#   define S_IFREG  _S_IFREG                    /* Regular file */
#   define S_IREAD  _S_IREAD                    /* Read permission */
#   define S_IWRITE _S_IWRITE                   /* Write permission */
#   define S_IEXEC  _S_IEXEC                    /* Execute permission */
#endif
#define S_IFBLK   0                             /* Block device */
#define S_IFLNK   0                             /* Link */
#define S_IFSOCK  0                             /* Socket */

#if defined(_MSC_VER)
#   define S_IRUSR  S_IREAD                     /* Read user */
#   define S_IWUSR  S_IWRITE                    /* Write user */
#   define S_IXUSR  0                           /* Execute user */
#   define S_IRGRP  0                           /* Read group */
#   define S_IWGRP  0                           /* Write group */
#   define S_IXGRP  0                           /* Execute group */
#   define S_IROTH  0                           /* Read others */
#   define S_IWOTH  0                           /* Write others */
#   define S_IXOTH  0                           /* Execute others */
#endif

/* Indicates that d_type field is available in dirent structure */
#define _DIRENT_HAVE_D_TYPE

/* File type flags for d_type */
#define DT_UNKNOWN  0
#define DT_REG      S_IFREG
#define DT_DIR      S_IFDIR
#define DT_FIFO     S_IFFIFO
#define DT_SOCK     S_IFSOCK
#define DT_CHR      S_IFCHR
#define DT_BLK      S_IFBLK

/* Macros for converting between st_mode and d_type */
#define IFTODT(mode) ((mode) & S_IFMT)
#define DTTOIF(type) (type)

/*
 * File type macros.  Note that block devices, sockets and links cannot be
 * distinguished on Windows and the macros S_ISBLK, S_ISSOCK and S_ISLNK are
 * only defined for compatibility.  These macros should always return false
 * on Windows.
 */
#define	S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFFIFO)
#define	S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#define	S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#define	S_ISLNK(mode)  (((mode) & S_IFMT) == S_IFLNK)
#define	S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#define	S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)
#define	S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)

/* For compatiblity with Unix */
#if !defined(PATH_MAX)
#   define PATH_MAX MAX_PATH
#endif
#if !defined(FILENAME_MAX)
#   define FILENAME_MAX MAX_PATH
#endif
#if !defined(NAME_MAX)
#   define NAME_MAX FILENAME_MAX
#endif

/* Set errno variable */
#if defined(_MSC_VER)
#define DIRENT_SET_ERRNO(x) _set_errno (x)
#else
#define DIRENT_SET_ERRNO(x) (errno = (x))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Wide-character versions */
struct _wdirent {
    long d_ino;                                 /* Always zero */
    unsigned short d_reclen;                    /* Structure size */
    size_t d_namlen;                            /* Length of name without \0 */
    int d_type;                                 /* File type */
    wchar_t d_name[PATH_MAX + 1];               /* File name */
};
typedef struct _wdirent _wdirent;

struct _WDIR {
    struct _wdirent ent;                        /* Current directory entry */
    WIN32_FIND_DATAW find_data;                 /* Private file data */
    int cached;                                 /* True if data is valid */
    HANDLE handle;                              /* Win32 search handle */
    wchar_t *patt;                              /* Initial directory name */
};
typedef struct _WDIR _WDIR;

static _WDIR *_wopendir (const wchar_t *dirname);
static struct _wdirent *_wreaddir (_WDIR *dirp);
static int _wclosedir (_WDIR *dirp);
static void _wrewinddir (_WDIR* dirp);

/* For compatibility with Symbian */
#define wdirent _wdirent
#define WDIR _WDIR
#define wopendir _wopendir
#define wreaddir _wreaddir
#define wclosedir _wclosedir
#define wrewinddir _wrewinddir


/* Multi-byte character versions */
struct dirent {
    long d_ino;                                 /* Always zero */
    unsigned short d_reclen;                    /* Structure size */
    size_t d_namlen;                            /* Length of name without \0 */
    int d_type;                                 /* File type */
    char d_name[PATH_MAX + 1];                  /* File name */
};
typedef struct dirent dirent;

struct DIR {
    struct dirent ent;
    struct _WDIR *wdirp;
};
typedef struct DIR DIR;

static DIR *opendir (const char *dirname);
static struct dirent *readdir (DIR *dirp);
static int closedir (DIR *dirp);
static void rewinddir (DIR* dirp);


/*
 * Open directory stream DIRNAME for read and return a pointer to the
 * internal working area that is used to retrieve individual directory
 * entries.
 */
static _WDIR*
_wopendir(
    const wchar_t *dirname)
{
    _WDIR *dirp = NULL;
    int error = 0;

    /* Allocate new _WDIR structure */
    dirp = (_WDIR*) malloc (sizeof (struct _WDIR));
    if (dirp != NULL) {
        DWORD n;

        /* Reset _WDIR structure */
        dirp->handle = INVALID_HANDLE_VALUE;
        dirp->patt = NULL;

        /* Compute the length of full path plus zero terminator */
        n = GetFullPathNameW (dirname, 0, NULL, NULL);

        /* Allocate room for full path and search patterns */
        dirp->patt = (wchar_t*) malloc (sizeof (wchar_t) * n + 16);
        if (dirp->patt) {

            /*
             * Convert relative directory name to an absolute one.  This
             * allows rewinddir() to function correctly when the current
             * working directory is changed between opendir() and rewinddir().
             */
            n = GetFullPathNameW (dirname, n, dirp->patt, NULL);
            if (n > 0) {
                wchar_t *p;

                /* Append search pattern \* to the directory name */
                p = dirp->patt + n;
                if (dirp->patt < p) {
                    switch (p[-1]) {
                    case '\\':
                    case '/':
                    case ':':
                        /* Directory ends in path separator, e.g. c:\temp\ */
                        /*NOP*/;
                        break;

                    default:
                        /* Directory name doesn't end in path separator */
                        *p++ = '\\';
                    }
                }
                *p++ = '*';
                *p = '\0';

                /* Open directory stream and retrieve the first entry */
                dirp->handle = FindFirstFileW (dirp->patt, &dirp->find_data);
                if (dirp->handle != INVALID_HANDLE_VALUE) {

                    /* Directory entry is now waiting in memory */
                    dirp->cached = 1;

                } else {
                    /* Search pattern is not a directory name? */
                    DIRENT_SET_ERRNO (ENOENT);
                    error = 1;
                }

            } else {
                /* Cannot convert directory name to wide character string */
                DIRENT_SET_ERRNO (ENOENT);
                error = 1;
            }

        } else {
            /* Cannot allocate memory for search pattern */
            error = 1;
        }

    } else {
        /* Cannot allocate _WDIR structure */
        error = 1;
    }

    /* Clean up in case of error */
    if (error  &&  dirp) {
        _wclosedir (dirp);
        dirp = NULL;
    }

    return dirp;
}

/*
 * Read next directory entry.  The directory entry is returned in dirent
 * structure in the d_name field.  Individual directory entries returned by
 * this function include regular files, sub-directories, pseudo-directories
 * "." and ".." as well as volume labels, hidden files and system files.
 */
static struct _wdirent*
_wreaddir(
    _WDIR *dirp)
{
    DWORD attr;
    errno_t error;

    /* Get next directory entry */
    if (dirp->cached != 0) {
        /* A valid directory entry already in memory */
        dirp->cached = 0;
    } else {
        /* Get the next directory entry from stream */
        if (dirp->handle == INVALID_HANDLE_VALUE) {
            return NULL;
        }
        if (FindNextFileW (dirp->handle, &dirp->find_data) == FALSE) {
            /* The very last entry has been processed or an error occured */
            FindClose (dirp->handle);
            dirp->handle = INVALID_HANDLE_VALUE;
            return NULL;
        }
    }

    /* Copy file name as a wide-character string */
    error = wcsncpy_s(
        dirp->ent.d_name,                       /* Destination string */
        PATH_MAX,                               /* Size of dest in words */
        dirp->find_data.cFileName,              /* Source string */
        PATH_MAX + 1);                          /* Max # of chars to copy */
    if (!error) {

        /* Compute the length of name */
        dirp->ent.d_namlen = wcsnlen (dirp->ent.d_name, PATH_MAX);

        /* Determine file type */
        attr = dirp->find_data.dwFileAttributes;
        if ((attr & FILE_ATTRIBUTE_DEVICE) != 0) {
            dirp->ent.d_type = DT_CHR;
        } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            dirp->ent.d_type = DT_DIR;
        } else {
            dirp->ent.d_type = DT_REG;
        }

        /* Reset dummy fields */
        dirp->ent.d_ino = 0;
        dirp->ent.d_reclen = sizeof (dirp->ent);

    } else {

        /* 
         * Cannot copy file name from find_data to ent.  Construct a
         * dummy _wdirent structure to pass error to caller.
         */
        dirp->ent.d_name[0] = '?';
        dirp->ent.d_name[1] = '\0';
        dirp->ent.d_namlen = 1;
        dirp->ent.d_type = DT_UNKNOWN;
        dirp->ent.d_ino = 0;
        dirp->ent.d_reclen = 0;
    }

    return &dirp->ent;
}

/*
 * Close directory stream opened by opendir() function.  This invalidates the
 * DIR structure as well as any directory entry read previously by
 * _wreaddir().
 */
static int
_wclosedir(
    _WDIR *dirp)
{
    int ok;
    if (dirp) {

        /* Release search handle */
        if (dirp->handle != INVALID_HANDLE_VALUE) {
            FindClose (dirp->handle);
            dirp->handle = INVALID_HANDLE_VALUE;
        }

        /* Release search pattern */
        if (dirp->patt) {
            free (dirp->patt);
            dirp->patt = NULL;
        }

        /* Release directory structure */
        free (dirp);
        ok = /*success*/0;

    } else {
        /* Invalid directory stream */
        DIRENT_SET_ERRNO (EBADF);
        ok = /*failure*/-1;
    }
    return ok;
}

/*
 * Rewind directory stream such that _wreaddir() returns the very first
 * file name again.
 */
static void
_wrewinddir(
    _WDIR* dirp)
{
    if (dirp != NULL) {
        /* release search handle */
        if (dirp->handle != INVALID_HANDLE_VALUE) {
            FindClose (dirp->handle);
        }

        /* Open new search handle and retrieve the first directory entry */
        dirp->handle = FindFirstFileW (dirp->patt, &dirp->find_data);
        if (dirp->handle != INVALID_HANDLE_VALUE) {
            /* a directory entry is now waiting in memory */
            dirp->cached = 1;
        } else {
            /* Failed to re-open directory: no directory entry in memory */
            dirp->cached = 0;
        }
    }
}

/* 
 * Open directory stream using plain old C-string.
 */
static DIR*
opendir(
    const char *dirname) 
{
    struct DIR *dirp;
    errno_t error = 0;

    /* Must have directory name */
    if (dirname == NULL) {
        DIRENT_SET_ERRNO (ENOENT);
        return NULL;
    }

    /* Allocate memory for multi-byte string directory structures */
    dirp = (DIR*) malloc (sizeof (struct DIR));
    if (dirp) {
        wchar_t wname[PATH_MAX + 1];
        size_t n;

        /* 
         * Convert directory name to wide-character string.
         *
         * Be ware of the return schemantics of MultiByteToWideChar() --
         * the function basically returns the number of characters written to
         * output buffer or zero if the conversion fails.  However, the
         * function does not necessarily zero-terminate the output
         * buffer and may return 0xFFFD if the string contains invalid
         * characters!
         */
        n = MultiByteToWideChar(
                CP_ACP,                         /* Input code page */
                MB_PRECOMPOSED,                 /* Conversion flags */
                dirname,                        /* Input string */
                -1,                             /* Length of input string */
                wname,                          /* Output buffer */
                PATH_MAX);                      /* Size of output buffer */
        if (n > 0  &&  n < PATH_MAX) {

            /* Zero-terminate output buffer */
            wname[n] = '\0';

            /* Open directory stream with wide-character string file name */
            dirp->wdirp = _wopendir (wname);
            if (dirp->wdirp) {

                /* Initialize directory structure */
                dirp->ent.d_name[0] = '\0';
                dirp->ent.d_namlen = 0;
                dirp->ent.d_type = 0;
                dirp->ent.d_ino = 0;
                dirp->ent.d_reclen = 0;


            } else {
                /* Failed to open directory stream */
                error = 1;
            }

        } else {
            /* 
             * Cannot convert file name to wide-character string.  This
             * occurs if the string contains invalid multi-byte sequences or
             * the output buffer is too small to contain the resulting
             * string.
             */
            error = 1;
        }

    } else {
        /* Cannot allocate DIR structure */
        error = 1;
    }

    /* Clean up in case of error */
    if (error  &&  dirp) {
        free (dirp);
        dirp = NULL;
    }

    return dirp;
}

/*
 * Read next directory entry.
 *
 * When working with console, please note that file names returned by
 * readdir() are represented in the default ANSI code page while the
 * console typically runs on another code page.  Thus, non-ASCII characters
 * will not usually display correctly.  The problem can be fixed in two ways:
 * (1) change the character set of console to 1252 using chcp utility and use
 * Lucida Console font, or (2) always use _cprintf function when writing to
 * console.  The _cprinf() will re-encode ANSI strings to the console code
 * page so non-ASCII characters will display correcly.
 */
static struct dirent*
readdir(
    DIR *dirp) 
{
    struct dirent *p;
    struct _wdirent *wp;

    /* Read next directory entry using wide-character string functions */
    wp = _wreaddir (dirp->wdirp);
    if (wp) {
        size_t n;

        /* 
         * Convert file name to multi-byte string.
         *
         * Be ware of the return schemantics of WideCharToMultiByte() --
         * the function basically returns the number of bytes
         * written to output buffer or zero if the conversion fails.
         * However, the function does not necessarily zero-terminate the
         * buffer and it may even return 0xFFFD the string contains
         * invalid characters!
         */
        n = WideCharToMultiByte(
                CP_ACP,                         /* Output code page */
                0,                              /* Conversion flags */
                wp->d_name,                     /* Input string */
                wp->d_namlen,                   /* Length of input string */
                dirp->ent.d_name,               /* Output buffer */
                PATH_MAX,                       /* Size of output buffer */
                NULL,                           /* Replacement character */
                NULL);                          /* If chars were replaced */
        if (n > 0  &&  n < PATH_MAX) {

            /* Zero-terminate buffer */
            dirp->ent.d_name[n] = '\0';

            /* Initialize directory entry for return */
            p = &dirp->ent;

            /* Compute length */
            p->d_namlen = strnlen (dirp->ent.d_name, PATH_MAX);

            /* Copy file attributes */
            p->d_type = wp->d_type;

            /* Reset dummy fields */
            p->d_ino = 0;
            p->d_reclen = sizeof (dirp->ent);


        } else {

            /* 
             * Cannot convert file name to multi-byte string so construct
             * an errornous directory entry and return that.  Note that
             * we cannot return NULL as that would stop the processing
             * of directory entries completely.
             */
            p = &dirp->ent;
            p->d_name[0] = '?';
            p->d_name[1] = '\0';
            p->d_namlen = 1;
            p->d_type = DT_UNKNOWN;
            p->d_ino = 0;
            p->d_reclen = 0;

        }

    } else {
    
        /* End of directory stream */
        p = NULL;

    }

    return p;
}

/*
 * Close directory stream.
 */
static int
closedir(
    DIR *dirp) 
{
    int ok;
    if (dirp) {

        /* Close wide-character directory stream */
        ok = _wclosedir (dirp->wdirp);
        dirp->wdirp = NULL;

        /* Release multi-byte character version */
        free (dirp);

    } else {
        /* Invalid directory stream */
        DIRENT_SET_ERRNO (EBADF);
        ok = /*failure*/-1;
    }
    return ok;
}

/*
 * Rewind directory stream to beginning.
 */
static void
rewinddir(
    DIR* dirp) 
{
    /* Rewind wide-character string directory stream */
    _wrewinddir (dirp->wdirp);
}


#ifdef __cplusplus
}
#endif
#endif /*DIRENT_H*/

