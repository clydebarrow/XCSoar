#!/usr/bin/perl -w

use strict;

my $next_id = 100;

# ICON_DENSITIES lists the icon densities to embed, such as "xhdpi";
# all four by default.  The others become null resources, and
# MaskedIcon::LoadResource() scales the nearest one that is there.
my %icon_density = map { $_ => 1 }
  split(/,/, $ENV{ICON_DENSITIES} || 'ldpi,mdpi,xhdpi,xxhdpi');

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my $id = $next_id++;
        print qq|MAKE_RESOURCE($1, $2, $id);\n|;
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my ($name, $file) = ($1, $2);
        foreach my $d (['ldpi', ''], ['mdpi', '_MDPI'],
                       ['xhdpi', '_XHDPI'], ['xxhdpi', '_XXHDPI']) {
            my ($density, $suffix) = @$d;
            my $id = $next_id++;
            if ($icon_density{$density}) {
                print qq|MAKE_RESOURCE($name$suffix, ${file}_$density, $id);\n|;
            } else {
                print qq|static constexpr ResourceId $name$suffix = ResourceId::Null();\n|;
            }
        }

        # this macro can be passed to MaskedIcon::LoadResource()
        print qq|#define ${name}_ALL $name, ${name}_MDPI, ${name}_XHDPI, ${name}_XXHDPI\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # not used here
    } else {
        die "Syntax error: $_";
    }
}
