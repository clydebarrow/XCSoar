#!/usr/bin/perl -w
#
# Read the (preprocessed) resources.txt and emit a
# name-to-ResourceId lookup table consumed by ResourceLookup.cpp.

use strict;

# ICON_DENSITIES lists the icon densities to embed, such as "xhdpi";
# all four by default.  The others become null resources, and
# MaskedIcon::LoadResource() scales the nearest one that is there.
my %icon_density = map { $_ => 1 }
  split(/,/, $ENV{ICON_DENSITIES} || 'ldpi,mdpi,xhdpi,xxhdpi');

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print qq|  { "$1", $1 },\n|;
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # a density that is not embedded is looked up as the nearest
        # one that is
        my $name = $1;
        my @order = (['ldpi', ''], ['mdpi', '_MDPI'],
                     ['xhdpi', '_XHDPI'], ['xxhdpi', '_XXHDPI']);
        for my $i (0 .. $#order) {
            my $found;
            for my $distance (0 .. $#order) {
                for my $j ($i - $distance, $i + $distance) {
                    next if $j < 0 || $j > $#order;
                    if ($icon_density{$order[$j][0]}) {
                        $found = $order[$j][1];
                        last;
                    }
                }
                last if defined $found;
            }
            print qq|  { "$name$order[$i][1]", $name$found },\n|;
        }
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # sounds are not bitmap resources — skip
    } else {
        die "Syntax error: $_";
    }
}
