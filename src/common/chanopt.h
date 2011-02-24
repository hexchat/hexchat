int chanopt_command (session *sess, char *tbuf, char *word[], char *word_eol[]);
gboolean chanopt_is_set (unsigned int global, guint8 per_chan_setting);
gboolean chanopt_is_set_a (unsigned int global, guint8 per_chan_setting);
void chanopt_save_all (void);
void chanopt_save (session *sess);
void chanopt_load (session *sess);
