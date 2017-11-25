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
	{ name => "partitions",  path => ["config", "cluster", "partitions_per_replica"] },
	{ name => "throughput",  path => ["stats", "cluster", "throughput"], ref_name => "throughput" },
	{ name => "get_latency", path => ["stats", "client", "get average latency"], ref_name => "get_latency" },
	{ name => "put_latency", path => ["stats", "client", "put average latency"], ref_name => "put_latency" },
	{ name => "cpu_usage"  , path => ["stats", "server", "cpu usage"]},
);

# Column to use as the X-axis
my $x_axis_column_name = "partitions";

# For each other column, a plot will be generated with that column on the Y-axis

# Reference data from real-world experiments
my %ref_data = (
	"gr"  => Common::read_json_file("../gr_ec2_data_2.json"),
	"grv" => Common::read_json_file("../grv_ec2_data_2.json"),
);

MakeDatAndPlot::make_dat_and_plot(
	outputs_dir => "outputs",
	x_axis_column_name => $x_axis_column_name,
	columns => \@columns,
	ref_data => \%ref_data,
	output_file => "plots_fragment.tex",
);
