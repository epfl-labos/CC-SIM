package Common;
use strict;
use warnings;
use JSON;

our $json = JSON->new->pretty->canonical;

sub read_json_file {
	my ($file) = @_;
	open(my $fh, "<", $file) or die "Couldn't open $file: $!";
	local $/;
	$/ = undef;
	my $doc = $json->decode(<$fh>);
	close($fh);
	return $doc;
}

sub write_json_file {
	my ($file, $doc) = @_;
	open(my $fh, ">", $file) or die "Couldn't open $file: $!";
	print $fh $json->encode($doc);
	close($fh);
}

sub get_config {
	my ($directory) = @_;
	my $config_file = $directory . "/config.json";
	return read_json_file($config_file);
}

sub get_stats {
	my ($directory) = @_;
	my $stats_file = $directory . "/stats.json";
	return read_json_file($stats_file);
}

1;
