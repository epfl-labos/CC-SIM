#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw "$Bin";
use lib "$Bin";
use Common;
use ReferenceData;

my @output_directories = @ARGV;

my %keys;
my @rows;

sub process_output_directory {
	my ($output_dir) = @_;
	my $config = Common::read_json_file("$output_dir/config.json");
	my $stats = Common::read_json_file("$output_dir/stats.json");
	my $reference = ReferenceData::reference_stats($config);

	return if $config->{application}{protocol} eq "gr";

	my %row;

	my $f;
	$f = sub {
		my ($name, $data) = @_;
		if (ref $data eq "HASH") {
			for my $k (keys %$data) {
				$f->("$name/$k", $data->{$k});
			}
		} else {
			$row{$name} = $data;
		}
	};

	$f->("", $stats);
	$f->("/reference", $reference);
	return \%row;
}

for my $directory (@output_directories) {
	my $row = process_output_directory $directory;
	next unless defined $row;
	$keys{$_} = undef for keys %$row;
	push @rows, $row;
}

my @keys = sort(keys %keys);
print join(",", @keys) . "\n";
for my $row (@rows) {
	my @columns;
	push @columns, $row->{$_} // "" for (@keys);
	print join(",", @columns) . "\n";
}
