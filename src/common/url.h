#include <glib/gslist.h>

extern GSList *url_list;

void url_clear (void);
void url_save (const char *fname, const char *mode);
void url_autosave (void);
void url_check (char *buf);
