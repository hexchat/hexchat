#!/usr/bin/perl

# ------------------------------------------------------------------
# Callback Wrappers for X-Chat 0.9.2 and above
# by Dagmar d'Surreal 
# Date:  March 11th, 1999
# Version: 1.0
#
# This comes without any kind of warranty whatsoever, and probably won't
# work with anything other than X-Chat 0.9.2.
#
# For people unfamiliar with calling packagized subroutines, just invoke these
# as Wrapper::dc_list and Wrapper::channel_list.  The example code sucks, but
# it's only meant to get the most basic ideas across.
#

package Wrapper;
BEGIN
  {

# sub Wrapper::dcc_list
#   This will return an array containing hash references which contain
#   information about the status of all current DCC connections.  The
#   advantage to using this is that it makes the code you write a lot
#   easier for other people to read, debug, and possibly improve upon.
#   The amount of time this routine takes to run is next to nothing.
#
#   Example:
#     my (@dcc_list) = Wrapper::dcc_list();
#     $first_nick = $dcc_list[0]->{nick};
#
#   MUCH simpler to read.
#
sub dcc_list {
  my @list = IRC::dcc_list();
  my @array;
  while ($list[0]) {
    my $ref;
      $ref->{nick} = shift(@list);
      $ref->{file} = shift(@list);
      $ref->{type} = shift(@list);
      $ref->{stat} = shift(@list);
      $ref->{cps} = shift(@list);
      $ref->{size} = shift(@list);
      $ref->{resumeable} = shift(@list);
      $ref->{addr} = shift(@list);
      $ref->{destfile} = shift(@list);
      push(@array, $ref);
  }
  return @array;
}

# sub Wrapper::channel_list
#   This will return a reference to a hash of arrays that will tell you what
#   servers you are currently connected to, what your nickname is on that server
#   and what channels that nickname is in.  I would strongly suggest against
#   trying to connect to the same server twice with one copy of X-Chat.
#   (Aside from the fact that you would be wasting a connection.)
#
#   Example:
#     print "List of servers currently connected to:\n";
#     my $ref = Wrapper::channel_list(); 
#     foreach $server (keys %{$ref}) {
#       print "  $server\n";
#     }
#
sub channel_list {
  my @list = IRC::channel_list();
  my $ref;
  while ($list[0]) {
    my $chan = shift(@list);
    my $server = shift(@list);
    my $nick = shift(@list);
    $ref->{$server}->{nick} = $nick;
    push(@{$ref->{$server}->{channels}}, $chan);
  }
  return $ref
}

  }
END;

#
# ------------------------------------------------------------------
# RFC case-sensitivity support for X-Chat
# by Dagmar d'Surreal
# GPL'd and stuff on March 25th, 1999
#
# My only comment is that people should remember that RFCs are not
# just suggestions, even in their most insignificant aspect.  Just take
# a look at what has happened to HTML for a hideous example.
#

# sub rfc_lcase ( string )
#   This will convert a string to all-lowercase using the character set
#   described in section 2.2 of RFC 1459, with respect given to the extra
#   set of characters converted by the modern version of ircd.
#
sub rfc_lcase {
  my $line = shift @_;
  # You woudn't believe how long it took me to figure out to use parens here.
  $line =~ tr(A-Z\133-\136)(a-z\173-\176);
  return $line;
}

# sub rfc_ucase ( string )
#   This is the complement to rfc_lcase.
#
sub rfc_ucase {
  my $line = shift @_;
  $line =~ tr(a-z\170-\176)(A-Z\130-\136);
  return $line;
}

# sortsub in_rfc_order
#   Call this sort subroutine when you want to sort a set of somethings
#   by the order they would be accoring to the IRC server.
#
sub in_rfc_order { rfc_ucase($a) cmp rfc_ucase($b) }



# That which does not true shall surely die();
1;
# Instant documentation provided by 'grep "^#" filename'.
