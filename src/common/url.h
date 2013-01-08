#ifndef HEXCHAT_URL_H
#define HEXCHAT_URL_H

extern void *url_tree;

#define WORD_URL     1
#define WORD_NICK    2
#define WORD_CHANNEL 3
#define WORD_HOST    4
#define WORD_EMAIL   5
/* anything >0 will be displayed as a link by gtk_xtext_motion_notify() */
#define WORD_DIALOG  -1
#define WORD_PATH    -2

void url_clear (void);
void url_save_tree (const char *fname, const char *mode, gboolean fullpath);
int url_last (int *, int *);
int url_check_word (const char *word);
void url_check_line (char *buf, int len);

#endif
