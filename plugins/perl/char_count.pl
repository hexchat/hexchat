
use strict;
use warnings;
use Xchat qw(:all);
use Glib;
use Gtk2 -init;

sub get_inputbox {
	my $widget = Glib::Object->new_from_pointer( get_info( "win_ptr" ), 0 );
	my $input_box;

	my @containers = ($widget);

	while( @containers ) {
		my $container = shift @containers;

		for my $child ( $container->get_children ) {
			if( $child->get( "name" ) eq 'xchat-inputbox' ) {
				$input_box = $child;
				last;
			} elsif( $child->isa( "Gtk2::Container" ) ) {
				push @containers, $child;
			}
		}
	}
	return $input_box;
}

sub get_hbox {
	my $widget = shift;
	my $hbox;

	while( $widget->parent ) {
		if( $widget->parent->isa( "Gtk2::HBox" ) ) {
			return $widget->parent;
		}
		$widget = $widget->parent;
	}

}

my $input_box = get_inputbox();

if( $input_box ) {
	my $hbox = get_hbox( $input_box );
	if( $hbox ) {
		my $label = Gtk2::Label->new();
		$label->set_alignment( 0.5, ($label->get_alignment)[1] );
		$hbox->pack_end( $label, 0, 0, 5 );
		$label->show();

		hook_print( "Key Press",
			sub {
				my $ctx_type = context_info->{"type"};
				hook_timer( 0, sub {
					if( $ctx_type == 2 || $ctx_type == 3 ) {
						my $count = length get_info "inputbox";
						$label->set_text( $count ? $count : "" );
					} else {
						$label->set_text( "" );
					}
					return REMOVE;
				});
				return EAT_NONE;
			}
		);

		hook_command( "",
			sub {
				$label->set_text( "" );
				return EAT_NONE;
			}
		);
		register( "Character Counter", "1.0.0",
			"Display the number of characters in the inputbox",
			sub {
				$hbox->remove( $label );
			}
		);
	} else {
		prnt "Counldn't find hbox";
	}

} else {
	prnt "Couldn't fint input box";
}
