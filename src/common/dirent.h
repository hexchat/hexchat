#ifndef DIRENT_H
#define DIRENT_H

#include <windows.h>
#include <string.h>
#include <assert.h>

typedef struct dirent
{
   char d_name[MAX_PATH + 1]; /* current dir entry (multi-byte char string) */
   WIN32_FIND_DATAA data;     /* file attributes */
}  dirent;

typedef struct DIR
{
   dirent current;            /* Current directory entry */
   int    cached;             /* Indicates un-processed entry in memory */
   HANDLE search_handle;      /* File search handle */
   char   patt[MAX_PATH + 3]; /* search pattern (3 = pattern + "\\*\0") */
} DIR;

/* Forward declarations */
DIR *opendir (const char *dirname);
struct dirent *readdir (DIR *dirp);
int closedir (DIR *dirp);
void rewinddir(DIR* dirp);

#endif /*DIRENT_H*/
