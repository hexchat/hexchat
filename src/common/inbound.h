#ifndef XCHAT_INBOUND_H
#define XCHAT_INBOUND_H

void inbound_next_nick (session *sess, char *nick);
void inbound_uback (server *serv);
void inbound_uaway (server *serv);
void inbound_part (server *serv, char *chan, char *user, char *ip, char *reason);
void inbound_upart (server *serv, char *chan, char *ip, char *reason);
void inbound_ukick (server *serv, char *chan, char *kicker, char *reason);
void inbound_kick (server *serv, char *chan, char *user, char *kicker, char *reason);
void inbound_notice (server *serv, char *to, char *nick, char *msg, char *ip, int id);
void inbound_quit (server *serv, char *nick, char *ip, char *reason);
void inbound_topicnew (server *serv, char *nick, char *chan, char *topic);
void inbound_join (server *serv, char *chan, char *user, char *ip);
void inbound_ujoin (server *serv, char *chan, char *nick, char *ip);
void inbound_topictime (server *serv, char *chan, char *nick, time_t stamp);
void inbound_topic (server *serv, char *chan, char *topic_text);
void inbound_user_info_start (session *sess, char *nick);
int inbound_user_info (session *sess, char *chan, char *user, char *host, char *servname, char *nick, char *realname, unsigned int away);
void inbound_foundip (session *sess, char *ip);
int inbound_banlist (session *sess, time_t stamp, char *chan, char *mask, char *banner, int is_exemption);
void inbound_ping_reply (session *sess, char *timestring, char *from);
void inbound_nameslist (server *serv, char *chan, char *names);
int inbound_nameslist_end (server *serv, char *chan);
void inbound_away (server *serv, char *nick, char *msg);
void inbound_login_start (session *sess, char *nick, char *servname);
void inbound_login_end (session *sess, char *text);
void inbound_chanmsg (server *serv, session *sess, char *chan, char *from, char *text, char fromme, int id);
void clear_channel (session *sess);
void set_topic (session *sess, char *topic);
void inbound_privmsg (server *serv, char *from, char *ip, char *text, int id);
void inbound_action (session *sess, char *chan, char *from, char *text, int fromme, int id);
void inbound_newnick (server *serv, char *nick, char *newnick, int quiet);
void do_dns (session *sess, char *nick, char *host);
void inbound_identified (server *serv);

#endif
