#!/usr/bin/perl -w

IRC::register("Ignore Script", "1.0", "", "");
IRC::print "\0035:: Loading ignore script ::\003 \n";
IRC::add_message_handler("PRIVMSG", "privmsg_handler");
IRC::add_command_handler("ignore", "ignore_handler");
IRC::add_command_handler("unignore", "unignore_handler");
IRC::add_command_handler("lignore", "lignore_handler");

sub privmsg_handler 
{
  my $line = shift(@_);
  $line =~ /:(.*!.*@.*) .*:.*/;
  $nick_address = $1;
  foreach my $luser (@ignore_list)
  {
    if ($nick_address =~ /$luser/) {return 1;}
  }
  return 0;
}

# ignore regex
sub ignore_handler
{
  my $luser = shift(@_);
  
  push @ignore_list, $luser;
  IRC::print "\0035:: regex $luser added to ignore list. ::\003 \n";
  return 1;
}

# remove regex from ignore list
sub unignore_handler
{
  my $luser = shift(@_);
  my $element;
  undef $element;

  for ($[ .. $#ignore_list) {
    $element = $_, last if $ignore_list[$_] eq $luser;
  }

  if (defined($element)) {
    splice(@ignore_list, $element, 1);
    IRC::print "\0035:: regex $luser removed from ignore list. ::\003 \n";
  } else {
    IRC::print "\0035:: regex $luser not found in ignore list. ::\003 \n";
  }

  return 1;
}

# list ignore list
sub lignore_handler
{
  IRC::print "\0035:: begin ignore list: ::\003 \n";
  foreach (@ignore_list) {
    IRC::print "\0035:: $_ \003 \n";
  }
  IRC::print "\0035:: end ignore list: ::\003 \n";

  return 1;
}
