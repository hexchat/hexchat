#!/usr/bin/perl -w

IRC::register("sample script", "0.1", "", "");
IRC::print "Loading sample script\n";
IRC::print "registering handlers\n";
IRC::add_message_handler("PRIVMSG", "privmsg_handler");
IRC::add_command_handler("sample", "sample_command_handler");
IRC::add_command_handler("blah", "blah");

sub privmsg_handler 
{
  my $line = shift(@_);
  $line =~ /:(.*)!(.*@.*) .*:(.*)/;
  IRC::print "[$1]\0034::\003 $3\n";
  return 1;
}

sub sample_command_handler
{
  IRC::print "\0034:: sample command ::\003 \n";
  return 1;
}

# blah user/channel
sub blah
{
  my $victim = shift(@_);

  IRC::print "\0034:: blah ::\003 \n";
  IRC::send_raw "PRIVMSG $victim :blah\n"; 
  return 1;
}
