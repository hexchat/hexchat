package HexChat::List::Network::Entry;
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
		autojoins         => HexChat::List::Network::AutoJoin->new( '' ),
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

	$entry->{ autojoins } = HexChat::List::Network::AutoJoin->new();

	for my $field ( @fields ) {
	SWITCH: for ( $field ) {
			/^($letter_key_re)=(.*)/ && do {
				$entry->{ $key_for{ $1 } } = $2;
				last SWITCH;
			};

			/^J.(.*)/ && do {
				$entry->{ autojoins }->add( $1 );
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

1
