#ifndef XCHAT_URL_H
#define XCHAT_URL_H

extern void *url_tree;

void url_clear (void);
void url_save (const char *fname, const char *mode);
void url_autosave (void);
void url_check (char *buf);

#endif
