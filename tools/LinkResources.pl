#!/usr/bin/perl -w

use strict;
use lib './tools';
use BinToC;

sub add_to_src($$) {
    my ($input, $name) = @_;
    BinToC::binary_to_c($input,
                        \*STDOUT,
                        ${name},
                        ${name}."_end",
                        ${name}."_size");
}

# ICON_DENSITIES lists the icon densities to embed, such as "xhdpi";
# all four by default.  The others become null resources, and
# MaskedIcon::LoadResource() scales the nearest one that is there.
my %icon_density = map { $_ => 1 }
  split(/,/, $ENV{ICON_DENSITIES} || 'ldpi,mdpi,xhdpi,xxhdpi');

my $enable_opengl = $ENV{ENABLE_OPENGL} eq 'y';
my $target_is_android = $ENV{TARGET_IS_ANDROID} eq 'y';

my $icons_dir = $enable_opengl
  ? 'output/data/icons'
  : 'output/data/icons2';

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        add_to_src("output/data/graphics2/$2.png", "resource_$1");
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my ($name, $file) = ($1, $2);
        add_to_src("$icons_dir/${file}_ldpi.png", "resource_$name")
          if $icon_density{ldpi};
        add_to_src("$icons_dir/${file}_mdpi.png", "resource_${name}_MDPI")
          if $icon_density{mdpi};
        add_to_src("$icons_dir/${file}_xhdpi.png", "resource_${name}_XHDPI")
          if $icon_density{xhdpi};
        add_to_src("$icons_dir/${file}_xxhdpi.png", "resource_${name}_XXHDPI")
          if $icon_density{xxhdpi};
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        add_to_src("output/data/sound/$2.raw", "resource_$1") unless $target_is_android;
    } else {
        die "Syntax error: $_";
    }
}
