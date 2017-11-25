#!/usr/bin/env perl
use strict;
use warnings;

use File::Basename;
use File::Glob ':bsd_glob';

use lib '../';
use Common;

my $config_dir = "config";
my $output_dir = "outputs";
my $errors_file = "errors.json";

sub read_throughput {
	my ($conf, $variant) = @_;
	my $output = $output_dir . "/" . (basename($conf) =~ s/\.json$//r) . "." . $variant . "/stats.json";
	my $stats = Common::read_json_file($output);
	return $stats->{cluster}{requests};
}

my %doc;
for my $c (bsd_glob("$config_dir/*")) {
	#my $gr_targets = Common::read_json_file("../gr_ec2_throughput_per_number_of_partitions.json");
	my $grv_targets = Common::read_json_file("../grv_ec2_throughput_per_number_of_partitions_2.json");
	my $config = Common::read_json_file($c);
	#my $gr_throughput = read_throughput($c, "gr");
	my $grv_throughput = read_throughput($c, "grv");
	my $partitions = $config->{cluster}{partitions_per_replica};
	$doc{$c} = [$grv_throughput - $grv_targets->{$partitions}];
	#$doc{$c} = [$gr_throughput - $gr_targets->{$partitions}, $grv_throughput - $grv_targets->{$partitions}];
}

our $json;
print "errors.json: " . $json->encode(\%doc);
Common::write_json_file($errors_file, \%doc);
