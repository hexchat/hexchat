BEGIN {
	$INC{'Xchat.pm'} = 'DUMMY';
}

$SIG{__WARN__} = sub {
	my $message = shift @_;
	my ($package) = caller;
	
	# redirect Gtk/Glib errors and warnings back to STDERR
	my $message_levels =	qr/ERROR|CRITICAL|WARNING|MESSAGE|INFO|DEBUG/i;
	if( $message =~ /^(?:Gtk|GLib|Gdk)(?:-\w+)?-$message_levels/i ) {
		print STDERR $message;
	} else {

		if( defined &Xchat::Internal::print ) {
			Xchat::print( $message );
		} else {
			warn $message;
		}
	}
};

use File::Spec ();
use File::Basename ();
use File::Glob ();
use List::Util ();
use Symbol();
use Time::HiRes ();
use Carp ();

package Xchat;
use base qw(Exporter);
use strict;
use warnings;

sub PRI_HIGHEST ();
sub PRI_HIGH ();
sub PRI_NORM ();
sub PRI_LOW ();
sub PRI_LOWEST ();

sub EAT_NONE ();
sub EAT_XCHAT ();
sub EAT_PLUIN ();
sub EAT_ALL ();

sub KEEP ();
sub REMOVE ();
sub FD_READ ();
sub FD_WRITE ();
sub FD_EXCEPTION ();
sub FD_NOTSOCKET ();

sub get_context;
sub Xchat::Internal::context_info;
sub Xchat::Internal::print;

our %EXPORT_TAGS = (
	constants => [
		qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST), # priorities
		qw(EAT_NONE EAT_XCHAT EAT_PLUGIN EAT_ALL), # callback return values
		qw(FD_READ FD_WRITE FD_EXCEPTION FD_NOTSOCKET), # fd flags
		qw(KEEP REMOVE), # timers
	],
	hooks => [
		qw(hook_server hook_command hook_print hook_timer hook_fd unhook),
	],
	util => [
		qw(register nickcmp strip_code send_modes), # misc
		qw(print prnt printf prntf command commandf emit_print), # output
		qw(find_context get_context set_context), # context
		qw(get_info get_prefs get_list context_info user_info), # input
	],
);

$EXPORT_TAGS{all} = [ map { @{$_} } @EXPORT_TAGS{qw(constants hooks util)}];
our @EXPORT = @{$EXPORT_TAGS{constants}};
our @EXPORT_OK = @{$EXPORT_TAGS{all}};

sub register {
	my $package = Xchat::Embed::find_pkg();
	my $pkg_info = Xchat::Embed::pkg_info( $package );
	my $filename = $pkg_info->{filename};
	my ($name, $version, $description, $callback) = @_;
	
	if( defined $pkg_info->{gui_entry} ) {
		Xchat::print( "Xchat::register called more than once in "
			. $pkg_info->{filename} );
		return ();
	}
	
	$description = "" unless defined $description;
	$pkg_info->{shutdown} = $callback;
	unless( $name && $name =~ /[[:print:]\w]/ ) {
		$name = "Not supplied";
	}
	unless( $version && $version =~ /\d+(?:\.\d+)?/ ) {
		$version = "NaN";
	}
	$pkg_info->{gui_entry} =
		Xchat::Internal::register( $name, $version, $description, $filename );
	# keep with old behavior
	return ();
}

sub _process_hook_options {
	my ($options, $keys, $store) = @_;

	unless( @$keys == @$store ) {
		die 'Number of keys must match the size of the store';
	}

	my @results;

	if( ref( $options ) eq 'HASH' ) {
		for my $index ( 0 .. @$keys - 1 ) {
			my $key = $keys->[$index];
			if( exists( $options->{ $key } ) && defined( $options->{ $key } ) ) {
				${$store->[$index]} = $options->{ $key };
			}
		}
	}

}

sub hook_server {
	return undef unless @_ >= 2;
	my $message = shift;
	my $callback = shift;
	my $options = shift;
	my $package = Xchat::Embed::find_pkg();
	
	$callback = Xchat::Embed::fix_callback( $package, $callback );
	
	my ($priority, $data) = ( Xchat::PRI_NORM, undef );
	_process_hook_options(
		$options,
		[qw(priority data)],
		[\($priority, $data)],
	);
	
	my $pkg_info = Xchat::Embed::pkg_info( $package );
	my $hook = Xchat::Internal::hook_server(
		$message, $priority, $callback, $data
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_command {
	return undef unless @_ >= 2;
	my $command = shift;
	my $callback = shift;
	my $options = shift;
	my $package = Xchat::Embed::find_pkg();

	$callback = Xchat::Embed::fix_callback( $package, $callback );
	
	my ($priority, $help_text, $data) = ( Xchat::PRI_NORM, undef, undef );
	_process_hook_options(
		$options,
		[qw(priority help_text data)],
		[\($priority, $help_text, $data)],
	);
	
	my $pkg_info = Xchat::Embed::pkg_info( $package );
	my $hook = Xchat::Internal::hook_command(
		$command, $priority, $callback, $help_text, $data
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_print {
	return undef unless @_ >= 2;
	my $event = shift;
	my $callback = shift;
	my $options = shift;
	my $package = Xchat::Embed::find_pkg();

	$callback = Xchat::Embed::fix_callback( $package, $callback );
	
	my ($priority, $run_after, $filter, $data) = ( Xchat::PRI_NORM, 0, 0, undef );
	_process_hook_options(
		$options,
		[qw(priority run_after_event filter data)],
		[\($priority, $run_after, $filter, $data)],
	);
	
	if( $run_after and $filter ) {
		Carp::carp( "Xchat::hook_print's run_after_event and filter options are mutually exclusive, you can only use of them at a time per hook" );
		return;
	}

	if( $run_after ) {
		my $cb = $callback;
		$callback = sub {
			my @args = @_;
			hook_timer( 0, sub {
				$cb->( @args );

				if( ref $run_after eq 'CODE' ) {
					$run_after->( @args );
				}
				return REMOVE;
			});
			return EAT_NONE;
		};
	}

	if( $filter ) {
		my $cb = $callback;
		$callback = sub {
			my @args = @{$_[0]};
			my $last_arg = @args - 1;

			my @new = $cb->( \@args, $_[1], $event );

			# a filter can either return the new results or it can modify
			# @_ in place. 
			if( @new ) {
				emit_print( $event, @new[ 0 .. $last_arg ] );
				return EAT_ALL;
			} elsif(
				join( "\0", @{$_[0]} ) ne join( "\0", @args[ 0 .. $last_arg ] )
			) {
				emit_print( $event, @args[ 0 .. $last_arg ] );
				return EAT_ALL;
			}

			return EAT_NONE;
		};

	}

	my $pkg_info = Xchat::Embed::pkg_info( $package );
	my $hook = Xchat::Internal::hook_print(
		$event, $priority, $callback, $data
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_timer {
	return undef unless @_ >= 2;
	my ($timeout, $callback, $data) = @_;
	my $package = Xchat::Embed::find_pkg();

	$callback = Xchat::Embed::fix_callback( $package, $callback );

	if(
		ref( $data ) eq 'HASH' && exists( $data->{data} )
		&& defined( $data->{data} )
	) {
		$data = $data->{data};
	}
	
	my $pkg_info = Xchat::Embed::pkg_info( $package );
	my $hook = Xchat::Internal::hook_timer( $timeout, $callback, $data, $package );
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_fd {
	return undef unless @_ >= 2;
	my ($fd, $callback, $options) = @_;
	return undef unless defined $fd && defined $callback;

	my $fileno = fileno $fd;
	return undef unless defined $fileno; # no underlying fd for this handle
	
	my ($package) = Xchat::Embed::find_pkg();
	$callback = Xchat::Embed::fix_callback( $package, $callback );
	
	my ($flags, $data) = (Xchat::FD_READ, undef);
	_process_hook_options(
		$options,
		[qw(flags data)],
		[\($flags, $data)],
	);
	
	my $cb = sub {
		my $userdata = shift;
		return $userdata->{CB}->(
			$userdata->{FD}, $userdata->{FLAGS}, $userdata->{DATA},
		);
	};
	
	my $pkg_info = Xchat::Embed::pkg_info( $package );
	my $hook = Xchat::Internal::hook_fd(
		$fileno, $cb, $flags, {
			DATA => $data, FD => $fd, CB => $callback, FLAGS => $flags,
		}
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub unhook {
	my $hook = shift @_;
	my $package = shift @_;
	($package) = caller unless $package;
	my $pkg_info = Xchat::Embed::pkg_info( $package );

	if( defined( $hook )
		&& $hook =~ /^\d+$/
		&& grep { $_ == $hook } @{$pkg_info->{hooks}} ) {
		$pkg_info->{hooks} = [grep { $_ != $hook } @{$pkg_info->{hooks}}];
		return Xchat::Internal::unhook( $hook );
	}
	return ();
}

sub _do_for_each {
	my ($cb, $channels, $servers) = @_;

	# not specifying any channels or servers is not the same as specifying
	# undef for both
	# - not specifying either results in calling the callback inthe current ctx
	# - specifying undef for for both results in calling the callback in the
	#   front/currently selected tab
	if( @_ == 3 && !($channels || $servers) ) { 
		$channels = [ undef ];
		$servers = [ undef ];
	} elsif( !($channels || $servers) ) {
		$cb->();
		return 1;
	}

	$channels = [ $channels ] unless ref( $channels ) eq 'ARRAY';

	if( $servers ) {
		$servers = [ $servers ] unless ref( $servers ) eq 'ARRAY';
	} else {
		$servers = [ undef ];
	}

	my $num_done = 0;
	my $old_ctx = Xchat::get_context();
	for my $server ( @$servers ) {
		for my $channel ( @$channels ) {
			if( Xchat::set_context( $channel, $server ) ) {
				$cb->();
				$num_done++
			}
		}
	}
	Xchat::set_context( $old_ctx );
	return $num_done;
}

sub print {
	my $text = shift @_;
	return "" unless defined $text;
	if( ref( $text ) eq 'ARRAY' ) {
		if( $, ) {
			$text = join $, , @$text;
		} else {
			$text = join "", @$text;
		}
	}
	
	return _do_for_each(
		sub { Xchat::Internal::print( $text ); },
		@_
	);
}

sub printf {
	my $format = shift;
	Xchat::print( sprintf( $format, @_ ) );
}

# make Xchat::prnt() and Xchat::prntf() as aliases for Xchat::print() and 
# Xchat::printf(), mainly useful when these functions are exported
sub prnt {
	goto &Xchat::print;
}

sub prntf {
	goto &Xchat::printf;
}

sub command {
	my $command = shift;
	return "" unless defined $command;
	my @commands;
	
	if( ref( $command ) eq 'ARRAY' ) {
		@commands = @$command;
	} else {
		@commands = ($command);
	}
	
	return _do_for_each(
		sub { Xchat::Internal::command( $_ ) foreach @commands },
		@_
	);
}

sub commandf {
	my $format = shift;
	Xchat::command( sprintf( $format, @_ ) );
}

sub set_context {
	my $context;
	if( @_ == 2 ) {
		my ($channel, $server) = @_;
		$context = Xchat::find_context( $channel, $server );
	} elsif( @_ == 1 ) {
		if( defined $_[0] && $_[0] =~ /^\d+$/ ) {
			$context = $_[0];
		} else {
			$context = Xchat::find_context( $_[0] );
		}
	} elsif( @_ == 0 ) {
		$context = Xchat::find_context();
	}
	return $context ? Xchat::Internal::set_context( $context ) : 0;
}

sub get_info {
	my $id = shift;
	my $info;
	
	if( defined( $id ) ) {
		if( grep { $id eq $_ } qw(state_cursor id) ) {
			$info = Xchat::get_prefs( $id );
		} else {
			$info = Xchat::Internal::get_info( $id );
		}
	}
	return $info;
}

sub user_info {
	my $nick = Xchat::strip_code(shift @_ || Xchat::get_info( "nick" ));
	my $user;
	for (Xchat::get_list( "users" ) ) {
		if ( Xchat::nickcmp( $_->{nick}, $nick ) == 0 ) {
			$user = $_;
			last;
		}
	}
	return $user;
}

sub context_info {
	my $ctx = shift @_ || Xchat::get_context;
	my $old_ctx = Xchat::get_context;
	my @fields = (
		qw(away channel charset host id inputbox libdirfs modes network),
		qw(nick nickserv server topic version win_ptr win_status),
		qw(xchatdir xchatdirfs state_cursor),
	);

	if( Xchat::set_context( $ctx ) ) {
		my %info;
		for my $field ( @fields ) {
			$info{$field} = Xchat::get_info( $field );
		}
		
		my $ctx_info = Xchat::Internal::context_info;
		@info{keys %$ctx_info} = values %$ctx_info;
		
		Xchat::set_context( $old_ctx );
		return %info if wantarray;
		return \%info;
	} else {
		return undef;
	}
}

sub get_list {
	unless( grep { $_[0] eq $_ } qw(channels dcc ignore notify users networks) ) {
		Carp::carp( "'$_[0]' does not appear to be a valid list name" );
	}
	if( $_[0] eq 'networks' ) {
		return Xchat::List::Network->get();
	} else {
		return Xchat::Internal::get_list( $_[0] );
	}
}

sub strip_code {
	my $pattern = qr<
		\cB| #Bold
		\cC\d{0,2}(?:,\d{1,2})?| #Color
		\e\[(?:\d{1,2}(?:;\d{1,2})*)?m| # ANSI color code
		\cG| #Beep
		\cO| #Reset
		\cV| #Reverse
		\c_  #Underline
	>x;
		
	if( defined wantarray ) {
		my $msg = shift;
		$msg =~ s/$pattern//g;
		return $msg;
	} else {
		$_[0] =~ s/$pattern//g if defined $_[0];
	}
}

1
