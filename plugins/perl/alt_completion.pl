use strict;
use warnings;
use Xchat ();
use File::Spec ();
use File::Basename qw(fileparse);

# if the last time you addressed someone was greater than this many minutes
# ago, ignore it
# this avoids having people you have talked to a long time ago coming up too
# early in the completion list
# Setting this to 0 will disable the check which is effectively the same as
# setting it to infinity
my $last_use_threshold = 10; # 10 minutes

# added to the front of a completion the same way as a suffix, only if
# the word is at the beginning of the line
my $prefix = '';

# ignore leading non-alphanumeric characters: -[\]^_`{|}
# Assuming you have the following nicks in a channel:
# [SomeNick] _SomeNick_ `SomeNick SomeNick SomeOtherNick
# when $ignore_leading_non_alnum is set to 0
#     s<tab> will cycle through SomeNick and SomeOtherNick
# when $ignore_leading_non_alnum is set to 1
#     s<tab> will cycle through [SomeNick] _SomeNick_ `SomeNick SomeNick
#     SomeOtherNick
my $ignore_leading_non_alnum = 0;

# enable path completion
my $path_completion = 1;
my $base_path = '';

# ignore the completion_amount setting and always cycle through nicks with tab
my $always_cycle = 0;

Xchat::register(
	"Tab Completion", "1.0500", "Alternative tab completion behavior"
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

sub TAB() { 0xFF09 }
sub LEFT_TAB() { 0xFE20 }

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

# used to determine if a word is the start of a path
my $path_pattern = qr{^(?:~|/|[[:alpha:]]:\\)};

sub complete {
	my ($key, $modifiers) = @{$_[0]};
	# if $_[0][0] contains the value of the key pressed
	# $_[0][1] contains modifiers
	# the value for tab is 0xFF09
	# the value for shift-tab(Left Tab) is 0xFE20
	# we don't care about other keys

	# the key must be a tab and left tab
	return Xchat::EAT_NONE unless $key == TAB || $key == LEFT_TAB;

	# if it is a tab then it must not have any modifiers
	return Xchat::EAT_NONE if $key == TAB && $modifiers & (CTRL|ALT|SHIFT);

	# loop backwards for shift+tab/left tab
	my $delta = $modifiers & SHIFT ? -1 : 1;
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

	if( $cursor_pos == $completions->{pos} ) {
		my $previous_word = $completions->{completed};
		my $new_left = $input;
		substr( $new_left, $cursor_pos ) = "";

		if( $previous_word and $new_left =~ s/(\Q$previous_word\E)$// ) {
			$word = $1;
			$word_start = length( $new_left );
			$left = $new_left;
		}
	}

	my $command_char = Xchat::get_prefs( "input_command_char" );
	# ignore commands
	if( ($word !~ m{^[${command_char}]})
		or ( $word =~ m{^[${command_char}]} and $word_start != 0 ) ) {

		if( $cursor_pos == length $input # end of input box
			# not a valid nick char
			&& $input =~ /(?<![\x41-\x5A\x61-\x7A\x30-\x39\x5B-\x60\x7B-\x7D-])$/
			&& $cursor_pos != $completions->{pos} # not continuing a completion
			&& $word !~ m{^(?:[&#/~]|[[:alpha:]]:\\)}  # not a channel or path
		) {
			# check for path completion
			unless( $path_completion and $word =~ $path_pattern ) {
				$word_start = $cursor_pos;
				$left = $input;
				$length = length $length;
				$right = "";
				$word = "";
			}
		}

		if( $word_start == 0 && $prefix && $word =~ /^\Q$prefix/ ) {
			$word =~ s/^\Q$prefix//;
		}

		my $completed; # this is going to be the "completed" word

		# for parital completions and channel names so a : isn't added
		#$completions->{skip_suffix} = ($word =~ /^[&#]/) ? 1 : 0;
		
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

		} else {

			if( $word =~ /^[&#]/ ) {
			# channel name completion
				$completions->{matches} = [ matching_channels( $word ) ];
				$completions->{skip_suffix} = 0;
			} elsif( $path_completion and $word =~ $path_pattern ) {
			# file name completion
				$completions->{matches} = [ matching_files( $word ) ];
				$completions->{skip_suffix} = 1;
			} else {
			# nick completion
				# fix $word so { equals [, ] equals }, \ equals |
				# and escape regex metacharacters
				$word =~ s/($escapes)/$escape_map{$1}/g;

				$completions->{matches} = [ matching_nicks( $word ) ];
				$completions->{skip_suffix} = 0;
			}
			$completions->{index} = 0;

		}
		$completed = $completions->{matches}[ $completions->{index} ];
		$completions->{completed} = $completed;

		my $completion_amount = Xchat::get_prefs( "completion_amount" );
		
		# don't cycle if the number of possible completions is greater than
		# completion_amount
		if(
			!$always_cycle && (
			@{$completions->{matches}} > $completion_amount
			&& @{$completions->{matches}} != 1 )
		) {
			# don't print if we tabbed in the beginning and the list of possible
			# completions includes all nicks in the channel
			my $context_type = Xchat::context_info->{type};
			if( $context_type != 2 # not a channel
				or @{$completions->{matches}} < Xchat::get_list("users")
			) {
				Xchat::print( join " ", @{$completions->{matches}}, "\n" );
			}
			
			$completed = lcs( $completions->{matches} );
			$completions->{skip_suffix} = 1;
		}
		
		if( $completed ) {
			
			if( $word_start == 0 && !$completions->{skip_suffix} ) {
				# at the start of the line append completion suffix
				Xchat::command( "settext $prefix$completed$suffix$right");
				$completions->{pos} = length( "$prefix$completed$suffix" );
			} else {
				Xchat::command( "settext $left$completed$right" );
				$completions->{pos} = length( "$left$completed" );
			}
			
			Xchat::command( "setcursor $completions->{pos}" );
		}

=begin
# debugging stuff
		local $, = " ";
		my $input_length = length $input;
		Xchat::print [
			qq{input[$input]},
			qq{input_length[$input_length]},
			qq{cursor[$cursor_pos]},
			qq{start[$word_start]},
			qq{length[$length]},
			qq{left[$left]},
			qq{word[$word]}, qq{right[$right]},
			qq{completed[}. ($completed||""). qq{]},
			qq{pos[$completions->{pos}]},
		];
		use Data::Dumper;
		local $Data::Dumper::Indent = 0;
		Xchat::print Dumper $completions->{matches};
=cut

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
			$chan =~ /^$word/i && $_->{id} == $conn_id ?
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
	my $word_re = shift;

	# for use in compare_nicks()
	our ($my_nick, $selections, $now);
	local $my_nick = Xchat::get_info( "nick" );
	local $selections = $selected{ Xchat::get_context() };
	local $now = time;

	my $pattern = $ignore_leading_non_alnum ?
		qr/^[\-\[\]^_`{|}\\]*$word_re/i : qr/^$word_re/i;
	return
		map { $_->{nick} }
		sort compare_nicks grep {
			$_->{nick} =~ $pattern;
		} Xchat::get_list( "users" )

}

sub max {
	return unless @_;
	my $max = shift;
	for(@_) {
		$max = $_ if $_ > $max;
	}
	return $max;
}

sub compare_times {
	# package variables set in matching_nicks()
	our $selections;
	our $now;
	
	for my $nick ( $a->{nick}, $b->{nick} ) {
		# turn off the warnings that get generated from users who have yet
		# to speak since the script was loaded
		no warnings "uninitialized";

		if( $last_use_threshold
			&& (( $now - $selections->{$nick}) > ($last_use_threshold * 60)) ) {
			delete $selections->{ $nick }
		}
	}
	my $a_time = $selections->{ $a->{nick} } || 0 ;
	my $b_time = $selections->{ $b->{nick} } || 0 ;
	
	if( $a_time || $b_time ) {
		return $b_time <=> $a_time;
	} elsif( !$a_time && !$b_time ) {
		return $b->{lasttalk} <=> $a->{lasttalk};
	}

}

sub compare_nicks {
	# more package variables, value set in matching_nicks()
	our $my_nick;

	# our own nick is always last, then ordered by the people we spoke to most
	# recently and the people who were speaking most recently
	return 
		$a->{nick} eq $my_nick ? 1 :
		$b->{nick} eq $my_nick ? -1 :
		compare_times()
		|| Xchat::nickcmp( $a->{nick}, $b->{nick} );

#		$selections->{ $b->{nick} } <=> $selections->{ $a->{nick} }
#		||	$b->{lasttalk} <=> $a->{lasttalk}

}

sub matching_files {
	my $word = shift;

	my ($file, $input_dir) = fileparse( $word );

	my $dir = expand_tilde( $input_dir );

	if( opendir my $dir_handle, $dir ) {
		my @files;

		if( $file ) {
			@files = grep {
				#Xchat::print( $_ );
				/^\Q$file/ } readdir $dir_handle;
		} else {
			@files = readdir $dir_handle;
		}

		return map {
			File::Spec->catfile( $input_dir, $_ );
		} sort
		grep { !/^[.]{1,2}$/ } @files;
	} else {
		return ();
	}
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
	return Xchat::EAT_NONE;
}

# keep track of the last time a message was addressed to someone
# a message is considered addressed to someone if their nick is used followed
# by the completion suffix
sub track_selected {
	my $input = $_[1][0];
	return Xchat::EAT_NONE unless defined $input;

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

sub expand_tilde {
	my $file = shift;

	$file =~ s/^~/home_dir()/e;
	return $file;
}

sub home_dir {
	return $base_path if $base_path;

	if ( $^O eq "MSWin32" ) {
		return $ENV{USERPROFILE};
	} else {
		return ((getpwuid($>))[7] ||  $ENV{HOME} || $ENV{LOGDIR});
	}
}
