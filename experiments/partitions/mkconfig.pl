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

my @partitions = (2, 4, 8, 16, 24, 32);
my @protocols = qw(gr grv);

for my $partitions (@partitions) {
	my $config = { %{$template} };
	$config->{cluster}{partitions_per_replica} = $partitions;
	for my $protocol (@protocols) {
		my $file = $output_prefix . sprintf("partitions_%03d_%s.json",
											$partitions, $protocol);
		$config->{application}{protocol} = $protocol;
		Common::write_json_file($file, $config);
	}
}
