#include "textenums.h"

#define EMIT_SIGNAL(i, sess, a, b, c, d, e) text_emit(i, sess, a, b, c, d)
#define WORD_URL     1
#define WORD_NICK    2
#define WORD_CHANNEL 3
#define WORD_HOST    4
#define WORD_EMAIL   5
#define WORD_DIALOG  -1

struct text_event
{
	char *name;
	char **help;
	int num_args;
	char *sound;
	char *def;
};

int text_word_check (char *word);
void PrintText (session *sess, unsigned char *text);
void PrintTextf (session *sess, char *format, ...);
void log_close (session *sess);
void log_open (session *sess);
void load_text_events (void);
void pevent_save (char *fn);
int pevt_build_string (const char *input, char **output, int *max_arg);
int pevent_load (char *filename);
void pevent_make_pntevts (void);
void text_emit (int index, session *sess, char *a, char *b, char *c, char *d);
int text_emit_by_name (char *name, session *sess, char *a, char *b, char *c, char *d);
char *text_validate (char **text, int *len);
