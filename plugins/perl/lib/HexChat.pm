$SIG{__WARN__} = sub {
	my $message = shift @_;
	my ($package) = caller;
	
	# redirect Gtk/Glib errors and warnings back to STDERR
	my $message_levels =	qr/ERROR|CRITICAL|WARNING|MESSAGE|INFO|DEBUG/i;
	if( $message =~ /^(?:Gtk|GLib|Gdk)(?:-\w+)?-$message_levels/i ) {
		print STDERR $message;
	} else {

		if( defined &HexChat::Internal::print ) {
			HexChat::print( $message );
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

package HexChat;
use base qw(Exporter);
use strict;
use warnings;

sub PRI_HIGHEST ();
sub PRI_HIGH ();
sub PRI_NORM ();
sub PRI_LOW ();
sub PRI_LOWEST ();

sub EAT_NONE ();
sub EAT_HEXCHAT ();
sub EAT_PLUGIN ();
sub EAT_ALL ();

sub KEEP ();
sub REMOVE ();
sub FD_READ ();
sub FD_WRITE ();
sub FD_EXCEPTION ();
sub FD_NOTSOCKET ();

sub get_context;
sub HexChat::Internal::context_info;
sub HexChat::Internal::print;

#keep compatibility with Xchat scripts
sub EAT_XCHAT ();
BEGIN {
	*Xchat:: = *HexChat::;
}

our %EXPORT_TAGS = (
	constants => [
		qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST), # priorities
		qw(EAT_NONE EAT_HEXCHAT EAT_XCHAT EAT_PLUGIN EAT_ALL), # callback return values
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
		qw(plugin_pref_set plugin_pref_get plugin_pref_delete plugin_pref_list), #settings
	],
);

$EXPORT_TAGS{all} = [ map { @{$_} } @EXPORT_TAGS{qw(constants hooks util)}];
our @EXPORT = @{$EXPORT_TAGS{constants}};
our @EXPORT_OK = @{$EXPORT_TAGS{all}};

sub register {
	my ($package, $calling_package) = HexChat::Embed::find_pkg();
	my $pkg_info = HexChat::Embed::pkg_info( $package );
	my $filename = $pkg_info->{filename};
	my ($name, $version, $description, $callback) = @_;
	
	if( defined $pkg_info->{gui_entry} ) {
		HexChat::print( "HexChat::register called more than once in "
			. $pkg_info->{filename} );
		return ();
	}
	
	$description = "" unless defined $description;
	if( $callback ) {
		$callback = HexChat::Embed::fix_callback(
			$package, $calling_package, $callback
		);
	}
	$pkg_info->{shutdown} = $callback;
	unless( $name && $name =~ /[[:print:]\w]/ ) {
		$name = "Not supplied";
	}
	unless( $version && $version =~ /\d+(?:\.\d+)?/ ) {
		$version = "NaN";
	}
	$pkg_info->{gui_entry} =
		HexChat::Internal::register( $name, $version, $description, $filename );
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
	my ($package, $calling_package) = HexChat::Embed::find_pkg();
	
	$callback = HexChat::Embed::fix_callback(
		$package, $calling_package, $callback
	);
	
	my ($priority, $data) = ( HexChat::PRI_NORM, undef );
	_process_hook_options(
		$options,
		[qw(priority data)],
		[\($priority, $data)],
	);
	
	my $pkg_info = HexChat::Embed::pkg_info( $package );
	my $hook = HexChat::Internal::hook_server(
		$message, $priority, $callback, $data, $package
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_command {
	return undef unless @_ >= 2;
	my $command = shift;
	my $callback = shift;
	my $options = shift;
	my ($package, $calling_package) = HexChat::Embed::find_pkg();

	$callback = HexChat::Embed::fix_callback(
		$package, $calling_package, $callback
	);
	
	my ($priority, $help_text, $data) = ( HexChat::PRI_NORM, undef, undef );
	_process_hook_options(
		$options,
		[qw(priority help_text data)],
		[\($priority, $help_text, $data)],
	);
	
	my $pkg_info = HexChat::Embed::pkg_info( $package );
	my $hook = HexChat::Internal::hook_command(
		$command, $priority, $callback, $help_text, $data, $package
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_print {
	return undef unless @_ >= 2;
	my $event = shift;
	my $callback = shift;
	my $options = shift;
	my ($package, $calling_package) = HexChat::Embed::find_pkg();

	$callback = HexChat::Embed::fix_callback(
		$package, $calling_package, $callback
	);
	
	my ($priority, $run_after, $filter, $data) = ( HexChat::PRI_NORM, 0, 0, undef );
	_process_hook_options(
		$options,
		[qw(priority run_after_event filter data)],
		[\($priority, $run_after, $filter, $data)],
	);
	
	if( $run_after and $filter ) {
		Carp::carp( "HexChat::hook_print's run_after_event and filter options are mutually exclusive, you can only use of them at a time per hook" );
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
			my $event_data = $_[1];
			my $event_name = $event;
			my $last_arg = @args - 1;

			my @new = $cb->( \@args, $event_data, $event_name );

			# allow changing event by returning the new value
			if( @new > @args ) {
				$event_name = pop @new;
			}

			# a filter can either return the new results or it can modify
			# @_ in place. 
			if( @new == @args ) {
				emit_print( $event_name, @new[ 0 .. $last_arg ] );
				return EAT_ALL;
			} elsif(
				join( "\0", @{$_[0]} ) ne join( "\0", @args[ 0 .. $last_arg ] )
			) {
				emit_print( $event_name, @args[ 0 .. $last_arg ] );
				return EAT_ALL;
			}

			return EAT_NONE;
		};

	}

	my $pkg_info = HexChat::Embed::pkg_info( $package );
	my $hook = HexChat::Internal::hook_print(
		$event, $priority, $callback, $data, $package
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_timer {
	return undef unless @_ >= 2;
	my ($timeout, $callback, $data) = @_;
	my ($package, $calling_package) = HexChat::Embed::find_pkg();

	$callback = HexChat::Embed::fix_callback(
		$package, $calling_package, $callback
	);

	if(
		ref( $data ) eq 'HASH' && exists( $data->{data} )
		&& defined( $data->{data} )
	) {
		$data = $data->{data};
	}
	
	my $pkg_info = HexChat::Embed::pkg_info( $package );
	my $hook = HexChat::Internal::hook_timer( $timeout, $callback, $data, $package );
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub hook_fd {
	return undef unless @_ >= 2;
	my ($fd, $callback, $options) = @_;
	return undef unless defined $fd && defined $callback;

	my $fileno = fileno $fd;
	return undef unless defined $fileno; # no underlying fd for this handle
	
	my ($package, $calling_package) = HexChat::Embed::find_pkg();
	$callback = HexChat::Embed::fix_callback(
		$package, $calling_package, $callback
	);
	
	my ($flags, $data) = (HexChat::FD_READ, undef);
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
	
	my $pkg_info = HexChat::Embed::pkg_info( $package );
	my $hook = HexChat::Internal::hook_fd(
		$fileno, $cb, $flags, {
			DATA => $data, FD => $fd, CB => $callback, FLAGS => $flags,
		},
		$package
	);
	push @{$pkg_info->{hooks}}, $hook if defined $hook;
	return $hook;
}

sub unhook {
	my $hook = shift @_;
	my $package = shift @_;
	($package) = caller unless $package;
	my $pkg_info = HexChat::Embed::pkg_info( $package );

	if( defined( $hook )
		&& $hook =~ /^\d+$/
		&& grep { $_ == $hook } @{$pkg_info->{hooks}} ) {
		$pkg_info->{hooks} = [grep { $_ != $hook } @{$pkg_info->{hooks}}];
		return HexChat::Internal::unhook( $hook );
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
	my $old_ctx = HexChat::get_context();
	for my $server ( @$servers ) {
		for my $channel ( @$channels ) {
			if( HexChat::set_context( $channel, $server ) ) {
				$cb->();
				$num_done++
			}
		}
	}
	HexChat::set_context( $old_ctx );
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
		sub { HexChat::Internal::print( $text ); },
		@_
	);
}

sub printf {
	my $format = shift;
	HexChat::print( sprintf( $format, @_ ) );
}

# make HexChat::prnt() and HexChat::prntf() as aliases for HexChat::print() and 
# HexChat::printf(), mainly useful when these functions are exported
sub prnt {
	goto &HexChat::print;
}

sub prntf {
	goto &HexChat::printf;
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
		sub { HexChat::Internal::command( $_ ) foreach @commands },
		@_
	);
}

sub commandf {
	my $format = shift;
	HexChat::command( sprintf( $format, @_ ) );
}

sub plugin_pref_set {
	my $setting = shift // return 0;
	my $value   = shift // return 0;

	return HexChat::Internal::plugin_pref_set($setting, $value);
}

sub plugin_pref_get {
	my $setting = shift // return 0;

	return HexChat::Internal::plugin_pref_get($setting);
}

sub plugin_pref_delete {
	my $setting = shift // return 0;

	return HexChat::Internal::plugin_pref_delete($setting);
}

sub plugin_pref_list {
	my %list = HexChat::Internal::plugin_pref_list();

	return \%list;
}

sub set_context {
	my $context;
	if( @_ == 2 ) {
		my ($channel, $server) = @_;
		$context = HexChat::find_context( $channel, $server );
	} elsif( @_ == 1 ) {
		if( defined $_[0] && $_[0] =~ /^\d+$/ ) {
			$context = $_[0];
		} else {
			$context = HexChat::find_context( $_[0] );
		}
	} elsif( @_ == 0 ) {
		$context = HexChat::find_context();
	}
	return $context ? HexChat::Internal::set_context( $context ) : 0;
}

sub get_info {
	my $id = shift;
	my $info;
	
	if( defined( $id ) ) {
		if( grep { $id eq $_ } qw(state_cursor id) ) {
			$info = HexChat::get_prefs( $id );
		} else {
			$info = HexChat::Internal::get_info( $id );
		}
	}
	return $info;
}

sub user_info {
	my $nick = HexChat::strip_code(shift @_ || HexChat::get_info( "nick" ));
	my $user;
	for (HexChat::get_list( "users" ) ) {
		if ( HexChat::nickcmp( $_->{nick}, $nick ) == 0 ) {
			$user = $_;
			last;
		}
	}
	return $user;
}

sub context_info {
	my $ctx = shift @_ || HexChat::get_context;
	my $old_ctx = HexChat::get_context;
	my @fields = (
		qw(away channel charset host id inputbox libdirfs modes network),
		qw(nick nickserv server topic version win_ptr win_status),
		qw(configdir xchatdir xchatdirfs state_cursor),
	);

	if( HexChat::set_context( $ctx ) ) {
		my %info;
		for my $field ( @fields ) {
			$info{$field} = HexChat::get_info( $field );
		}
		
		my $ctx_info = HexChat::Internal::context_info;
		@info{keys %$ctx_info} = values %$ctx_info;
		
		HexChat::set_context( $old_ctx );
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
		return HexChat::List::Network->get();
	} else {
		return HexChat::Internal::get_list( $_[0] );
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
