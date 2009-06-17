use strict;
use warnings;

# if the last time you addressed someone was greater than this many minutes
# ago, ignore it
# this avoids having people you have talked to a long time ago coming up too
# early in the completion list
my $last_use_threshold = 10; # 10 minutes

Xchat::register(
	"Tab Completion", "1.0302", "Alternative tab completion behavior"
);
Xchat::hook_print( "Key Press", \&complete );
Xchat::hook_print( "Close Context", \&close_context );
Xchat::hook_print( "Focus Tab", \&focus_tab );
Xchat::hook_print( "Part", \&clean_selected );
Xchat::hook_print( "Part with Reason", \&clean_selected );
Xchat::hook_command( "", \&track_selected );

sub SHIFT() { 1 }
sub CTRL() { 4 }
sub ALT() { 8 }

my %completions;
my %last_visit;
my %selected;
my %escape_map = (
	'[' => qr![\[{]!,
	'{' => qr![\[{]!,
	'}' => qr![\]}]!,
	']' => qr![\]}]!,
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
	'-' => qr!\-!,
);

my $escapes = join "", keys %escape_map;
$escapes = qr/[\Q$escapes\E]/;

sub complete {
	# if $_[0][0] contains the value of the key pressed
	# $_[0][1] contains modifiers
	# the value for tab is 0xFF09
	# the value for shift-tab(Left Tab) is 0xFE20
	# we don't care about other keys

	return Xchat::EAT_NONE unless $_[0][0] == 0xFF09 || $_[0][0] == 0xFE20;
	return Xchat::EAT_NONE if $_[0][0] == 0xFF09 && $_[0][1] & (CTRL|ALT|SHIFT);
	
	# we also don't care about other kinds of tabs besides channel tabs
	return Xchat::EAT_NONE unless Xchat::context_info()->{type} == 2;
	
	# In case some other script decides to be stupid and alter the base index
	local $[ = 0;
	
	# loop backwards for shift+tab
	my $delta = $_[0][1] & SHIFT ? -1 : 1;
	my $context = Xchat::get_context;
	$completions{$context} ||= {};
	
	my $completions = $completions{$context};
	$completions->{pos} ||= -1;
	
	my $suffix = Xchat::get_prefs( "completion_suffix" );
	$suffix =~ s/^\s+//;
	
	my $input = Xchat::get_info( "inputbox" );
	my $cursor_pos = Xchat::get_info( "state_cursor" );
	my $left = substr( $input, 0, $cursor_pos );
	my $right = substr( $input, $cursor_pos );
	my $length = length $left;

	# trim spaces from the end of $left to avoid grabbing the wrong word
	# this is mainly needed for completion at the very beginning where a space
	# is added after the completion
	$left =~ s/\s+$//;

	# always add one to the index because
	# 1) if a space is found we want the position after it
	# 2) if a space isn't found then we get back -1
	my $word_start = rindex( $left, " " ) + 1;
	my $word = substr( $left, $word_start );
	$left = substr( $left, 0, -length $word );

	my $command_char = Xchat::get_prefs( "input_command_char" );
	# ignore commands
	if( $word !~ m{^[${command_char}]} ) {
		if( $cursor_pos == length $input # end of input box
#			&& $input =~ /(?<!\w|$escapes)$/ # not a valid nick char
			&& $input =~ /(?<![\x41-\x5A\x61-\x7A\x30-\x39\x5B-\x60\x7B-\x7D-])$/
			&& $cursor_pos != $completions->{pos} # not continuing a completion
			&& $word !~ /^[&#]/ ) { # not a channel
			$word_start = $cursor_pos;
			$left = $input;
			$length = length $length;
			$right = "";
			$word = "";
		}

		my $completed; # this is going to be the "completed" word

		# for parital completions and channel names so a : isn't added
		my $skip_suffix = ($word =~ /^[&#]/) ? 1 : 0;
		
		# continuing from a previous completion
		if(
			exists $completions->{matches} && @{$completions->{matches}}
			&& $cursor_pos == $completions->{pos}
			&& $word =~ /^\Q$completions->{matches}[$completions->{index}]/
		) {
			$completions->{index} += $delta;

			if( $completions->{index} < 0 ) {
				$completions->{index} += @{$completions->{matches}};
			} else {
				$completions->{index} %= @{$completions->{matches}};
			}

			$completed = $completions->{matches}[ $completions->{index} ];
		} else {

			if( $word =~ /^[&#]/ ) {
				$completions->{matches} = [ matching_channels( $word ) ];
			} else {
				# fix $word so { equals [, ] equals }, \ equals |
				# and escape regex metacharacters
				$word =~ s/($escapes)/$escape_map{$1}/g;

				$completions->{matches} = [ matching_nicks( $word ) ];
			}
			$completions->{index} = 0;

			$completed = $completions->{matches}[ $completions->{index} ];
		}
		
		my $completion_amount = Xchat::get_prefs( "completion_amount" );
		
		# don't cycle if the number of possible completions is greater than
		# completion_amount
		if(
			@{$completions->{matches}} > $completion_amount
			&& @{$completions->{matches}} != 1
		) {
			# don't print if we tabbed in the beginning and the list of possible
			# completions includes all nicks in the channel
			if( @{$completions->{matches}} < Xchat::get_list("users") ) {
				Xchat::print( join " ", @{$completions->{matches}}, "\n" );
			}
			
			$completed = lcs( $completions->{matches} );
			$skip_suffix = 1;
		}
		
		if( $completed ) {
			
			if( $word_start == 0 && !$skip_suffix ) {
				# at the start of the line append completion suffix
				Xchat::command( "settext $completed$suffix$right");
				$completions->{pos} = length( "$completed$suffix" );
			} else {
				Xchat::command( "settext $left$completed$right" );
				$completions->{pos} = length( "$left$completed" );
			}
			
			Xchat::command( "setcursor $completions->{pos}" );
		}
# debugging stuff
#		local $, = " ";
#		my $input_length = length $input;
#		Xchat::print [
#			qq{[input:$input]},
#			qq{[input_length:$input_length]},				
#			qq{[cursor:$cursor_pos]},
#			qq{[start:$word_start]},
#			qq{[length:$length]},
#			qq{[left:$left]},
#			qq{[word:$word]}, qq{[right:$right]},
#			qq{[completed:$completed]},
#			qq{[pos:$completions->{pos}]},
#		];
#		use Data::Dumper;
#		local $Data::Dumper::Indent = 0;
#		Xchat::print Dumper $completions->{matches};

		return Xchat::EAT_ALL;
	} else {
		return Xchat::EAT_NONE;
	}
}


# all channels starting with $word
sub matching_channels {
	my $word = shift;

	# for use in compare_channels();
	our $current_chan;
	local $current_chan = Xchat::get_info( "channel" );

	my $conn_id = Xchat::get_info( "id" );
	$word =~ s/^[&#]+//;

	return
		map {	$_->[1]->{channel} }
		sort compare_channels map {
			my $chan = $_->{channel};
			$chan =~ s/^[#&]+//;

			# comparisons will be done based only on the name
			# matching name, same connection, only channels
			$chan =~ /^$word/ && $_->{id} == $conn_id ?
			[ $chan, $_ ] :
			()
		} channels();
}

sub channels {
	return grep { $_->{type} == 2 } Xchat::get_list( "channels" );
}

sub compare_channels {
	# package variable, value set in matching_channels()
	our $current_chan;

	# turn off warnings generated from channels that have not yet been visited
	# since the script was loaded
	no warnings "uninitialized";

	# the current channel is always first, then ordered by most recently visited
	return
		$a->[1]{channel} eq $current_chan ? -1 :
		$b->[1]{channel} eq $current_chan ? 1 :
		$last_visit{ $b->[1]{context} } <=> $last_visit{ $a->[1]{context} }
		|| $a->[1]{channel} cmp $b->[1]{channel};

}

sub matching_nicks {
	my $word = shift;

	# for use in compare_nicks()
	our ($my_nick, $selections, $now);
	local $my_nick = Xchat::get_info( "nick" );
	local $selections = $selected{ Xchat::get_context() };
	local $now = time;

	return
		map { $_->{nick} }
		sort compare_nicks grep {
			$_->{nick} =~ /^$word/i
		} Xchat::get_list( "users" )

}

sub compare_nicks {
	# more package variables, value set in matching_nicks()
	our $my_nick;
	our $selections;
	our $now;

	# turn off the warnings that get generated from users who have yet to speak
	# since the script was loaded
	no warnings "uninitialized";
	
	my $a_time
		= ($now - $selections->{ $a->{nick} }) < ($last_use_threshold * 60) ?
		$selections->{ $a->{nick} } :
		$a->{lasttalk};
	my $b_time
		= ($now - $selections->{ $b->{nick} }) < ($last_use_threshold * 60) ?
		$selections->{ $b->{nick} } :
		$b->{lasttalk};

	# our own nick is always last, then ordered by the people we spoke to most
	# recently and the people who were speaking most recently
	return 
		$a->{nick} eq $my_nick ? 1 :
		$b->{nick} eq $my_nick ? -1 :
		$b_time <=> $a_time
		|| Xchat::nickcmp( $a->{nick}, $b->{nick} );

#		$selections->{ $b->{nick} } <=> $selections->{ $a->{nick} }
#		||	$b->{lasttalk} <=> $a->{lasttalk}

}

# Remove completion related data for tabs that are closed
sub close_context {
	my $context = Xchat::get_context;
	delete $completions{$context};
	delete $last_visit{$context};
	return Xchat::EAT_NONE;
}

# track visit times
sub focus_tab {
	$last_visit{Xchat::get_context()} = time();
}

# keep track of the last time a message was addressed to someone
# a message is considered addressed to someone if their nick is used followed
# by the completion suffix
sub track_selected {
	my $input = $_[1][0];
	
	my $suffix = Xchat::get_prefs( "completion_suffix" );
	for( grep defined, $input =~ /^(.+)\Q$suffix/, $_[0][0] ) {
		if( in_channel( $_ ) ) {
			$selected{Xchat::get_context()}{$_} = time();
			last;
		}
	}

	return Xchat::EAT_NONE;
}

# if a user is in the current channel
# user_info() can also be used instead of the loop
sub in_channel {
	my $target = shift;
	for my $nick ( nicks() ) {
		if( $nick eq $target ) {
			return 1;
		}
	}

	return 0;
}

# list of nicks in the current channel
sub nicks {
	return map { $_->{nick} } Xchat::get_list( "users" );
}

# remove people from the selected list when they leave the channel
sub clean_selected {
	delete $selected{ Xchat::get_context() }{$_[0][0]};
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

	$index++ while(
		($index < length $nick1) && ($index < length $nick2) &&
			lc(substr( $nick1, $index, 1 )) eq lc(substr( $nick2, $index, 1 ))
	);
	
	
	return substr( $nick1, 0, $index );
}
