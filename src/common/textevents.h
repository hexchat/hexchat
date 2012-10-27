/* this file is auto generated, edit textevents.in instead! */

const struct text_event te[] = {

{"Add Notify", pevt_generic_nick_help, 1, 
N_("%C18*%O$t%C18$1%O added to notify list.")},

{"Ban List", pevt_banlist_help, 4, 
N_("%C22*%O$t%C22$1%O Banlist:%C18 $2%O for%C24 $4%O by%C26 $3%O")},

{"Banned", pevt_generic_channel_help, 1, 
N_("%C22*%O$tCannot join%C22 $1 %O(%C20You are banned%O).")},

{"Beep", pevt_generic_none_help, 128, 
""},

{"Change Nick", pevt_changenick_help, 2, 
N_("%C24*%O$t%C28$1%O is now known as%C18 $2%O")},

{"Channel Action", pevt_chanaction_help, 132, 
"%C18*$t%B$1%O $2"},

{"Channel Action Hilight", pevt_chanaction_help, 132, 
"%C19*$t%B$1%B $2%O"},

{"Channel Ban", pevt_chanban_help, 2, 
N_("%C22*%O$t%C26$1%O sets ban on %C18$2%O")},

{"Channel Creation", pevt_chandate_help, 1, 
N_("%C22*%O$tChannel %C22$1%O created")},

{"Channel DeHalfOp", pevt_chandehop_help, 2, 
N_("%C22*%O$t%C26$1%O removes channel half-operator status from %C18$2%O")},

{"Channel DeOp", pevt_chandeop_help, 2, 
N_("%C22*%O$t%C26$1%O removes channel operator status from %C18$2%O")},

{"Channel DeVoice", pevt_chandevoice_help, 2, 
N_("%C22*%O$t%C26$1%O removes voice from %C18$2%O")},

{"Channel Exempt", pevt_chanexempt_help, 2, 
N_("%C22*%O$t%C26 $1%C sets exempt on %C18$2%O")},

{"Channel Half-Operator", pevt_chanhop_help, 2, 
N_("%C22*%O$t%C26$1%O gives channel half-operator status to %C18$2%O")},

{"Channel INVITE", pevt_chaninvite_help, 2, 
N_("%C22*%O$t%C26 $1%C sets invite on %C18$2%O")},

{"Channel List", pevt_generic_none_help, 0, 
N_("%UChannel          Users   Topic")},

{"Channel Message", pevt_chanmsg_help, 132, 
"%C18%H<%H$4$1%H>%H%O$t$2"},

{"Channel Mode Generic", pevt_chanmodegen_help, 4, 
N_("%C22*%O$t%C26$1%O sets mode %C24$2$3%O on %C22$4%O")},

{"Channel Modes", pevt_chanmodes_help, 2, 
N_("%C22*%O$tChannel %C22$1%O modes: %C24$2")},

{"Channel Msg Hilight", pevt_chanmsg_help, 132, 
"%C19%H<%H$4%B$1%B%H>%H$t$2%O"},

{"Channel Notice", pevt_channotice_help, 131, 
"-%C18$1%C/%C22$2%C-$t$3%O"},

{"Channel Operator", pevt_chanop_help, 2, 
N_("%C22*%O$t%C26$1%O gives channel operator status to %C18$2%O")},

{"Channel Remove Exempt", pevt_chanrmexempt_help, 2, 
N_("%C22*%O$t%C26$1%O removes exempt on %C18$2%O")},

{"Channel Remove Invite", pevt_chanrminvite_help, 2, 
N_("%C22*%O$t%C26 $1%O removes invite on %C18$2%O")},

{"Channel Remove Keyword", pevt_chanrmkey_help, 1, 
N_("%C22*%O$t%C26$1%O removes channel keyword")},

{"Channel Remove Limit", pevt_chanrmlimit_help, 1, 
N_("%C22*%O$t%C26$1%O removes user limit")},

{"Channel Set Key", pevt_chansetkey_help, 2, 
N_("%C22*%O$t$1 sets channel keyword to %C24$2%O")},

{"Channel Set Limit", pevt_chansetlimit_help, 2, 
N_("%C22*%O$t%C26$1%O sets channel limit to %C24$2%O")},

{"Channel UnBan", pevt_chanunban_help, 2, 
N_("%C22*%O$t%C26$1%O removes ban on %C24$2%O")},

{"Channel Voice", pevt_chanvoice_help, 2, 
N_("%C22*%O$t%C26$1%O gives voice to %C18$2%O")},

{"Connected", pevt_generic_none_help, 0, 
N_("%C23*%O$tConnected. Now logging in.")},

{"Connecting", pevt_connect_help, 3, 
N_("%C23*%O$tConnecting to %C29$1%C (%C23$2:$3%O)")},

{"Connection Failed", pevt_connfail_help, 1, 
N_("%C20*%O$tConnection failed (%C20$1%O)")},

{"CTCP Generic", pevt_ctcpgen_help, 2, 
N_("%C24*%O$tReceived a CTCP %C24$1%C from %C18$2%O")},

{"CTCP Generic to Channel", pevt_ctcpgenc_help, 3, 
N_("%C24*%C$tReceived a CTCP %C24$1%C from %C18$2%C (to %C22$3%C)%O")},

{"CTCP Send", pevt_ctcpsend_help, 2, 
N_(">%C18$1%C<$tCTCP %C24$2%O")},

{"CTCP Sound", pevt_ctcpsnd_help, 2, 
N_("%C24*%O$tReceived a CTCP Sound %C24$1%C from %C18$2%O")},

{"CTCP Sound to Channel", pevt_ctcpsnd_help, 3, 
N_("%C24*%O$tReceived a CTCP Sound %C24$1%C from %C18$2%C (to %C22$3%O)")},

{"DCC CHAT Abort", pevt_dccchatabort_help, 1, 
N_("%C23*%O$tDCC CHAT to %C18$1%O aborted.")},

{"DCC CHAT Connect", pevt_dccchatcon_help, 2, 
N_("%C24*%O$tDCC CHAT connection established to %C18$1%C %C30[%C24$2%C30]%O")},

{"DCC CHAT Failed", pevt_dccchaterr_help, 4, 
N_("%C20*%O$tDCC CHAT to %C18$1%O lost (%C20$4%O)")},

{"DCC CHAT Offer", pevt_generic_nick_help, 1, 
N_("%C24*%O$tReceived a DCC CHAT offer from %C18$1%O")},

{"DCC CHAT Offering", pevt_generic_nick_help, 1, 
N_("%C24*%O$tOffering DCC CHAT to %C18$1%O")},

{"DCC CHAT Reoffer", pevt_generic_nick_help, 1, 
N_("%C24*%O$tAlready offering CHAT to %C18$1%O")},

{"DCC Conection Failed", pevt_dccconfail_help, 3, 
N_("%C20*%O$tDCC $1 connect attempt to %C18$2%O failed (%C20$3%O)")},

{"DCC Generic Offer", pevt_dccgenericoffer_help, 2, 
N_("%C23*%O$tReceived '%C23$1%C' from %C18$2%O")},

{"DCC Header", pevt_generic_none_help, 0, 
N_("%C16,17 Type  To/From      Status  Size    Pos     File              ")},

{"DCC Malformed", pevt_malformed_help, 2, 
N_("%C20*%O$tReceived a malformed DCC request from %C18$1%O.%010%C23*%O$tContents of packet: %C23$2%O")},

{"DCC Offer", pevt_dccoffer_help, 3, 
N_("%C24*%O$tOffering '%C24$1%O' to %C18$2%O")},

{"DCC Offer Not Valid", pevt_generic_none_help, 0, 
N_("%C23*%O$tNo such DCC offer.")},

{"DCC RECV Abort", pevt_dccfileabort_help, 2, 
N_("%C23*%O$tDCC RECV '%C23$2%O' to %C18$1%O aborted.")},

{"DCC RECV Complete", pevt_dccrecvcomp_help, 4, 
N_("%C24*%O$tDCC RECV '%C23$1%O' from %C18$3%O complete %C30[%C24$4%O cps%C30]%O")},

{"DCC RECV Connect", pevt_dcccon_help, 3, 
N_("%C24*%O$tDCC RECV connection established to %C18$1 %C30[%O%C24$2%C30]%O")},

{"DCC RECV Failed", pevt_dccrecverr_help, 4, 
N_("%C20*%O$tDCC RECV '%C23$1%O' from %C18$3%O failed (%C20$4%O)")},

{"DCC RECV File Open Error", pevt_generic_file_help, 2, 
N_("%C20*%O$tDCC RECV: Cannot open '%C23$1%C' for writing (%C20$2%O)")},

{"DCC Rename", pevt_dccrename_help, 2, 
N_("%C23*%O$tThe file '%C24$1%C' already exists, saving it as '%C23$2%O' instead.")},

{"DCC RESUME Request", pevt_dccresumeoffer_help, 3, 
N_("%C24*%O$t%C18$1%C has requested to resume '%C23$2%C' from %C24$3%O.")},

{"DCC SEND Abort", pevt_dccfileabort_help, 2, 
N_("%C23*%O$tDCC SEND '%C23$2%C' to %C18$1%O aborted.")},

{"DCC SEND Complete", pevt_dccsendcomp_help, 3, 
N_("%C24*%O$tDCC SEND '%C23$1%C' to %C18$2%C complete %C30[%C24$3%C cps%C30]%O")},

{"DCC SEND Connect", pevt_dcccon_help, 3, 
N_("%C24*%O$tDCC SEND connection established to %C18$1 %C30[%O%C24$2%C30]%O")},

{"DCC SEND Failed", pevt_dccsendfail_help, 3, 
N_("%C20*%O$tDCC SEND '%C23$1%C' to %C18$2%C failed (%C20$3%O)")},

{"DCC SEND Offer", pevt_dccsendoffer_help, 4, 
N_("%C24*%O$t%C18$1%C has offered '%C23$2%C' (%C24$3%O bytes)")},

{"DCC Stall", pevt_dccstall_help, 3, 
N_("%C20*%O$tDCC $1 '%C23$2%C' to %C18$3%O stalled, aborting.")},

{"DCC Timeout", pevt_dccstall_help, 3, 
N_("%C20*%O$tDCC $1 '%C23$2%C' to %C18$3%O timed out, aborting.")},

{"Delete Notify", pevt_generic_nick_help, 1, 
N_("%C24*%O$t%C18$1%O deleted from notify list.")},

{"Disconnected", pevt_discon_help, 1, 
N_("%C20*%O$tDisconnected (%C20$1%O)")},

{"Found IP", pevt_foundip_help, 1, 
N_("%C24*%O$tFound your IP: %C30[%C24$1%C30]%O")},

{"Generic Message", pevt_genmsg_help, 130, 
"$1$t$2"},

{"Ignore Add", pevt_ignoreaddremove_help, 1, 
N_("%O%C18$1%O added to ignore list.")},

{"Ignore Changed", pevt_ignoreaddremove_help, 1, 
N_("%OIgnore on %C18$1%O changed.")},

{"Ignore Footer", pevt_generic_none_help, 0, 
N_("%C16,17                                                              ")},

{"Ignore Header", pevt_generic_none_help, 0, 
N_("%C16,17 Hostmask                  PRIV NOTI CHAN CTCP DCC  INVI UNIG ")},

{"Ignore Remove", pevt_ignoreaddremove_help, 1, 
N_("%O%C18$1%O removed from ignore list.")},

{"Ignorelist Empty", pevt_generic_none_help, 0, 
N_("%OIgnore list is empty.")},

{"Invite", pevt_generic_channel_help, 1, 
N_("%C20*%O$tCannot join %C22$1%C (%C20Channel is invite only%O)")},

{"Invited", pevt_invited_help, 3, 
N_("%C24*%O$tYou have been invited to %C22$1%O by %C18$2%O (%C29$3%O)")},

{"Join", pevt_join_help, 2, 
N_("%C23*$t$1 ($3) has joined")},

{"Keyword", pevt_generic_channel_help, 1, 
N_("%C20*%O$tCannot join %C22$1%C (%C20Requires keyword%O)")},

{"Kick", pevt_kick_help, 4, 
N_("%C22*%O$t%C26$1%C has kicked %C18$2%C from %C22$3%C (%C24$4%O)")},

{"Killed", pevt_kill_help, 2, 
N_("%C19*%O$t%C19You have been killed by %C26$1%C (%C20$2%O)")},

{"Message Send", pevt_ctcpsend_help, 130, 
"%O>%C18$1%C<%O$t$2"},

{"Motd", pevt_servertext_help, 129, 
"%C29*%O$t%C29$1%O"},

{"MOTD Skipped", pevt_generic_none_help, 0, 
N_("%C29*%O$t%C29MOTD Skipped%O")},

{"Nick Clash", pevt_nickclash_help, 2, 
N_("%C23*%O$t%C28$1%C already in use. Retrying with %C18$2%O...")},

{"Nick Failed", pevt_generic_none_help, 0, 
N_("%C20*%O$tNickname already in use. Use /NICK to try another.")},

{"No DCC", pevt_generic_none_help, 0, 
N_("%C20*%O$tNo such DCC.")},

{"No Running Process", pevt_generic_none_help, 0, 
N_("%C23*%O$tNo process is currently running")},

{"Notice", pevt_notice_help, 130, 
"%O-%C18$1%O-$t$2"},

{"Notice Send", pevt_ctcpsend_help, 130, 
"%O->%C18$1%O<-$t$2"},

{"Notify Empty", pevt_generic_none_help, 0, 
N_("$tNotify list is empty.")},

{"Notify Header", pevt_generic_none_help, 0, 
N_("%C16,17 Notify List                           ")},

{"Notify Number", pevt_notifynumber_help, 1, 
N_("%C23*%O$t%C23$1%O users in notify list.")},

{"Notify Offline", pevt_generic_nick_help, 3, 
N_("%C23*%O$tNotify: %C18$1%C is offline (%C29$3%O)")},

{"Notify Online", pevt_generic_nick_help, 3, 
N_("%C23*%O$tNotify: %C18$1%C is online (%C29$3%O)")},

{"Open Dialog", pevt_generic_none_help, 128, 
""},

{"Part", pevt_part_help, 1, 
N_("%C24*$t$1 has left")},

{"Part with Reason", pevt_partreason_help, 3, 
N_("%C24*%O$t%C18$1%C ($2) has left (%C24$4%O)")},

{"Ping Reply", pevt_pingrep_help, 2, 
N_("%C24*%O$tPing reply from%C18 $1%C: %C24$2%O second(s)")},

{"Ping Timeout", pevt_pingtimeout_help, 1, 
N_("%C20*%O$tNo ping reply for %C24$1%O seconds, disconnecting.")},

{"Private Action", pevt_privmsg_help, 131, 
"%C18**$t$3$1%O $2 %C18**"},

{"Private Action to Dialog", pevt_privmsg_help, 131, 
"%C18*$t$3$1%O $2"},

{"Private Message", pevt_privmsg_help, 131, 
"%C18*%C18$3$1*%O$t$2"},

{"Private Message to Dialog", pevt_privmsg_help, 131, 
"%C18%H<%H$3$1%H>%H%O$t$2"},

{"Process Already Running", pevt_generic_none_help, 0, 
N_("%C24*%O$tA process is already running")},

{"Quit", pevt_quit_help, 3, 
N_("%C24*$t$1 has quit ($2)")},

{"Raw Modes", pevt_rawmodes_help, 2, 
N_("%C24*%O$t%C26$1%C sets modes %C30[%C24$2%C30]%O")},

{"Receive Wallops", pevt_privmsg_help, 2, 
N_("%O-%C29$1/Wallops%O-$t$2")},

{"Resolving User", pevt_resolvinguser_help, 2, 
N_("%C24*%O$tLooking up IP number for %C18$1%O...")},

{"Server Connected", pevt_generic_none_help, 0, 
N_("%C29*%O$tConnected.")},

{"Server Error", pevt_servererror_help, 129, 
"%C29*%O$t%C20$1%O"},

{"Server Lookup", pevt_serverlookup_help, 1, 
N_("%C29*%O$tLooking up %C29$1%O")},

{"Server Notice", pevt_servertext_help, 130, 
"%C29*%O$t$1"},

{"Server Text", pevt_servertext_help, 131, 
"%C29*%O$t$1"},

{"SSL Message", pevt_sslmessage_help, 130, 
"%C29*%O$t$1"},

{"Stop Connection", pevt_sconnect_help, 1, 
N_("%C23*%O$tStopped previous connection attempt (%C24$1%O)")},

{"Topic", pevt_topic_help, 2, 
N_("%C22*%O$tTopic for %C22$1%C is: $2%O")},

{"Topic Change", pevt_newtopic_help, 3, 
N_("%C22*%O$t%C26$1%C has changed the topic to: $2%O")},

{"Topic Creation", pevt_topicdate_help, 3, 
N_("%C22*%O$tTopic for %C22$1%C set by %C26$2%C (%C24$3%O)")},

{"Unknown Host", pevt_generic_none_help, 0, 
N_("%C20*%O$tUnknown host. Maybe you misspelled it?")},

{"User Limit", pevt_generic_channel_help, 1, 
N_("%C20*%O$tCannot join %C22$1%C (%C20User limit reached%O)")},

{"Users On Channel", pevt_usersonchan_help, 2, 
N_("%C22*%O$tUsers on %C22$1%C: %C24$2%O")},

{"WhoIs Authenticated", pevt_whoisauth_help, 3, 
N_("%C23*%O$t%C28[%C18$1%C28]%O $2 %C18$3%O")},

{"WhoIs Away Line", pevt_whois5_help, 2, 
N_("%C23*%O$t%C28[%C18$1%C28] %Cis away %C30(%C23$2%O%C30)%O")},

{"WhoIs Channel/Oper Line", pevt_whois2_help, 2, 
N_("%C23*%O$t%C28[%C18$1%C28]%O $2")},

{"WhoIs End", pevt_whois6_help, 1, 
N_("%C23*%O$t%C28[%C18$1%C28] %OEnd of WHOIS list.")},

{"WhoIs Identified", pevt_whoisid_help, 2, 
N_("%C23*%O$t%C28[%C18$1%C28]%O $2")},

{"WhoIs Idle Line", pevt_whois4_help, 2, 
N_("%C23*%O$t%C28[%C18$1%C28]%O idle %C23$2%O")},

{"WhoIs Idle Line with Signon", pevt_whois4t_help, 3, 
N_("%C23*%O$t%C28[%C18$1%C28]%O idle %C23$2%O, signon: %C23$3%O")},

{"WhoIs Name Line", pevt_whois1_help, 4, 
N_("%C23*%O$t%C28[%C18$1%C28] %C30(%C24$2@$3%C30)%O: %C18$4%O")},

{"WhoIs Real Host", pevt_whoisrealhost_help, 4, 
N_("%C23*%O$t%C28[%C18$1%C28]%O Real Host: %C23$2%O, Real IP: %C30[%C23$3%C30]%O")},

{"WhoIs Server Line", pevt_whois3_help, 2, 
N_("%C23*%O$t%C28[%C18$1%C28]%O %C29$2%O")},

{"WhoIs Special", pevt_whoisid_help, 3, 
N_("%C23*%O$t%C28[%C18$1%C28]%O $2")},

{"You Join", pevt_join_help, 3, 
N_("%C19*%O$tNow talking on %C22$2%O")},

{"You Kicked", pevt_ukick_help, 4, 
N_("%C19*%O$tYou have been kicked from %C22$2%C by %C26$3%O (%C20$4%O)")},

{"You Part", pevt_part_help, 3, 
N_("%C19*%O$tYou have left channel %C22$3%O")},

{"You Part with Reason", pevt_partreason_help, 4, 
N_("%C19*%O$tYou have left channel %C22$3%C (%C24$4%O)")},

{"Your Action", pevt_chanaction_help, 131, 
"%C20*$t%B$1%B %C30$2%O"},

{"Your Invitation", pevt_uinvite_help, 3, 
N_("%C20*%O$tYou've invited %C18$1%O to %C22$2%O (%C24$3%O)")},

{"Your Message", pevt_chanmsg_help, 132, 
"%C20%H<%H$4$1%H>%H%O%C30$t$2%O"},

{"Your Nick Changing", pevt_uchangenick_help, 2, 
N_("%C20*%O$tYou are now known as %C18$2%O")},
};
