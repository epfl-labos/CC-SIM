#!/usr/bin/env perl
use strict;
use warnings;
no warnings 'experimental::autoderef';
use v5.20;
use Data::Dumper;
$Data::Dumper::Sortkeys = 1;

use File::Basename;
use List::Util 'sum';

use lib '../';
use Common;

my $config_file = $ARGV[0];
my $step_file = $ARGV[1];
my $make_args = "-j4";
my @partitions = (8);
main();
our $json;

sub main {
	my $step_params = Common::read_json_file($step_file);
	my $config_params = Common::read_json_file($config_file);

	my $i = 0;
	my $last_error;
	while (1) {
		my $improved = improve_config($config_params);
		$i++;
		print "Finished iteration $i, current error is " . $improved->{error} . "\n";
		if ($i > 1 and $last_error < $improved->{error}) {
			print "Last error was $last_error\n";
			exit 0;
		}
		$last_error = $improved->{error};
		Common::write_json_file($config_file . ".new", $improved->{config});
		rename($config_file . ".new", $config_file);
		print $json->encode($improved->{config}) . "\n";
	}
}

sub improve_config {
	my ($config) = @_;
	my $best_error;
	my $best_config;
	my @configs = generate_candidate_configs($config);
	my %candidates = evaluate_configs(@configs);
	for my $k (keys %candidates) {
		if (!defined($best_error) or $candidates{$k} < $best_error) {
			$best_config = $configs[$k];
			$best_error = $candidates{$k};
		}
	}
	return { config => $best_config, error => $best_error };
}

sub write_config_set {
	my ($set, $config) = @_;
	for my $partition (@partitions) {
		$config->{cluster}{partitions_per_replica} = $partition;
		my $file = sprintf("config/config_%03d_%08d.json", $set, $partition);
		Common::write_json_file($file, $config);
	}
}

sub evaluate_configs {
	my @configs = @_;
	my $i = 0;
	foreach my $config (@configs) {
		write_config_set($i, {%{$config}});
		$i += 1;
	}
	print "Simulations are running ...\n";
	print `make $make_args errors.json`; # This is the ugly way to do it…
	my $results = Common::read_json_file("errors.json");
	my %candidates;
	for my $conf_file (keys $results) {
		my $set = 0 + (basename($conf_file) =~ s/.*config_(\d+)_.*/$1/r);
		$candidates{$set} //= [];
		push $candidates{$set}, @{$results->{$conf_file}}
	}
	for my $set (keys %candidates) {
		$candidates{$set} = sum(map { $_ ** 2 } @{$candidates{$set}}) / @{$candidates{$set}};
	}
	#exit 1;
	print `make clean`;
	return %candidates;
}

sub generate_candidate_configs {
	my ($base_config) = @_;
	my $params_file = "step.json";
	my $params = Common::read_json_file($params_file);
	my @configs;
	local *process = sub {
		my ($base, $params) = @_;
		my @new_configs = ();
		for my $k (keys $base) {
			die "Key $k missing in $params_file" unless exists $params->{$k};
			next unless (defined $params->{$k});
			die "Value of $k should be an object in $params_file" unless ref $params->{$k} eq "HASH";
			if (ref $base->{$k} eq 'HASH') {
				foreach my $c (process($base->{$k}, $params->{$k})) {
					my $new_config = { %{$base} };
					$new_config->{$k} = $c;
					push @new_configs, $new_config;
				}
			} else {
				if ($base->{$k} < $params->{$k}{max}) {
					my $new_val = $base->{$k} + ($base->{$k} * $params->{$k}{step});
					if ($new_val > $params->{$k}{max}) {
						$new_val = $params->{$k}{max};
					}
					my $new_config = { %{$base} };
					$new_config->{$k} = $new_val;
					push @new_configs, $new_config;
				}
				if ($base->{$k} > $params->{$k}{min}) {
					my $new_val = $base->{$k} - ($base->{$k} * $params->{$k}{step});
					if ($new_val < $params->{$k}{min}) {
						$new_val = $params->{$k}{min};
					}
					my $new_config = { %{$base} };
					$new_config->{$k} = $new_val;
					push @new_configs, $new_config;
				}
			}
		}
		return @new_configs;
	};
	return process($base_config, $params);
}
