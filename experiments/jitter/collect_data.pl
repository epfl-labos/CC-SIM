#!/usr/bin/env perl
use strict;
use warnings;
no warnings 'experimental::autoderef';
use autodie;
use v5.20;


use File::Basename;
use File::Glob ':bsd_glob';
use List::Util qw(min max);

use lib '../';
use Common;

use Data::Dumper;

my $output_dir = $ARGV[0];

my %data;

for my $output (bsd_glob("$output_dir/*")) {
	my $stats = Common::read_json_file("$output/stats.json");
	my $config = Common::read_json_file("$output/config.json");
	my $ref = \$data{$config->{application}{variant}}{$config->{application}{stop_after_simulated_seconds}};
	$$ref //= [];
	push @$$ref, $stats->{cluster}{requests};
}

write_plot_data('grv');

sub write_plot_data {
	my ($variant) = @_;
	my $variant_data = $data{$variant};
	open(my $f_min_max, ">", $variant . "_min_max.dat");
	open(my $f, ">", $variant . ".dat");

	for my $k (sort {$a <=> $b} keys $variant_data) {
		printf $f_min_max "%g\t%g\t%g\n", $k, min(@{$variant_data->{$k}}), max(@{$variant_data->{$k}});
		printf $f "%g\t%g\n", $k, $_ for @{$variant_data->{$k}};
	}

	open(my $p, ">", $variant . ".plot");
	print $p <<EOF;
set style line 5 lc rgb 'grey'
set title "Throughput variation between successive runs"
plot '${variant}_min_max.dat' using 1:2:3 with filledcurves notitle ls 5 fs solid 0.5, \\
     '${variant}.dat' using 1:2 with points lc rgb 'red' title 'samples', \\
     '${variant}_min_max.dat' using 1:(\$3 - \$2) with lines lc rgb 'green' title 'width' axes x1y2
EOF
}
