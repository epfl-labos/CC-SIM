#!/usr/bin/env perl
use strict;
use warnings;

use lib '../';
use Common;
use MakeDatAndPlot;

my $dir = $ARGV[0];

# name: name of the column to use for this statistic in the .dat file
# path: where this statistic is found
# ref_name: key used to find the value of this statistic in %ref_data{$protocol}{$x_axis_column_name}
my @columns = (
	{ name => "clients",     path => ["config", "cluster", "clients_per_partition"] },
	{ name => "throughput",  path => ["stats", "cluster", "throughput"] },
	{ name => "get_latency", path => ["stats", "client", "get average latency"] },
	{ name => "put_latency", path => ["stats", "client", "put average latency"] },
	{ name => "cpu_usage"  , path => ["stats", "server", "cpu usage"] },
);

# Column to use as the X-axis
my $x_axis_column_name = "clients";

# For each other column, a plot will be generated with that column on the Y-axis

# Reference data from real-world experiments
my %ref_data = ();

MakeDatAndPlot::make_dat_and_plot(
	outputs_dir => "outputs",
	x_axis_column_name => $x_axis_column_name,
	columns => \@columns,
	ref_data => \%ref_data,
	output_file => "plots_fragment.tex",
);
