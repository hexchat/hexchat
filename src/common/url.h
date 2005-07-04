#ifndef XCHAT_URL_H
#define XCHAT_URL_H

extern void *url_tree;

#define WORD_URL     1
#define WORD_NICK    2
#define WORD_CHANNEL 3
#define WORD_HOST    4
#define WORD_EMAIL   5
#define WORD_DIALOG  -1

void url_clear (void);
void url_save (const char *fname, const char *mode);
void url_autosave (void);
int url_check_word (char *word, int len);
void url_check_line (char *buf, int len);

#endif
