#ifndef XCHAT_HISTORY_H
#define XCHAT_HISTORY_H

#define HISTORY_SIZE 100

struct history
{
	char *lines[HISTORY_SIZE];
	int pos;
	int realpos;
};

void history_add (struct history *his, char *text);
void history_free (struct history *his);
char *history_up (struct history *his, char *current_text);
char *history_down (struct history *his);

#endif
