BEGIN {
  $INC{'Xchat.pm'} = 'DUMMY';
}

{
  package Xchat;
  use base qw(Exporter);
  our @EXPORT = (qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST),
		 qw(EAT_NONE EAT_XCHAT EAT_PLUGIN EAT_ALL),
		);
  our @EXPORT_OK = (qw(register hook_server hook_command hook_print),
		    qw(hook_timer unhook print command find_context),
		    qw(get_context set_context get_info get_prefs emit_print),
		    qw(nickcmp get_list context_info strip_code),
		    qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW PRI_LOWEST),
		    qw(EAT_NONE EAT_XCHAT EAT_PLUGIN EAT_ALL),
		   );
  our %EXPORT_TAGS = ( all => [
			       qw(register hook_server hook_command),
			       qw(hook_print hook_timer unhook print command),
			       qw(find_context get_context set_context),
			       qw(get_info get_prefs emit_print nickcmp),
			       qw(get_list context_info strip_code),
			       qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW),
			       qw(PRI_LOWEST EAT_NONE EAT_XCHAT EAT_PLUGIN),
			       qw(EAT_ALL),
			      ],
		       constants => [
				     qw(PRI_HIGHEST PRI_HIGH PRI_NORM PRI_LOW),
				     qw(PRI_LOWEST EAT_NONE EAT_XCHAT),
				     qw(EAT_PLUGIN EAT_ALL),
				    ],
		       hooks => [
				 qw(hook_server hook_command),
				 qw(hook_print hook_timer unhook),
				],
		       util => [
				qw(register print command find_context),
				qw(get_context set_context get_info get_prefs),
				qw(emit_print nickcmp get_list context_info),
				qw(strip_code),
			       ],
		     );

}

sub Xchat::hook_server {
  return undef unless @_ >= 2;

  my $message = shift;
  my $callback = shift;
  my $options = shift;
  
  my ($priority, $data) = ( Xchat::PRI_NORM, undef );
  
  if ( ref( $options ) eq 'HASH' ) {
	 if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {
		$priority = $options->{priority};
    }
    if ( exists( $options->{data} ) && defined( $options->{data} ) ) {
      $data = $options->{data};
    }
  }
  
  return Xchat::_hook_server( $message, $priority, $callback, $data);

}

sub Xchat::hook_command {
  return undef unless @_ >= 2;

  my $command = shift;
  my $callback = shift;
  my $options = shift;

  my ($priority, $help_text, $data) = ( Xchat::PRI_NORM, '', undef );

  if ( ref( $options ) eq 'HASH' ) {
    if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {
      $priority = $options->{priority};
    }
    if ( exists( $options->{help_text} ) && defined( $options->{help_text} ) ) {
      $help_text = $options->{help_text};
    }
    if ( exists( $options->{data} ) && defined( $options->{data} ) ) {
      $data = $options->{data};
    }
  }

  return Xchat::_hook_command( $command, $priority, $callback, $help_text,
			       $data);

}

sub Xchat::hook_print {
  return undef unless @_ >= 2;

  my $event = shift;
  my $callback = shift;
  my $options = shift;

  my ($priority, $data) = ( Xchat::PRI_NORM, undef );

  if ( ref( $options ) eq 'HASH' ) {
    if ( exists( $options->{priority} ) && defined( $options->{priority} ) ) {
      $priority = $options->{priority};
    }
    if ( exists( $options->{data} ) && defined( $options->{data} ) ) {
      $data = $options->{data};
    }
  }

  return Xchat::_hook_print( $event, $priority, $callback, $data);

}


sub Xchat::hook_timer {
  return undef unless @_ >= 2;

  my $timeout = shift;
  my $callback = shift;
  my $data = shift;

  if( ref( $data ) eq 'HASH' && exists( $data->{data} )
      && defined( $data->{data} ) ) {
    $data = $data->{data};
  }

  return Xchat::_hook_timer( $timeout, $callback, $data );

}

sub Xchat::print {

  my $text = shift @_;
  return 1 unless $text;
  if( ref( $text ) eq 'ARRAY' ) {
    if( $, ) {
      $text = join $, , @$text;
    } else {
      $text = join "", @$text;
    }
  }

  
  if( @_ >= 1 ) {
    my $channel = shift @_;
    my $server = shift @_;
    my $old_ctx = Xchat::get_context();
    my $ctx = Xchat::find_context( $channel, $server );
    
    if( $ctx ) {
      Xchat::set_context( $ctx );
      Xchat::_print( $text );
      Xchat::set_context( $old_ctx );
      return 1;
    } else {
      return 0;
    }
  } else {
    Xchat::_print( $text );
    return 1;
  }

}

sub Xchat::printf {
  my $format = shift;
  Xchat::print( sprintf( $format, @_ ) );
}

sub Xchat::command {

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
      Xchat::_command( $_ ) foreach @commands;
      Xchat::set_context( $old_ctx );
      return 1;
    } else {
      return 0;
    }
  } else {
    Xchat::_command( $_ ) foreach @commands;
    return 1;
  }

}

sub Xchat::commandf {
  my $format = shift;
  Xchat::command( sprintf( $format, @_ ) );
}

sub Xchat::user_info {
  my $nick = shift @_ || Xchat::get_info( "nick" );
  my $user;

  for(Xchat::get_list( "users" ) ) {
	 if( Xchat::nickcmp( $_->{nick}, $nick ) == 0 ) {
		$user = $_;
		last;
	 }
  }
  return $user;
}

sub Xchat::context_info {
  my $ctx = shift @_;
  my $old_ctx = Xchat::get_context;
  my @fields = (qw(away channel host network nick server topic version),
					 qw(win_status xchatdir xchatdirfs),
					);

  Xchat::set_context( $ctx );
  my %info;
  for my $field ( @fields ) {
	 $info{$field} = Xchat::get_info( $field );
  }
  Xchat::set_context( $old_ctx );

  return %info if wantarray;
  return \%info;
}

sub Xchat::strip_code {
  my $pattern =
	 qr/\cB| #Bold
       \cC\d{0,2}(?:,\d{0,2})?| #Color
       \cG| #Beep
       \cO| #Reset
       \cV| #Reverse
       \c_  #Underline
      /x;

  if( defined wantarray ) {
	 my $msg = shift;
	 $msg =~ s/$pattern//g;
	 return $msg;
  } else {
	 $_[0] =~ s/$pattern//g;
  }
}

$SIG{__WARN__} = sub {
  local $, = "\n";
  my ($package, $file, $line, $sub) = caller(1);
  Xchat::print( "Warning from ${sub}." );
  Xchat::print( @_ );
};

sub Xchat::Embed::load {
  my $file = shift @_;

  if( open FH, $file ) {
	 my $data = do {local $/; <FH>};
	 close FH;

# 	 my $package = Xchat::Embed::valid_package( $file );
# 	 if( $data =~ m/^\s*package .*?;/m ) {
# 		$data =~ s/^\s*package .*?;/package $package;/m;
# 	 } else {
# 		$data = "package $package;" . $data;
# 	 }
	 eval $data;

	 if( $@ ) {
		# something went wrong
		Xchat::print( "Error loading '$file':\n$@\n" );
		return 1;
	 }

  } else {

	 Xchat::print( "Error opening '$file': $!\n" );
	 return 2;
  }

  return 0;
}

# sub Xchat::Embed::valid_package {

#   my $string = shift @_;
#   $string =~ s/\.pl$//i;
#   $string =~ s/([^A-Za-z0-9\/])/sprintf("_%2x",unpack("C",$1))/eg;
#   # second pass only for words starting with a digit
#   $string =~ s|/(\d)|sprintf("/_%2x",unpack("C",$1))|eg;

#   # Dress it up as a real package name
#   $string =~ s|/|::|g;
#   return "Xchat::Embed" . $string;
# }
