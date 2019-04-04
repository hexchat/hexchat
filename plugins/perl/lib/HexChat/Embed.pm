package HexChat::Embed;
use strict;
use warnings;
use Data::Dumper;
# list of loaded scripts keyed by their package names
# The package names are generated from the filename of the script using
# the file2pkg() function.
# The values of this hash are hash references with the following keys:
#   filename
#     The full path to the script.
#   gui_entry
#     This is hexchat_plugin pointer that is used to remove the script from
#     Plugins and Scripts window when a script is unloaded. This has also
#     been converted with the PTR2IV() macro.
#   hooks
#     This is an array of hooks that are associated with this script.
#     These are pointers that have been converted with the PTR2IV() macro.
#   inner_packages
#     Other packages that are defined in a script. This is not recommended
#     partly because these will also get removed when a script is unloaded.
#   loaded_at
#     A timestamp of when the script was loaded. The value is whatever
#     Time::HiRes::time() returns. This is used to retain load order when
#     using the RELOADALL command.
#   shutdown
#     This is either a code ref or undef. It will be executed just before a
#     script is unloaded.
our %scripts;

# This is a mapping of "inner package" => "containing script package"
our %owner_package;

# used to keep track of which package a hook belongs to, if the normal way of
# checking which script is calling a hook function fails this will be used
# instead. When a hook is created this will be copied to the HookData structure
# and when a callback is invoked this it will be used to set this value.
our $current_package;

sub load {
	my $file = expand_homedir( shift @_ );
	my $package = file2pkg( $file );
	
	if( exists $scripts{$package} ) {
		my $pkg_info = pkg_info( $package );
		my $filename = File::Basename::basename( $pkg_info->{filename} );
		HexChat::printf(
			qq{'%s' already loaded from '%s'.\n},
			$filename, $pkg_info->{filename}
		);
		HexChat::print(
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
		
		# this must come before the eval or the filename will not be found in
		# HexChat::register
		$scripts{$package}{filename} = $file;
		$scripts{$package}{loaded_at} = Time::HiRes::time();

		# this must be done before the error check so the unload will remove
		# any inner packages defined by the script. if a script fails to load
		# then any inner packages need to be removed as well.
		my @inner_packages = $source =~
			m/^\s*package \s+
				((?:[^\W:]+(?:::)?)+)\s*? # package name
				# strict version number
				(?:\d+(?:[.]\d+) # positive integer or decimal-fraction
					|v\d+(?:[.]\d+){2,})? # dotted-decimal v-string
				[{;]
			/mgx;

		# check if any inner package defined in the to be loaded script has
		# already been defined by another script
		my @conflicts;
		for my $inner ( @inner_packages ) {
			if( exists $owner_package{ $inner } ) {
				push @conflicts, $inner;
			}
		}

		# report conflicts and bail out
		if( @conflicts ) {
			my $error_message =
				"'$file' won't be loaded due to conflicting inner packages:\n";
			for my $conflict_package ( @conflicts ) {
				$error_message .= "   $conflict_package already defined in " .
					pkg_info($owner_package{ $conflict_package })->{filename}."\n";
			}
			HexChat::print( $error_message );

			return 2;
		}

		my $full_path = File::Spec->rel2abs( $file );
		$source =~ s/^/#line 1 "$full_path"\n\x7Bpackage $package;/;

		# make sure we add the closing } even if the last line is a comment
		if( $source =~ /^#.*\Z/m ) {
			$source =~ s/^(?=#.*\Z)/\x7D/m;
		} else {
			$source =~ s/\Z/\x7D/;
		}

		$scripts{$package}{inner_packages} = [ @inner_packages ];
		@owner_package{ @inner_packages } = ($package) x @inner_packages;
		_do_eval( $source );

		unless( exists $scripts{$package}{gui_entry} ) {
			$scripts{$package}{gui_entry} =
				HexChat::Internal::register(
					"", "unknown", "", $file
				);
		}

		if( $@ ) {
			# something went wrong
			$@ =~ s/\(eval \d+\)/$file/g;
			HexChat::print( "Error loading '$file':\n$@\n" );
			# make sure the script list doesn't contain false information
			unload( $scripts{$package}{filename} );
			return 1;
		}
	} else {
		HexChat::print( "Error opening '$file': $!\n" );
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
				HexChat::unhook( $hook, $package );
			}
		}

		if( exists $pkg_info->{gui_entry} ) {
			plugingui_remove( $pkg_info->{gui_entry} );
		}
		
		delete @owner_package{ @{$pkg_info->{inner_packages}} };
		for my $inner_package ( @{$pkg_info->{inner_packages}} ) {
			Symbol::delete_package( $inner_package );
		}
		Symbol::delete_package( $package );
		delete $scripts{$package};
		return HexChat::EAT_ALL;
	} else {
		HexChat::print( qq{"$file" is not loaded.\n} );
		return HexChat::EAT_NONE;
	}
}

sub unload_all {
	for my $package ( keys %scripts ) {
		unload( $scripts{$package}->{filename} );
	}
	
	return HexChat::EAT_ALL;
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
	return HexChat::EAT_ALL;
}

sub reload_all {
	my @dirs = HexChat::get_info( "configdir" );
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

sub evaluate {
	my ($code) = @_;

	my @results = eval $code;
	HexChat::print $@ if $@; #print warnings

	local $Data::Dumper::Sortkeys = 1;
	local $Data::Dumper::Terse    = 1;

	if (@results > 1) {
		HexChat::print Dumper \@results;
	}
	elsif (ref $results[0] || !$results[0]) {
		HexChat::print Dumper $results[0];
	}
	else {
		HexChat::print $results[0];
	}

	return HexChat::EAT_HEXCHAT;
};

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
	return "HexChat::Script::" . $string;
}

sub pkg_info {
	my $package = shift @_;
	return $scripts{$package};
}

sub find_external_pkg {
	my $level = 1;

	while( my @frame = caller( $level ) ) {
		return @frame if $frame[0] !~ /(?:^IRC$|^HexChat)/;
		$level++;
	}
	return;
}

sub find_pkg {
	my $level = 1;

	while( my ($package, $file, $line) = caller( $level ) ) {
		return $package if $package =~ /^HexChat::Script::/;
		$level++;
	}

	my $current_package = get_current_package();
	if( defined $current_package ) {
		return $current_package;
	}

	my @frame = find_external_pkg();
	my $location;

	if( $frame[0] or $frame[1] ) {
		my $calling_package = $frame[0];
		if( defined( my $owner = $owner_package{ $calling_package } ) ) {
			return ($owner, $calling_package);
		}

		$location = $frame[1] ? $frame[1] : "package $frame[0]";
		$location .= " line $frame[2]";
	} else {
		$location = "unknown location";
	}

	die "Unable to determine which script this hook belongs to. at $location\n";

}

# convert function names into code references
sub fix_callback {
	my ($package, $calling_package, $callback) = @_;
	
	unless( ref $callback ) {
		unless( $callback =~ /::/ ) {
			my $prefix = defined $calling_package ? $calling_package : $package;
			$callback =~ s/^/${prefix}::/;
		}

		no strict 'subs';
		$callback = \&{$callback};
	}
	
	return $callback;
}

sub get_current_package {
	return $current_package;
}

sub set_current_package {
	my $old_package = $current_package;
	$current_package = shift;

	return $old_package;
}

1
