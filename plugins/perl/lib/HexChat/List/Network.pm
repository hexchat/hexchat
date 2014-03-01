package HexChat::List::Network;
use strict;
use warnings;
use Storable qw(dclone);
my $last_modified;
my @servers;

sub get {
	my $server_file = HexChat::get_info( "configdir" ) . "/servlist.conf";

	# recreate the list only if the server list file has changed
	if( -f $server_file && 
			(!defined $last_modified || $last_modified != -M $server_file ) ) {
		$last_modified = -M _;

		@servers = ();
		if( open my $fh, "<", $server_file ) {
			local $/ = "\n\n";
			while( my $record = <$fh> ) {
				chomp $record;
				next if $record =~ /^v=/; # skip the version line
				push @servers, HexChat::List::Network::Entry::parse( $record );
			}
		} else {
			warn "Unable to open '$server_file': $!";
		}
	}

	my $clone = dclone( \@servers );
	return @$clone;
}

1
