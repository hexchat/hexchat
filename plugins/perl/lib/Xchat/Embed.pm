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

1
