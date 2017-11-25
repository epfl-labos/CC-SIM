#!/usr/bin/env perl
use strict;
use warnings;

use File::Basename;
use File::Glob ':bsd_glob';
use List::Util qw(min max);

use lib '../';
use Common;

my $outputs_dir = "outputs";
my $errors_file = "errors.json";

sub error_percentage {
    my ($target, $value) = @_;
    #return ($target - $value) / max($target, $value);
    return ($target - $value) / $target;
}

my %targets = (
    "get_put_rr" => {
        "gr" => Common::read_json_file("../gr_ec2_data_2.json"),
        "grv" => Common::read_json_file("../grv_ec2_data_2.json"),
    },
    "rotx_put_rr" => index_rotx_put_data(Common::read_json_file("../ec2_rotx_data.json")),
);

sub index_rotx_put_data {
    my ($doc) = @_;
    my %h;
    foreach my $e (@$doc) {
        $h{$e->{protocol}}{$e->{partitions}}{$e->{clients_per_partition}}{$e->{keys_per_rotx}} = {
            throughput => $e->{throughput},
            rotx_latency => $e->{rotx_latency},
            put_latency => $e->{put_latency},
        };
    }
    return \%h;
}

sub compute_error_get_put_rr {
    my ($config, $stats) = @_;
    my $protocol = $config->{application}{protocol};
    my $partitions = $config->{cluster}{partitions_per_replica};
    return [
        #error_percentage(
        #    get_target("get_put_rr", $protocol, $partitions, "throughput"),
        #    $stats->{cluster}{"throughput"}),
        error_percentage(
            get_target("get_put_rr", $protocol, $partitions, "get_latency"),
            $stats->{client}{"get average latency"}),
        error_percentage(
            get_target("get_put_rr", $protocol, $partitions, "put_latency"),
            $stats->{client}{"put average latency"}),
    ];
}

sub compute_error_rotx_put_rr {
    my ($config, $stats) = @_;
    my @target_args = (
        "rotx_put_rr",
        $config->{application}{protocol},
        $config->{cluster}{partitions_per_replica},
        $config->{cluster}{clients_per_partition},
        $config->{workload}{rotx_put_rr}{"values per rotx"},
    );

    my $rotx_latency = $stats->{client}{"rotx average latency"};
    my $put_latency  = $stats->{client}{"put average latency"};
    my $expected_rotx_latency = get_target(@target_args, "rotx_latency");
    my $expected_put_latency  = get_target(@target_args, "put_latency");

    my $latency_avg = ($rotx_latency + $put_latency) / 2;
    my $expected_latency_avg = ($expected_rotx_latency + $expected_put_latency) / 2;

    return [
            ($latency_avg - $expected_latency_avg) / $expected_latency_avg,
    ];
}

sub get_target {
    my $ref = \%targets;
    for my $k (@_) {
        $ref = $ref->{$k};
    }
    unless (defined $ref) {
        die "No target for " . join(", ", @_);
    }
    return $ref;
}

my %errors;
for my $dir (bsd_glob("$outputs_dir/*")) {
    my $config = Common::read_json_file($dir . "/config.json");
    my $stats = Common::read_json_file($dir . "/stats.json");
    my $workload = $config->{client}{workload};

    if ($workload eq "get_put_rr") {
        $errors{$dir} = compute_error_get_put_rr($config, $stats);
    } elsif ($workload eq "rotx_put_rr") {
        $errors{$dir} = compute_error_rotx_put_rr($config, $stats);
    } else {
        die "Cannot compute error with \"$workload\" workload.";
    }
}

Common::write_json_file($errors_file, \%errors);
