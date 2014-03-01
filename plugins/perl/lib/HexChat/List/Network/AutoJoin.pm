package HexChat::List::Network::AutoJoin;
use strict;
use warnings;

use overload
#	'%{}' => \&as_hash,
#	'@{}' => \&as_array,
	'""'   => 'as_string',
	'0+'   => 'as_bool';

sub new {
	my $class = shift;

	my @autojoins;

	return bless \@autojoins, $class;
}

sub add {
	my $self = shift;

	my $line = shift;

	my ( $channel, $key ) = split /,/, $line, 2;
	$key = $key || '';

	push @$self, {
		channel => $channel,
		key     => $key,
		};
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

1
