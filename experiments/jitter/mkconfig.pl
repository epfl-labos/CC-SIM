#!/usr/bin/env perl
use strict;
use warnings;

use JSON;

use lib '../';
use Common;

my $json = JSON->new->pretty;
my $input_file = $ARGV[0];
my $output_prefix = $ARGV[1];

my $template = Common::read_json_file($input_file);

for my $simtime (0.25, 0.5, 0.75, 1, 1.5, 2, 5, 10, 15, 20, 30) {
	my $config = { %{$template} };
	$config->{application}{stop_after_simulated_seconds} = $simtime;
	for my $i (0..9) {
		my $file = $output_prefix . sprintf("jitter_%g_%03d.json", $simtime, $i);
		Common::write_json_file($file, $config);
	}
}
