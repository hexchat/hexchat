# line 1 "Xchat.pm"
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

{
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
	
	my ($priority, $data) = ( Xchat::PRI_NORM, undef );
	_process_hook_options(
		$options,
		[qw(priority data)],
		[\($priority, $data)],
	);
	
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

} # end of Xchat package

{
package Xchat::Embed;
use strict;
use warnings;
# list of loaded scripts keyed by their package names
our %scripts;

sub load {
	my $file = expand_homedir( shift @_ );
	my $package = file2pkg( $file );
	
	if( exists $scripts{$package} ) {
		my $pkg_info = pkg_info( $package );
		my $filename = File::Basename::basename( $pkg_info->{filename} );
		Xchat::printf(
			qq{'%s' already loaded from '%s'.\n},
			$filename, $pkg_info->{filename}
		);
		Xchat::print(
			'If this is a different script then it rename and try '.
			'loading it again.'
		);
		return 2;
	}
	
	if( open my $source_handle, $file ) {
		my $source = do {local $/; <$source_handle>};
		close $source_handle;
		# we shouldn't care about things after __END__
		$source =~ s/^__END__.*//ms;
		
		if(
			my @replacements = $source =~
				m/^\s*package ((?:[^\W:]+(?:::)?)+)\s*?;/mg
		) {
			
			if ( @replacements > 1 ) {
				Xchat::print(
					"Too many package defintions, only 1 is allowed\n"
				);
				return 1;
			}
			
			my $original_package = shift @replacements;
			
			# remove original package declaration
			$source =~ s/^(package $original_package\s*;)/#$1/m;
			
			# fixes things up for code calling subs with fully qualified names
			$source =~ s/${original_package}:://g;
		}
		
		# this must come before the eval or the filename will not be found in
		# Xchat::register
		$scripts{$package}{filename} = $file;
		$scripts{$package}{loaded_at} = Time::HiRes::time();

		my $full_path = File::Spec->rel2abs( $file );
		$source =~ s/^/#line 1 "$full_path"\n\x7Bpackage $package;/;

		# make sure we add the closing } even if the last line is a comment
		if( $source =~ /^#.*\Z/m ) {
			$source =~ s/^(?=#.*\Z)/\x7D/m;
		} else {
			$source =~ s/\Z/\x7D/;
		}

		_do_eval( $source );

		unless( exists $scripts{$package}{gui_entry} ) {
			$scripts{$package}{gui_entry} =
				Xchat::Internal::register(
					"", "unknown", "", $file
				);
		}
		
		if( $@ ) {
			# something went wrong
			$@ =~ s/\(eval \d+\)/$file/g;
			Xchat::print( "Error loading '$file':\n$@\n" );
			# make sure the script list doesn't contain false information
			unload( $scripts{$package}{filename} );
			return 1;
		}
	} else {
		Xchat::print( "Error opening '$file': $!\n" );
		return 2;
	}

	return 0;
}

sub _do_eval {
	no strict;
	no warnings;
	eval $_[0];
}

sub unload {
	my $file = shift @_;
	my $package = file2pkg( $file );
	my $pkg_info = pkg_info( $package );

	if( $pkg_info ) {	
		# take care of the shutdown callback
		if( exists $pkg_info->{shutdown} ) {
			# allow incorrectly written scripts to be unloaded
			eval {
				if( ref $pkg_info->{shutdown} eq 'CODE' ) {
					$pkg_info->{shutdown}->();
				} elsif ( $pkg_info->{shutdown} ) {
					no strict 'refs';
					&{$pkg_info->{shutdown}};
				}
			};
		}

		if( exists $pkg_info->{hooks} ) {
			for my $hook ( @{$pkg_info->{hooks}} ) {
				Xchat::unhook( $hook, $package );
			}
		}


		if( exists $pkg_info->{gui_entry} ) {
			plugingui_remove( $pkg_info->{gui_entry} );
		}
		
		Symbol::delete_package( $package );
		delete $scripts{$package};
		return Xchat::EAT_ALL;
	} else {
		Xchat::print( qq{"$file" is not loaded.\n} );
		return Xchat::EAT_NONE;
	}
}

sub unload_all {
	for my $package ( keys %scripts ) {
		unload( $scripts{$package}->{filename} );
	}
	
	return Xchat::EAT_ALL;
}

sub reload {
	my $file = shift @_;
	my $package = file2pkg( $file );
	my $pkg_info = pkg_info( $package );
	my $fullpath = $file;
	
	if( $pkg_info ) {
		$fullpath = $pkg_info->{filename};
		unload( $file );
	}
	
	load( $fullpath );
	return Xchat::EAT_ALL;
}

sub reload_all {
	my @dirs = Xchat::get_info( "xchatdirfs" ) || Xchat::get_info( "xchatdir" );
	push @dirs, File::Spec->catdir( $dirs[0], "plugins" );
	for my $dir ( @dirs ) {
		my $auto_load_glob = File::Spec->catfile( $dir, "*.pl" );
		my @scripts = map { $_->{filename} }
			sort { $a->{loaded_at} <=> $b->{loaded_at} } values %scripts;
		push @scripts, File::Glob::bsd_glob( $auto_load_glob );

		my %seen;
		@scripts = grep { !$seen{ $_ }++ } @scripts;

		unload_all();
		for my $script ( @scripts ) {
			if( !pkg_info( file2pkg( $script ) ) ) {
				load( $script );
			}
		}
	}
}

sub expand_homedir {
	my $file = shift @_;

	if ( $^O eq "MSWin32" ) {
		$file =~ s/^~/$ENV{USERPROFILE}/;
	} else {
		$file =~ s{^~}{
			(getpwuid($>))[7] ||  $ENV{HOME} || $ENV{LOGDIR}
		}ex;
	}
	return $file;
}

sub file2pkg {
	my $string = File::Basename::basename( shift @_ );
	$string =~ s/\.pl$//i;
	$string =~ s|([^A-Za-z0-9/])|'_'.unpack("H*",$1)|eg;
	return "Xchat::Script::" . $string;
}

sub pkg_info {
	my $package = shift @_;
	return $scripts{$package};
}

sub find_external_pkg {
	my $level = 1;

	while( my @frame = caller( $level ) ) {
		return @frame if $frame[0] !~ /^Xchat/;
		$level++;
	}

}

sub find_pkg {
	my $level = 1;

	while( my ($package, $file, $line) = caller( $level ) ) {
		return $package if $package =~ /^Xchat::Script::/;
		$level++;
	}

	my @frame = find_external_pkg();
	my $location;

	if( $frame[0] or $frame[1] ) {
		$location = $frame[1] ? $frame[1] : "package $frame[0]";
		$location .= " line $frame[2]";
	} else {
		$location = "unknown location";
	}

	die "Unable to determine which script this hook belongs to. at $location\n";

}

sub fix_callback {
	my ($package, $callback) = @_;
	
	unless( ref $callback ) {
		# change the package to the correct one in case it was hardcoded
		$callback =~ s/^.*:://;
		$callback = qq[${package}::$callback];

		no strict 'subs';
		$callback = \&{$callback};
	}
	
	return $callback;
}
} # end of Xchat::Embed package

{
package Xchat::List::Network;
use strict;
use warnings;
use Storable qw(dclone);
my $last_modified;
my @servers;

sub get {
	my $server_file = Xchat::get_info( "xchatdirfs" ) . "/servlist_.conf";

	# recreate the list only if the server list file has changed
	if( -f $server_file && 
			(!defined $last_modified || $last_modified != -M $server_file ) ) {
		$last_modified = -M _;

		if( open my $fh, "<", $server_file ) {
			local $/ = "\n\n";
			while( my $record = <$fh> ) {
				chomp $record;
				next if $record =~ /^v=/; # skip the version line
				push @servers, Xchat::List::Network::Entry::parse( $record );
			}
		} else {
			warn "Unable to open '$server_file': $!";
		}
	}

	my $clone = dclone( \@servers );
	return @$clone;
}
} # end of Xchat::List::Network

{
package Xchat::List::Network::Entry;
use strict;
use warnings;

my %key_for = (
	I => "irc_nick1",
	i => "irc_nick2",
	U => "irc_user_name",
	R => "irc_real_name",
	P => "server_password",
	B => "nickserv_password",
	N => "network",
	D => "selected",
	E => "encoding",
);
my $letter_key_re = join "|", keys %key_for;

sub parse {
	my $data  = shift;
	my $entry = {
		irc_nick1       => undef,
		irc_nick2       => undef,
		irc_user_name   => undef,
		irc_real_name   => undef,
		server_password => undef,

		# the order of the channels need to be maintained
		# list of { channel => .., key => ... }
		autojoins         => Xchat::List::Network::AutoJoin->new( '' ),
		connect_commands   => [],
		flags             => {},
		selected          => undef,
		encoding          => undef,
		servers           => [],
		nickserv_password => undef,
		network           => undef,
	};

	my @fields = split /\n/, $data;
	chomp @fields;

	for my $field ( @fields ) {
	SWITCH: for ( $field ) {
			/^($letter_key_re)=(.*)/ && do {
				$entry->{ $key_for{ $1 } } = $2;
				last SWITCH;
			};

			/^J.(.*)/ && do {
				$entry->{ autojoins } =
					Xchat::List::Network::AutoJoin->new( $1 );
			};

			/^F.(.*)/ && do {
				$entry->{ flags } = parse_flags( $1 );
			};

			/^S.(.+)/ && do {
				push @{$entry->{servers}}, parse_server( $1 );
			};

			/^C.(.+)/ && do {
				push @{$entry->{connect_commands}}, $1;
			};
		}
	}

#	$entry->{ autojoins } = $entry->{ autojoin_channels };
	return $entry;
}

sub parse_flags {
	my $value = shift || 0;
	my %flags;

	$flags{ "cycle" }         = $value & 1  ? 1 : 0;
	$flags{ "use_global" }    = $value & 2  ? 1 : 0;
	$flags{ "use_ssl" }       = $value & 4  ? 1 : 0;
	$flags{ "autoconnect" }   = $value & 8  ? 1 : 0;
	$flags{ "use_proxy" }     = $value & 16 ? 1 : 0;
	$flags{ "allow_invalid" } = $value & 32 ? 1 : 0;

	return \%flags;
}

sub parse_server {
	my $data = shift;
	if( $data ) {
		my ($host, $port) = split /\//, $data;
		unless( $port ) {
			my @parts = split /:/, $host;

			# if more than 2 then we are probably dealing with a IPv6 address
			# if less than 2 then no port was specified
			if( @parts == 2 ) {
				$port = $parts[1];
			}
		}

		$port ||= 6667;
		return { host => $host, port => $port };
	}
}

} # end of Xchat::List::Network::Entry

{
package Xchat::List::Network::AutoJoin;
use strict;
use warnings;

use overload
#	'%{}' => \&as_hash,
#	'@{}' => \&as_array,
	'""'   => 'as_string',
	'0+'   => 'as_bool';

sub new {
	my $class = shift;
	my $line = shift;

	my @autojoins;

	if ( $line ) {
		my ( $channels, $keys ) = split / /, $line, 2;
		my @channels = split /,/, $channels;
		my @keys     = split /,/, ($keys || '');

		for my $channel ( @channels ) {
			my $key = shift @keys;
			$key = '' unless defined $key;

			push @autojoins, {
				channel => $channel,
				key     => $key,
				};
		}
	}
	return bless \@autojoins, $class;
}

sub channels {
	my $self = shift;

	if( wantarray ) {
		return map { $_->{channel} } @$self;
	} else {
		return scalar @$self;
	}
}

sub keys {
	my $self = shift;
	return map { $_->{key} } @$self  ;

}

sub pairs {
	my $self = shift;

	my @channels = $self->channels;
	my @keys = $self->keys;

	my @pairs = map { $_ => shift @keys } @channels;
}

sub as_hash {
	my $self = shift;
	return +{ $self->pairs };
}

sub as_string {
	my $self = shift;
	return join " ",
		join( ",", $self->channels ),
		join( ",", $self->keys );
}

sub as_array {
	my $self = shift;
	return [ map { \%$_ } @$self ];
}

sub as_bool {
	my $self = shift;
	return $self->channels ? 1 : "";
}

} # end of Xchat::Server::AutoJoin

1;
