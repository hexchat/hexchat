#ifndef XCHAT_MODES_H
#define XCHAT_MODES_H

int is_channel (server *serv, char *chan);
char get_nick_prefix (server *serv, unsigned int access);
unsigned int nick_access (server *serv, char *nick, int *modechars);
int mode_access (server *serv, char mode, char *prefix);
void inbound_005 (server *serv, char *word[]);
void handle_mode (server *serv, char *word[], char *word_eol[], char *nick, int numeric_324);
void send_channel_modes (session *sess, char *tbuf, char *word[], int start, int end, char sign, char mode, int modes_per_line);

#endif
