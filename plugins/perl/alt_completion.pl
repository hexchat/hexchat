
use strict;
use warnings;

Xchat::register( "Tab Completion", "1.0102",
                 "Alternative tab completion behavior" );
Xchat::hook_print( "Key Press", \&complete );
Xchat::hook_print( "Close Context", \&close_context );

my %completions;
my %escape_map = ( '[' => qr![\\\[{]!,
                   '{' => qr![\\\[{]!,
                   '}' => qr![\\\]}]!,
                   ']' => qr![\\\]}]!,
                   '\\' => qr![\\\|]!,
                   '|' => qr![\\\|]!,
                   '.' => qr!\.!,
                   '^' => qr!\^!,
                   '$' => qr!\$!,
                   '*' => qr!\*!,
                   '+' => qr!\+!,
                   '?' => qr!\?!,
                   '(' => qr!\(!,
                   ')' => qr!\)!,
               );
my $escapes = join "", keys %escape_map;
$escapes = qr/[\Q$escapes\E]/;

sub complete {
  # if $_[0][0] contains the value of the key pressed
  # the value for tab is 0xFF09
  # we don't care about other keys
  return Xchat::EAT_NONE unless $_[0][0] == 0xFF09;
  
  # we also don't care about other kinds of tabs besides channel tabs
  return Xchat::EAT_NONE unless Xchat::get_info( "channel" ) =~ m/^(?:#|&)/;

  # In case some other script decides to be stupid and alter the base index
  local $[ = 0;
  
  my $context = Xchat::get_context;
  $completions{$context} ||= {};
  my $completions = $completions{$context};
  my $suffix = Xchat::get_prefs( "completion_suffix" );
  $suffix =~ s/\s+//g;
  $suffix .= " ";
  
  my $input = Xchat::get_info( "inputbox" );
  my $cursor_pos = Xchat::get_info( "state_cursor" );
  my $left = substr( $input, 0, $cursor_pos );
  my $right = substr( $input, $cursor_pos );
  my $length = length $left;
  
  # trim spaces from the end of $left to avoid grabbing the wrong word
  # this is mainly needed for completion at the very beginning where a space
  # is added after the completion
  $left =~ s/(\s+)$//;

  # always add one to the index because
  # 1) if a space is found we want the position after it
  # 2) if a space isn't found then we get back -1
  my $word_start = rindex( $left, " " ) + 1;
  my $word = substr( $left, $word_start );
  $left = substr( $left, 0, -length $word );

  my $command_char = Xchat::get_prefs( "input_command_char" );

  # ignore channels and commands
  if ( $word !~ m{^[${command_char}&#]} ) {
    # this is going to be the "completed" word
    my $completed;

    # used to indicate parital completions so a : isn't added
    my $partial;

    # continuing from a previous completion
    if ( exists $completions->{nicks} && @{$completions->{nicks}}
         && $cursor_pos == $completions->{pos}
         && $word =~ /^\Q$completions->{nicks}[$completions->{index}]/ ) {
      $completions->{index} =
        ( $completions->{index} + 1 ) % @{$completions->{nicks}};
      $completed = $completions->{nicks}[ $completions->{index} ];
    } else {
      # fix $word so { equals [, ] equals }, \ equals |
      # and escape regex metacharacters
      $word =~ s/($escapes)/$escape_map{$1}/g;
      $completions->{nicks} = [ map { $_->{nick} }
                                sort { $b->{lasttalk} <=> $a->{lasttalk}}
                                grep { $_->{nick} =~ /^$word/i }
                                Xchat::get_list( "users" )
                              ];
      $completions->{index} = 0;
      $completed = $completions->{nicks}[ $completions->{index} ];
    }

    my $completion_amount = Xchat::get_prefs( "completion_amount" );
    # don't cycle if the number of possible completions is greater than
    # completion_amount
    if( @{$completions->{nicks}} > $completion_amount
        && @{$completions->{nicks}} != 1 ) {
      # don't print if we tabbed in the beginning and the list of possible
      # completions includes all nicks in the channel
      if( @{$completions->{nicks}} < Xchat::get_list("users") ) {
        Xchat::print( join " ", @{$completions->{nicks}} );
      }
      $completed = lcs( $completions->{nicks} );
      $partial = 1;
    }

    if ( $completed ) {

      # move the cursor back to the front
      Xchat::command( "setcursor -$cursor_pos" );
      if ( $word_start == 0 && !$partial ) {
        # at the start of the line append completion suffix
        Xchat::command( "settext $completed$suffix$right");
        $completions->{pos} = length( "$completed$suffix" );
      } else {
        Xchat::command( "settext $left$completed$right" );
        $completions->{pos} = length( "$left$completed" );
      }
      Xchat::command( "setcursor +$completions->{pos}" );
    
      # debugging stuff
#       local $, = " ";
#       Xchat::print [ qq{[input:$input]},
#                      qq{[cursor:$cursor_pos]},
#                      qq{[start:$word_start]},
#                      qq{[length:$length]},
#                      qq{[left:$left]},
#                      qq{[word:$word]}, qq{[right:$right]},
#                      qq{[completed:$completed]},
#                    ];
    }
    return Xchat::EAT_ALL;
  } else {
    return Xchat::EAT_NONE;
  }
}

# Remove completion related data for tabs that are closed
sub close_context {
  my $context = Xchat::get_context;
  delete $completions{$context};
  return Xchat::EAT_NONE;
}

# Longest common substring
# Used for partial completion when using non-cycling completion

sub lcs {
  my @nicks = @{+shift};

  return "" if @nicks == 0;
  return $nicks[0] if @nicks == 1;

  my $substring = shift @nicks;

  while(@nicks) {
    $substring = common_string( $substring, shift @nicks );
  }
  return $substring;
}

sub common_string {
  my ($nick1, $nick2) = @_;
  my $index = 0;
  while( ($index < length $nick1) && ($index < length $nick2)
         && lc(substr( $nick1, $index, 1)) eq lc(substr( $nick2, $index, 1))
       ) { $index++ }
  return substr( $nick1, 0, $index );
}
