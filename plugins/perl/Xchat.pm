BEGIN {
	$INC{'Xchat.pm'} = 'DUMMY';
}

BEGIN {
	$SIG{__WARN__} = sub {
		my $message = shift @_;
		my ($package) = caller;
		my $pkg_info = Xchat::Embed::pkg_info( $package );
	
		if( $pkg_info ) {
			if( $message =~ /\(eval 1\)/ ) {
				$message =~ s/\(eval 1\)/(PERL PLUGIN CODE)/;
			} else {
				$message =~ s/\(eval \d+\)/$pkg_info->{filename}/;
			}
		}
		Xchat::print( $message );
	};
}
use File::Spec();
use File::Basename();
use Symbol();

{
package Xchat;
use base qw(Exporter);
use strict;
use warnings;

our %EXPORT_TAGS = (
#	all => [
##		qw(register hook_server hook_command),
#		qw(hook_print hook_timer unhook print command),
#		qw(find_context get_context set_context),
#		qw(get_info get_prefs emit_print nickcmp),
#		qw(get_list context_info strip_code),
#		qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW),
#		qw(PRI_LOWEST EAT_NONE EAT_XCHAT EAT_PLUGIN),
#		qw(EAT_ALL KEEP REMOVE),
#	],
	constants => [
		qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST), # priorities
		qw(EAT_NONE EAT_XCHAT EAT_PLUGIN EAT_ALL), # callback return values
		qw(FD_READ FD_WRITE FD_EXCEPTION FD_NOTSOCKET), # fd flags
		qw(KEEP REMOVE), # timers
	],
	hooks => [
		qw(hook_server hook_command hook_print hook_timer unhook),
	],
	util => [
		qw(register nickcmp strip_code), # misc
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
	$description = "" unless defined $description;
	$pkg_info->{shutdown} = $callback;
	
	$pkg_info->{gui_entry} =
		Xchat::Internal::register( $name, $version, $description, $filename );
	# keep with old behavior
	return ();
}

sub hook_server {
	return undef unless @_ >= 2;
	my $message = shift;
	my $callback = shift;
	my $options = shift;
	my $package = Xchat::Embed::find_pkg();
	
	$callback = Xchat::Embed::fix_callback( $package, $callback );
	
	my ($priority, $data) = ( Xchat::PRI_NORM, undef );
	
	if( ref( $options ) eq 'HASH' ) {
		if( exists( $options->{priority} ) && defined( $options->{priority} ) ) {
			$priority = $options->{priority};
		}
		
		if( exists( $options->{data} ) && defined( $options->{data} ) ) {
			$data = $options->{data};
		}
	}
	
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
	
	my ($priority, $help_text, $data) = ( Xchat::PRI_NORM, '', undef );
	
	if( ref( $options ) eq 'HASH' ) {
		if( exists( $options->{priority} ) && defined( $options->{priority} ) ) {
			$priority = $options->{priority};
		}

		if(
			exists( $options->{help_text} )
			&& defined( $options->{help_text} )
		) {
			$help_text = $options->{help_text};
		}

		if ( exists( $options->{data} ) && defined( $options->{data} ) ) {
			$data = $options->{data};
		}
	}
	
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
	
	if ( ref( $options ) eq 'HASH' ) {
		if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {
			$priority = $options->{priority};
		}

		if ( exists( $options->{data} ) && defined( $options->{data} ) ) {
			$data = $options->{data};
		}
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
	my $hook = Xchat::Internal::hook_timer( $timeout, $callback, $data );
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

	if( ref( $options ) eq 'HASH' ) {
		if( exists( $options->{flags} ) && defined( $options->{flags} ) ) {
			$flags = $options->{flags};
		}
		
		if ( exists( $options->{data} ) && defined( $options->{data} ) ) {
			$data = $options->{data};
		}
	}
	
	my $cb = sub {
		my $userdata = shift;
		no strict 'refs';
		return &{$userdata->{CB}}(
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

	if( $hook =~ /^\d+$/ && grep { $_ == $hook } @{$pkg_info->{hooks}} ) {
		$pkg_info->{hooks} = [grep { $_ != $hook } @{$pkg_info->{hooks}}];
		return Xchat::Internal::unhook( $hook );
	}
	return ();
}

sub print {
	my $text = shift @_;
	return 1 unless defined $text;
	if( ref( $text ) eq 'ARRAY' ) {
		if( $, ) {
			$text = join $, , @$text;
		} else {
			$text = join "", @$text;
		}
	}
	
	if ( @_ >= 1 ) {
		my $channel = shift @_;
		my $server = shift @_;
		my $old_ctx = Xchat::get_context();
		my $ctx = Xchat::find_context( $channel, $server );
		
		if ( $ctx ) {
			Xchat::set_context( $ctx );
			Xchat::Internal::print( $text );
			Xchat::set_context( $old_ctx );
			return 1;
		} else {
			return 0;
		}
	} else {
		Xchat::Internal::print( $text );
		return 1;
	}
}

sub printf {
	my $format = shift;
	Xchat::print( sprintf( $format, @_ ) );
}

# make Xchat::prnt() and Xchat::prntf() as aliases for Xchat::print() and 
# Xchat::printf(), mainly useful when these functions are exported
*Xchat::prnt = *Xchat::print{CODE};
*Xchat::prntf = *Xchat::printf{CODE};

sub command {
	my $command = shift;
	my @commands;
	
	if( ref( $command ) eq 'ARRAY' ) {
		@commands = @$command;
	} else {
		@commands = ($command);
	}
	
	if( @_ >= 1 ) {
		my ($channel, $server) = @_;
		my $old_ctx = Xchat::get_context();
		my $ctx = Xchat::find_context( $channel, $server );
		if( $ctx ) {
			Xchat::set_context( $ctx );
			Xchat::Internal::command( $_ ) foreach @commands;
			Xchat::set_context( $old_ctx );
			return 1;
		} else {
			return 0;
		}
	} else {
		Xchat::Internal::command( $_ ) foreach @commands;
		return 1;
	}
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
	my $nick = shift @_ || Xchat::get_info( "nick" );
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
		qw(away channel charset host id inputbox libdirfs network),
		qw(nick nickserv server topic version win_status xchatdir xchatdirfs),
		qw(state_cursor),
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

sub strip_code {
	my $pattern = qr[
		\cB| #Bold
		\cC\d{0,2}(?:,\d{1,2})?| #Color
		\cG| #Beep
		\cO| #Reset
		\cV| #Reverse
		\c_  #Underline
	]x;
		
	if( defined wantarray ) {
		my $msg = shift;
		$msg =~ s/$pattern//g;
		return $msg;
	} else {
		$_[0] =~ s/$pattern//g;
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
		Xchat::print(
			qq{'$filename' already loaded from '$pkg_info->{filename}'.\n}
		);
		Xchat::print(
			'If this is a different script then it rename and try '.
			'loading it again.'
		);
		return 2;
	}
	
	if( open FH, $file ) {
		my $source = do {local $/; <FH>};
		close FH;
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

		{
			no strict; no warnings;
			eval "package $package; $source;";

			unless( exists $scripts{$package}{gui_entry} ) {
				$scripts{$package}{gui_entry} =
					Xchat::Internal::register(
						"???", "???", "This script did not call register()", $file
					);
			}
		}
		
		if( $@ ) {
			# something went wrong
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

sub unload {
	my $file = shift @_;
	my $package = file2pkg( $file );
	my $pkg_info = pkg_info( $package );

	if( $pkg_info ) {
		if( exists $pkg_info->{hooks} ) {
			for my $hook ( @{$pkg_info->{hooks}} ) {
				Xchat::unhook( $hook, $package );
			}
		}
		
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

sub unload_all {
	for my $package ( keys %scripts ) {
		unload( $scripts{$package}->{filename} );
	}
	
	return Xchat::EAT_ALL;
}
#sub auto_load {
#	my $dir = Xchat::get_info( "xchatdirfs" ) || Xchat::get_info( "xchatdir" );
#
#	if( opendir my $dir_handle, $dir ) {
#		my @files = readdir $dir_handle;
# 		
#		for( @files ) {
#			my $fullpath = File::Spec->catfile( $dir, $_ );
#			load( $fullpath ) if $fullpath =~ m/\.pl$/i;
#		}
#
#		closedir $dir_handle;
#	}
#}

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

sub find_pkg {
	my $level = 1;
	my $package = (caller( $level ))[0];
	while( $package !~ /^Xchat::Script::/ ) {
		$level++;
		$package = (caller( $level ))[0];
	}
	return $package;
}

sub fix_callback {
	my ($package, $callback) = @_;
	
	unless( ref $callback ) {
		# change the package to the correct one in case it was hardcoded
		$callback =~ s/^.*:://;
		$callback = qq[${package}::$callback];
	}
	
	return $callback;
}
} # end of Xchat::Embed package
