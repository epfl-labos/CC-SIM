#!/usr/bin/env perl
use strict;

use Cwd qw(abs_path);
use File::Basename qw(dirname);
use JSON;

use lib dirname($0);
use Common;

chdir $ARGV[0] if ($ARGV[0]);

sub sum {
	my $sum = 0;
	$sum += $_ foreach (@_);
	return $sum;
}

sub average {
	return sum(@_) / scalar(@_);
}

sub standard_deviation {
	my $average = average(@_);
	return sqrt(sum(map {($_ - $average)**2} @_) / scalar(@_));
}

sub client_stats {
	my @client_files = grep {/r\d+_c\d+_stats.json/} glob('r*_c*_stats.json');
	my %stats;
	for my $f (@client_files) {
		my $data = Common::read_json_file($f);
		for my $k (keys %$data) {
			next if $k eq "name";
			push @{$stats{$k . " average latency"}}, $data->{$k}{"average latency"};
			push @{$stats{$k . " rate"}}, $data->{$k}{"rate"};
		}
	}

	my %client;
	$client{$_} = average(@{$stats{$_}}) for keys %stats;
	$client{throughput} = $client{"get rate"} + $client{"put rate"} + $client{"rotx rate"};
	return \%client;
}

sub average_line_value
{
	my $sum = 0;
	my $num = 0;
	my @size_samples;
	for (@_) {
		open(my $fh, "<", $_) or die "Can't open $_ for reading.";
		while (<$fh>) {
			$sum += $_;
			$num++;
		}
	}
	return 0 if $num == 0;
	return $sum / $num;
}

sub server_stats {
	my @base_file_names = map {s/\.json//r} grep {/r\d+_p\d+.json$/} glob('r*_p*.json');
	my %stats;
	for my $basename (@base_file_names) {
		my $f = $basename . ".json";
		my $data = Common::read_json_file($f);
		for my $k (keys %{$data->{stats}}) {
			if (exists $data->{stats}{$k}{average}) {
				push @{$stats{$k}}, $data->{stats}{$k}{average};
			} elsif (exists $data->{stats}{$k}{rate}) {
				push @{$stats{$k . " rate"}}, $data->{stats}{$k}{rate};
			} else {
				warn "I don't know how to aggregate $k defined in  $f.";
			}
		}
		push @{$stats{"cpu usage"}}, $data->{cpu}{usage};
	}
	my %server;
	$server{$_} = average(@{$stats{$_}}) for keys %stats;

	$server{average_cpu_queue_size} = average_line_value(map {$_ . '_cpu_queue_size'} @base_file_names);
	$server{average_max_cpu_queue_size} = average_line_value(map {$_ . '_cpu_max_queue_size'} @base_file_names);

	my %cluster;
	for my $k (keys %stats) {
		next unless ($k =~ /requests/);
		$cluster{$k} = sum(@{$stats{$k}});
	}
	$cluster{throughput} = $cluster{"get requests rate"}
	                     + $cluster{"put requests rate"}
	                     + $cluster{"roxt requests rate"};
	return \%server, \%cluster;
}

sub app_infos {
	open(my $fh, "<", "startup");
	my $command = <$fh>;
	chomp($command);
	<$fh> =~ /Started on (\d+-\d+-\d+ \d+:\d+:\d+) revision (.*)/;
	close($fh);
	my $config = Common::read_json_file("config.json");
	return {
		command => $command,
		date => $1,
		revision => $2,
		protocol => $config->{application}{protocol},
		workload => $config->{client}{workload},
	};
}

my $json = JSON->new;
$json->pretty(1);
$json->canonical(1);
my ($server_stats, $cluster_stats) = server_stats();
print $json->encode({
	application => app_infos(),
	client => client_stats(),
	server => $server_stats,
	cluster => $cluster_stats,
});
