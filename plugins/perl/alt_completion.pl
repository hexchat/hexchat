
use strict;
use warnings;

my $previous_pos = 0;
my @completions;
my $completion_idx = 0;

Xchat::register( "Tab Completion", "1.0001",
                 "Alternative tab completion behavior" );
Xchat::hook_print( "Key Press", \&tab_complete );
sub tab_complete {
  # if $_[0][0] contains the value of the key pressed
  # the value for tab is 0xFF09
  # we don't care about other keys
  return Xchat::EAT_NONE unless $_[0][0] == 0xFF09;
  
  # In case some other script decides to be stupid and
  # alter the base index
  local $[ = 0;

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

  # fix $word so { equals [, ] equals }, \ equals |
  my %case_map = ( "[" => "[\\\\[{]",
                   "{" => "[\\\\[{]",
                   "}" => "[\\\\]}]",
                   "]" => "[\\\\]}]",
                   "\\" => "[\\\\|]",
                   "|" => "[\\\\|]"
                 );
  $word =~ s/([[\]{}|])/$case_map{$1}/g;


  # ignore channels and commands 
  if ( $word !~ m{^[/&#]} ) {
    #Xchat::print Dumper $_[0];
    
    # this is going to be the "completed" word
    my $completed;

    # continuing from a previous completion
    if ( @completions && $cursor_pos == $previous_pos ) {
      $completion_idx = ( $completion_idx + 1 )
        % @completions;
      $completed = $completions[ $completion_idx ];
    } else {
      @completions = map { $_->{nick} }
        sort { $b->{lasttalk} <=> $a->{lasttalk}}
          grep { $_->{nick} =~ /^$word/i }
            Xchat::get_list( "users" );
      $completion_idx = 0;
      $completed = $completions[ $completion_idx ];
    }

    my $completion_amount = Xchat::get_prefs( "completion_amount" );
    # don't cycle if the number of possible completions is greater than
    # completion_amount
    if( @completions > $completion_amount && @completions != 1 ) {
      # don't print if we tabbed in the beginning and the list of possible
      # completions includes all nicks in the channel
      if( @completions < Xchat::get_list("users") ) {
        Xchat::print [join " ", @completions];
      }
      return Xchat::EAT_XCHAT;
    }

    if ( $completed ) {

      # move the cursor back to the front
      Xchat::command( "setcursor -$cursor_pos" );
      if ( $word_start == 0 ) {
        # at the start of the line append completion suffix
        Xchat::command( "settext $completed$suffix$right");
        $previous_pos = length( "$completed$suffix" );
      } else {
        Xchat::command( "settext $left$completed$right" );
        $previous_pos = length( "$left$completed" );
      }
      Xchat::command( "setcursor +$previous_pos" );
    
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
