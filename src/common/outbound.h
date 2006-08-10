#ifndef XCHAT_OUTBOUND_H
#define XCHAT_OUTBOUND_H

extern const struct commands xc_cmds[];
extern GSList *menu_list;

int auto_insert (char *dest, int destlen, unsigned char *src, char *word[], char *word_eol[],
				 char *a, char *c, char *d, char *h, char *n, char *s);
int handle_command (session *sess, char *cmd, int check_spch);
void process_data_init (char *buf, char *cmd, char *word[], char *word_eol[], gboolean handle_quotes, gboolean allow_escape_quotes);
void handle_multiline (session *sess, char *cmd, int history, int nocommand);
void check_special_chars (char *cmd, int do_ascii);
void notc_msg (session *sess);
void server_sendpart (server * serv, char *channel, char *reason);
void server_sendquit (session * sess);
int menu_streq (const char *s1, const char *s2, int def);
void open_query (server *serv, char *nick, gboolean focus_existing);

#endif
