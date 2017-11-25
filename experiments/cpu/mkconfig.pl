#!/usr/bin/env perl
use strict;
use warnings;

use JSON;

my $json = JSON->new->pretty;
my $input_file = $ARGV[0];
my $output_prefix = $ARGV[1] . "/clients_";

$/ = undef;
my $fh;
open($fh, "<", $input_file) or die "Couldn't open $input_file: $!";
my $doc = $json->decode(<$fh>);
close $fh;

for my $i (1..64) {
	$doc->{cluster}->{clients_per_partition} = $i;
	my $file = $output_prefix . sprintf("%02d", $i) . ".json";
	open($fh, ">", $file) or die "Couldn't open $file: $!";
	print $fh $json->encode($doc);
	close $fh;
}
