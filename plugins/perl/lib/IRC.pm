
package IRC;
sub IRC::register {
  my ($script_name, $version, $callback) = @_;
  my $package = caller;
  $callback = Xchat::Embed::fix_callback( $package, undef, $callback) if $callback;
  Xchat::register( $script_name, $version, undef, $callback );
}


sub IRC::add_command_handler {
  my ($command, $callback) = @_;
  my $package = caller;

  $callback = Xchat::Embed::fix_callback( $package, undef, $callback );

  # starting index for word_eol array
  # this is for compatibility with '' as the command
  my $start_index = $command ? 1 : 0;

  Xchat::hook_command( $command,
		       sub {
			 no strict 'refs';
			 return &{$callback}($_[1][$start_index]);
		       }
		     );
  return;
}

sub IRC::add_message_handler {
  my ($message, $callback) = @_;
  my $package = caller;
  $callback = Xchat::Embed::fix_callback( $package, undef, $callback );

  Xchat::hook_server( $message,
		      sub {
			no strict 'refs';
			return &{$callback}( $_[1][0] );
		      }
		    );
  return;
}

sub IRC::add_print_handler {
  my ($event, $callback) = @_;
  my $package = caller;
  $callback = Xchat::Embed::fix_callback( $package, undef, $callback );
  Xchat::hook_print( $event,
		     sub {
		       my @word = @{$_[0]};
		       no strict 'refs';
		       return &{$callback}( join( ' ', @word[0..3] ), @word );
		     }
		   );
  return;
}

sub IRC::add_timeout_handler {
  my ($timeout, $callback) = @_;
  my $package = caller;
  $callback = Xchat::Embed::fix_callback( $package, undef, $callback );
  Xchat::hook_timer( $timeout,
		     sub {
		       no strict 'refs';
		       &{$callback};
		       return 0;
		     }
		   );
  return;
}

sub IRC::command {
  my $command = shift;
  if( $command =~ m{^/} ) {
    $command =~ s{^/}{};
    Xchat::command( $command );
  } else {
    Xchat::command( qq[say $command] );
  }
}

sub IRC::command_with_channel {
  my ($command, $channel, $server) = @_;
  my $old_ctx = Xchat::get_context;
  my $ctx = Xchat::find_context( $channel, $server );

  if( $ctx ) {
    Xchat::set_context( $ctx );
    IRC::command( $command );
    Xchat::set_context( $ctx );
  }
}

sub IRC::command_with_server {
  my ($command, $server) = @_;
  my $old_ctx = Xchat::get_context;
  my $ctx = Xchat::find_context( undef, $server );

  if( $ctx ) {
    Xchat::set_context( $ctx );
    IRC::command( $command );
    Xchat::set_context( $ctx );
  }
}

sub IRC::dcc_list {
  my @dccs;
  for my $dcc ( Xchat::get_list( 'dcc' ) ) {
    push @dccs, $dcc->{nick};
    push @dccs, $dcc->{file} ? $dcc->{file} : '';
    push @dccs, @{$dcc}{qw(type status cps size)};
    push @dccs, $dcc->{type} == 0 ? $dcc->{pos} : $dcc->{resume};
    push @dccs, $dcc->{address32};
    push @dccs, $dcc->{destfile} ? $dcc->{destfile} : '';
  }
  return @dccs;
}

sub IRC::channel_list {
  my @channels;
  for my $channel ( Xchat::get_list( 'channels' ) ) {
    push @channels, @{$channel}{qw(channel server)},
      Xchat::context_info( $channel->{context} )->{nick};
  }
  return @channels;
}

sub IRC::get_info {
  my $id = shift;
  my @ids = qw(version nick channel server configdir xchatdir away network host topic);
  
  if( $id >= 0 && $id <= 8 && $id != 5 ) {
    my $info = Xchat::get_info($ids[$id]);
    return defined $info ? $info : '';
  } else {
    if( $id == 5 ) {
      return Xchat::get_info( 'away' ) ? 1 : 0;
    } else {
      return 'Error2';
    }
  }
}

sub IRC::get_prefs {
  return 'Unknown variable' unless defined $_[0];
  my $result = Xchat::get_prefs(shift);
  return defined $result ? $result : 'Unknown variable';
}

sub IRC::ignore_list {
  my @ignores;
  for my $ignore ( Xchat::get_list( 'ignore' ) ) {
    push @ignores, $ignore->{mask};
    my $flags = $ignore->{flags};
    push @ignores, $flags & 1, $flags & 2, $flags & 4, $flags & 8, $flags & 16,
      $flags & 32, ':';
  }
  return @ignores;
}

sub IRC::print {
  Xchat::print( $_ ) for @_;
  return;
}

sub IRC::print_with_channel {
  Xchat::print( @_ );
}

sub IRC::send_raw {
  Xchat::commandf( qq[quote %s], shift );
}

sub IRC::server_list {
  my @servers;
  for my $channel ( Xchat::get_list( 'channels' ) ) {
    push @servers, $channel->{server} if $channel->{server};
  }
  return @servers;
}

sub IRC::user_info {
  my $user;
  if( @_ > 0 ) {
    $user = Xchat::user_info( shift );
  } else {
    $user = Xchat::user_info();
  }

  my @info;
  if( $user ) {
    push @info, $user->{nick};
    if( $user->{host} ) {
      push @info, $user->{host};
    } else {
      push @info, 'FETCHING';
    }
    push @info, $user->{prefix} eq '@' ? 1 : 0;
    push @info, $user->{prefix} eq '+' ? 1 : 0;
  }
  return @info;
}

sub IRC::user_list {
  my ($channel, $server) = @_;
  my $ctx = Xchat::find_context( $channel, $server );
  my $old_ctx = Xchat::get_context;

  if( $ctx ) {
    Xchat::set_context( $ctx );
    my @users;
    for my $user ( Xchat::get_list( 'users' ) ) {
      push @users, $user->{nick};
      if( $user->{host} ) {
	push @users, $user->{host};
      } else {
	push @users, 'FETCHING';
      }
      push @users, $user->{prefix} eq '@' ? 1 : 0;
      push @users, $user->{prefix} eq '+' ? 1 : 0;
      push @users, ':';
    }
    Xchat::set_context( $old_ctx );
    return @users;
  } else {
    return;
  }
}

sub IRC::user_list_short {
  my ($channel, $server) = @_;
  my $ctx = Xchat::find_context( $channel, $server );
  my $old_ctx = Xchat::get_context;

  if( $ctx ) {
    Xchat::set_context( $ctx );
    my @users;
    for my $user ( Xchat::get_list( 'users' ) ) {
      my $nick = $user->{nick};
      my $host = $user->{host} || 'FETCHING';
      push @users, $nick, $host;
    }
    Xchat::set_context( $old_ctx );
    return @users;
  } else {
    return;
  }

}

sub IRC::add_user_list {}
sub IRC::sub_user_list {}
sub IRC::clear_user_list {}
sub IRC::notify_list {}
sub IRC::perl_script_list {}

1
