/* this file is auto generated, edit textevents.in instead! */

const struct text_event te[] = {

{"Add Notify", pevt_generic_nick_help, 1, 
N_("%C22*%O$t$1 added to notify list.")},

{"Ban List", pevt_banlist_help, 4, 
N_("%C22*%O$t$1 Banlist:%C19 $4%C20 $2%C21 $3")},

{"Banned", pevt_generic_channel_help, 1, 
N_("%C22*%O$tCannot join%C26 %B$1 %O(You are banned).")},

{"Beep", pevt_generic_none_help, 0, 
""},

{"Change Nick", pevt_changenick_help, 2, 
N_("%C22*%O$t$1 is now known as $2")},

{"Channel Action", pevt_chanaction_help, 3, 
N_("%C18*$t$1%O $2")},

{"Channel Action Hilight", pevt_chanaction_help, 3, 
N_("%C21*%O$t%C21%B$1%O%C21 $2")},

{"Channel Ban", pevt_chanban_help, 2, 
N_("%C22*%O$t$1 sets ban on $2")},

{"Channel Creation", pevt_chandate_help, 2, 
N_("%C22*%O$tChannel $1 created on $2")},

{"Channel DeHalfOp", pevt_chandehop_help, 2, 
N_("%C22*%O$t%C26$1%O removes channel half-operator status from%C26 $2")},

{"Channel DeOp", pevt_chandeop_help, 2, 
N_("%C22*%O$t%C26$1%O removes channel operator status from%C26 $2")},

{"Channel DeVoice", pevt_chandevoice_help, 2, 
N_("%C22*%O$t%C26$1%O removes voice from%C26 $2")},

{"Channel Exempt", pevt_chanexempt_help, 2, 
N_("%C22*%O$t$1 sets exempt on $2")},

{"Channel Half-Operator", pevt_chanhop_help, 2, 
N_("%C22*%O$t%C26$1%O gives channel half-operator status to%C26 $2")},

{"Channel INVITE", pevt_chaninvite_help, 2, 
N_("%C22*%O$t$1 sets invite on $2")},

{"Channel List", pevt_generic_none_help, 0, 
N_("%UChannel          Users   Topic")},

{"Channel Message", pevt_chanmsg_help, 4, 
N_("%C18%B%B$4$1%O%C18%O$t$2")},

{"Channel Mode Generic", pevt_chanmodegen_help, 4, 
N_("%C22*%O$t$1 sets mode $2$3 $4")},

{"Channel Modes", pevt_chanmodes_help, 2, 
N_("%C22*%O$t%C22Channel $1 modes: $2")},

{"Channel Msg Hilight", pevt_chanmsg_help, 4, 
N_("$4%C21%B$1%O%C21$t$2")},

{"Channel Notice", pevt_channotice_help, 3, 
N_("%C28-%C29$1/$2%C28-%O$t$3")},

{"Channel Operator", pevt_chanop_help, 2, 
N_("%C22*%O$t%C26$1%O gives channel operator status to%C26 $2")},

{"Channel Remove Exempt", pevt_chanrmexempt_help, 2, 
N_("%C22*%O$t$1 removes exempt on $2")},

{"Channel Remove Invite", pevt_chanrminvite_help, 2, 
N_("%C22*%O$t$1 removes invite on $2")},

{"Channel Remove Keyword", pevt_chanrmkey_help, 1, 
N_("%C22*%O$t$1 removes channel keyword")},

{"Channel Remove Limit", pevt_chanrmlimit_help, 1, 
N_("%C22*%O$t$1 removes user limit")},

{"Channel Set Key", pevt_chansetkey_help, 2, 
N_("%C22*%O$t$1 sets channel keyword to $2")},

{"Channel Set Limit", pevt_chansetlimit_help, 2, 
N_("%C22*%O$t$1 sets channel limit to $2")},

{"Channel UnBan", pevt_chanunban_help, 2, 
N_("%C22*%O$t$1 removes ban on $2")},

{"Channel Voice", pevt_chanvoice_help, 2, 
N_("%C22*%O$t%C26$1%O gives voice to%C26 $2")},

{"Connected", pevt_generic_none_help, 0, 
N_("%C22*%O$t%C22Connected. Now logging in...")},

{"Connecting", pevt_connect_help, 3, 
N_("%C22*%O$t%C22Connecting to $1 ($2) port $3%O...")},

{"Connection Failed", pevt_connfail_help, 1, 
N_("%C21*%O$t%C21Connection failed. Error: $1")},

{"CTCP Generic", pevt_ctcpgen_help, 2, 
N_("%C22*%O$tReceived a CTCP $1 from $2")},

{"CTCP Generic to Channel", pevt_ctcpgenc_help, 3, 
N_("%C22*%O$tReceived a CTCP $1 from $2 (to $3)")},

{"CTCP Send", pevt_ctcpsend_help, 2, 
N_("%C19>%O$1%C19<%O$tCTCP $2")},

{"CTCP Sound", pevt_ctcpsnd_help, 2, 
N_("%C22*%O$tReceived a CTCP Sound $1 from $2")},

{"CTCP Sound to Channel", pevt_ctcpsnd_help, 3, 
N_("%C22*%O$tReceived a CTCP Sound $1 from $2 (to $3)")},

{"DCC CHAT Abort", pevt_dccchatabort_help, 1, 
N_("%C22*%O$tDCC CHAT to %C26$1%O aborted.")},

{"DCC CHAT Connect", pevt_dccchatcon_help, 2, 
N_("%C22*%O$tDCC CHAT connection established to %C26$1 %C30[%O$2%C30]")},

{"DCC CHAT Failed", pevt_dccchaterr_help, 4, 
N_("%C22*%O$tDCC CHAT to %C26$1%O lost ($4).")},

{"DCC CHAT Offer", pevt_generic_nick_help, 1, 
N_("%C22*%O$tReceived a DCC CHAT offer from $1")},

{"DCC CHAT Offering", pevt_generic_nick_help, 1, 
N_("%C22*%O$tOffering DCC CHAT to $1")},

{"DCC CHAT Reoffer", pevt_generic_nick_help, 1, 
N_("%C22*%O$tAlready offering CHAT to $1")},

{"DCC Conection Failed", pevt_dccconfail_help, 3, 
N_("%C22*%O$tDCC $1 connect attempt to%C26 $2%O failed (err=$3).")},

{"DCC Generic Offer", pevt_dccgenericoffer_help, 2, 
N_("%C22*%O$tReceived '$1%O' from $2")},

{"DCC Header", pevt_generic_none_help, 0, 
N_("%C24,18 Type  To/From    Status  Size    Pos     File         ")},

{"DCC Malformed", pevt_malformed_help, 2, 
N_("%C22*%O$tReceived a malformed DCC request from %C26$1%O.%010%C22*%O$tContents of packet: $2")},

{"DCC Offer", pevt_dccoffer_help, 3, 
N_("%C22*%O$tOffering%C26 $1%O to%C26 $2")},

{"DCC Offer Not Valid", pevt_generic_none_help, 0, 
N_("%C22*%O$tNo such DCC offer.")},

{"DCC RECV Abort", pevt_dccfileabort_help, 2, 
N_("%C22*%O$tDCC RECV%C26 $2%O to%C26 $1%O aborted.")},

{"DCC RECV Complete", pevt_dccrecvcomp_help, 4, 
N_("%C22*%O$tDCC RECV%C26 $1%O from%C26 $3%O complete %C30[%C26$4%O cps%C30]%O.")},

{"DCC RECV Connect", pevt_dcccon_help, 3, 
N_("%C22*%O$tDCC RECV connection established to%C26 $1 %C30[%O$2%C30]")},

{"DCC RECV Failed", pevt_dccrecverr_help, 4, 
N_("%C22*%O$tDCC RECV%C26 $1%O from%C26 $3%O failed ($4).")},

{"DCC RECV File Open Error", pevt_generic_file_help, 2, 
N_("%C22*%O$tDCC RECV: Cannot open $1 for writing ($2).")},

{"DCC Rename", pevt_dccrename_help, 2, 
N_("%C22*%O$tThe file%C26 $1%C already exists, saving it as%C26 $2%O instead.")},

{"DCC RESUME Request", pevt_dccresumeoffer_help, 3, 
N_("%C22*%O$t%C26$1 %Ohas requested to resume%C26 $2 %Cfrom%C26 $3%C.")},

{"DCC SEND Abort", pevt_dccfileabort_help, 2, 
N_("%C22*%O$tDCC SEND%C26 $2%O to%C26 $1%O aborted.")},

{"DCC SEND Complete", pevt_dccsendcomp_help, 3, 
N_("%C22*%O$tDCC SEND%C26 $1%O to%C26 $2%O complete %C30[%C26$3%O cps%C30]%O.")},

{"DCC SEND Connect", pevt_dcccon_help, 3, 
N_("%C22*%O$tDCC SEND connection established to%C26 $1 %C30[%O$2%C30]")},

{"DCC SEND Failed", pevt_dccsendfail_help, 3, 
N_("%C22*%O$tDCC SEND%C26 $1%O to%C26 $2%O failed. $3")},

{"DCC SEND Offer", pevt_dccsendoffer_help, 4, 
N_("%C22*%O$t%C26$1 %Ohas offered%C26 $2 %O(%C26$3 %Obytes)")},

{"DCC Stall", pevt_dccstall_help, 3, 
N_("%C22*%O$tDCC $1%C26 $2 %Oto%C26 $3 %Cstalled - aborting.")},

{"DCC Timeout", pevt_dccstall_help, 3, 
N_("%C22*%O$tDCC $1%C26 $2 %Oto%C26 $3 %Otimed out - aborting.")},

{"Delete Notify", pevt_generic_nick_help, 1, 
N_("%C22*%O$t$1 deleted from notify list.")},

{"Disconnected", pevt_discon_help, 1, 
N_("%C22*%O$tDisconnected ($1).")},

{"Found IP", pevt_foundip_help, 1, 
N_("%C22*%O$tFound your IP: [$1]")},

{"Generic Message", pevt_genmsg_help, 2, 
N_("$1$t$2")},

{"Ignore Add", pevt_ignoreaddremove_help, 1, 
N_("%O%C26$1%O added to ignore list.")},

{"Ignore Changed", pevt_ignoreaddremove_help, 1, 
N_("Ignore on %C26$1%O changed.")},

{"Ignore Footer", pevt_generic_none_help, 0, 
N_("%C24,18                                                              ")},

{"Ignore Header", pevt_generic_none_help, 0, 
N_("%C24,18 Hostmask                  PRIV NOTI CHAN CTCP DCC  INVI UNIG ")},

{"Ignore Remove", pevt_ignoreaddremove_help, 1, 
N_("%O%C26$1%O removed from ignore list.")},

{"Ignorelist Empty", pevt_generic_none_help, 0, 
N_("  Ignore list is empty.")},

{"Invite", pevt_generic_channel_help, 1, 
N_("%C22*%O$tCannot join%C26 %B$1 %O(Channel is invite only).")},

{"Invited", pevt_invited_help, 3, 
N_("%C22*%O$tYou have been invited to%C26 $1%O by%C26 $2%C (%C26$3%C)")},

{"Join", pevt_join_help, 3, 
N_("%C19*%O$t%C19%B$1 %B($3) has joined $2")},

{"Keyword", pevt_generic_channel_help, 1, 
N_("%C22*%O$tCannot join%C26 %B$1 %O(Requires keyword).")},

{"Kick", pevt_kick_help, 4, 
N_("%C21*%O$t%C21$1 has kicked $2 from $3 ($4%O%C21)")},

{"Killed", pevt_kill_help, 2, 
N_("%C22*%O$tYou have been killed by $1 ($2%O%C22)")},

{"Message Send", pevt_ctcpsend_help, 2, 
N_("%C19>%O$1%C19<%O$t$2")},

{"Motd", pevt_servertext_help, 1, 
N_("%C16*%O$t$1%O")},

{"MOTD Skipped", pevt_generic_none_help, 0, 
N_("%C22*%O$t%C22MOTD Skipped.")},

{"Nick Clash", pevt_nickclash_help, 2, 
N_("%C22*%O$t$1 already in use. Retrying with $2...")},

{"Nick Failed", pevt_generic_none_help, 0, 
N_("%C22*%O$tNickname already in use. Use /NICK to try another.")},

{"No DCC", pevt_generic_none_help, 0, 
N_("%C22*%O$tNo such DCC.")},

{"No Running Process", pevt_generic_none_help, 0, 
N_("%C22*%O$tNo process is currently running")},

{"Notice", pevt_notice_help, 2, 
N_("%C28-%C29$1%C28-%O$t$2")},

{"Notice Send", pevt_ctcpsend_help, 2, 
N_("%C19>%O$1%C19<%O$t$2")},

{"Notify Empty", pevt_generic_none_help, 0, 
N_("$tNotify list is empty.")},

{"Notify Header", pevt_generic_none_help, 0, 
N_("%C24,18 %B  Notify List                           ")},

{"Notify Number", pevt_notifynumber_help, 1, 
N_("%C22*%O$t$1 users in notify list.")},

{"Notify Offline", pevt_generic_nick_help, 2, 
N_("%C22*%O$tNotify: $1 is offline ($2).")},

{"Notify Online", pevt_generic_nick_help, 2, 
N_("%C22*%O$tNotify: $1 is online ($2).")},

{"Open Dialog", pevt_generic_none_help, 0, 
""},

{"Part", pevt_part_help, 3, 
N_("%C23*%O$t%C23$1 (%O%C23$2) has left $3")},

{"Part with Reason", pevt_partreason_help, 4, 
N_("%C23*%O$t%C23$1 (%O%C23$2) has left $3 (%O%C23%B%B$4%O%C23)")},

{"Ping Reply", pevt_pingrep_help, 2, 
N_("%C22*%O$tPing reply from $1: $2 second(s)")},

{"Ping Timeout", pevt_pingtimeout_help, 1, 
N_("%C22*%O$tNo ping reply for $1 seconds, disconnecting.")},

{"Private Message", pevt_privmsg_help, 3, 
N_("%C28*%C29$3$1%C28*$t%O$2")},

{"Private Message to Dialog", pevt_dprivmsg_help, 3, 
N_("%C18%B%B$3$1%O$t$2")},

{"Process Already Running", pevt_generic_none_help, 0, 
N_("%C22*%O$tA process is already running")},

{"Quit", pevt_quit_help, 3, 
N_("%C23*%O$t%C23$1 has quit (%O%C23%B%B$2%O%C23)")},

{"Raw Modes", pevt_rawmodes_help, 2, 
N_("%C22*%O$t$1 sets modes%B %C30[%O$2%B%C30]")},

{"Receive Wallops", pevt_dprivmsg_help, 2, 
N_("%C28-%C29$1/Wallops%C28-%O$t$2")},

{"Resolving User", pevt_resolvinguser_help, 2, 
N_("%C22*%O$tLooking up IP number for%C26 $1%O...")},

{"Server Connected", pevt_generic_none_help, 0, 
N_("%C22*%O$t%C22Connected.")},

{"Server Error", pevt_servererror_help, 1, 
N_("%C22*%O$t$1")},

{"Server Lookup", pevt_serverlookup_help, 1, 
N_("%C22*%O$t%C22Looking up $1")},

{"Server Notice", pevt_servertext_help, 2, 
N_("%C22*%O$t$1")},

{"Server Text", pevt_servertext_help, 2, 
N_("%C22*%O$t$1")},

{"Stop Connection", pevt_sconnect_help, 1, 
N_("%C22*%O$tStopped previous connection attempt (pid=$1)")},

{"Topic", pevt_topic_help, 2, 
N_("%C29*%O$t%C29Topic for $1%C %C29is: $2")},

{"Topic Change", pevt_newtopic_help, 3, 
N_("%C22*%O$t$1 has changed the topic to: $2")},

{"Topic Creation", pevt_topicdate_help, 3, 
N_("%C29*%O$t%C29Topic for $1%C %C29set by $2%C %C29at $3")},

{"Unknown Host", pevt_generic_none_help, 0, 
N_("%C22*%O$tUnknown host. Maybe you misspelled it?")},

{"User Limit", pevt_generic_channel_help, 1, 
N_("%C22*%O$tCannot join%C26 %B$1 %O(User limit reached).")},

{"Users On Channel", pevt_usersonchan_help, 2, 
N_("%C22*%O$t%C26Users on $1:%C $2")},

{"WhoIs Authenticated", pevt_whoisauth_help, 3, 
N_("%C22*%O$t%C28[%O$1%C28] %O$2%C27 $3")},

{"WhoIs Away Line", pevt_whois5_help, 2, 
N_("%C22*%O$t%C28[%O$1%C28] %Cis away %C30(%O$2%O%C30)")},

{"WhoIs Channel/Oper Line", pevt_whois2_help, 2, 
N_("%C22*%O$t%C28[%O$1%C28]%O $2")},

{"WhoIs End", pevt_whois6_help, 1, 
N_("%C22*%O$t%C28[%O$1%C28] %OEnd of WHOIS list.")},

{"WhoIs Identified", pevt_whoisid_help, 2, 
N_("%C22*%O$t%C28[%O$1%C28]%O $2")},

{"WhoIs Idle Line", pevt_whois4_help, 2, 
N_("%C22*%O$t%C28[%O$1%C28]%O idle%C26 $2")},

{"WhoIs Idle Line with Signon", pevt_whois4t_help, 3, 
N_("%C22*%O$t%C28[%O$1%C28]%O idle%C26 $2%O, signon:%C26 $3")},

{"WhoIs Name Line", pevt_whois1_help, 4, 
N_("%C22*%O$t%C28[%O$1%C28] %C30(%O$2@$3%C30)%O: $4")},

{"WhoIs Real Host", pevt_whoisrealhost_help, 4, 
N_("%C22*%O$t%C28[%O$1%C28] %Oreal user@host%C27 $2%O, real IP%C27 $3")},

{"WhoIs Server Line", pevt_whois3_help, 2, 
N_("%C22*%O$t%C28[%O$1%C28]%O $2")},

{"WhoIs Special", pevt_whoisid_help, 3, 
N_("%C22*%O$t%C28[%O$1%C28]%O $2")},

{"You Join", pevt_join_help, 3, 
N_("%C19*%O$t%C19Now talking on $2")},

{"You Kicked", pevt_ukick_help, 4, 
N_("%C23*$tYou have been kicked from $2 by $3 ($4%O%C23)")},

{"You Part", pevt_part_help, 3, 
N_("%C23*$tYou have left channel $3")},

{"You Part with Reason", pevt_partreason_help, 4, 
N_("%C23*$tYou have left channel $3 (%O%C23%B%B$4%O%C23)")},

{"Your Invitation", pevt_uinvite_help, 3, 
N_("%C22*%O$tYou've invited%C26 $1%O to%C26 $2%O (%C26$3%O)")},

{"Your Message", pevt_chanmsg_help, 4, 
N_("%C31%B%B$4$1%O%C30$t$2")},

{"Your Nick Changing", pevt_uchangenick_help, 2, 
N_("%C22*%O$tYou are now known as $2")},
};
