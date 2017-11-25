package EvaluateConfig;
use strict;
use warnings;

use threads;

use Clone qw(clone);
use JSON;
use List::Util qw(shuffle);

my @HOSTS = (
	{ name => "localhost", path => "/home/florian/sim2", username => "florian", make_args => [ "-j", "2" ] },
	{ name => "localhost", path => "/home/florian/sim3", username => "florian", make_args => [ "-j", "2" ] },
);

sub config_files {
	opendir(my $dh, "config");
	my @configs = grep /\.json$/, readdir($dh);
	closedir($dh);
	return @configs;
}

sub remotely_evaluate_configs {
	my ($directory) = @_;
	mkdir "outputs";

	my @configs = shuffle config_files;
	my $hosts = clone(\@HOSTS);

	# Split configurations list accross hosts
	$_->{path} .= "/" . $directory foreach @$hosts;
	while (my $c = shift @configs) {
		my $h = shift @$hosts;
		push @{$h->{configs}}, $c;
		push @$hosts, $h;
	}

	# Run simulations on each hosts
	my @threads;
	foreach my $host (@$hosts) {
		my $th = threads->create(\&evaluate_on_host, $host);
		unless (defined $th) {
			die "Couldn't create thread.";
		}
		push @threads, $th;
	}

	foreach my $th (@threads) {
		$th->join();
	}
}

sub evaluate_on_host {
	my ($host_info) = @_;
	my $host = $host_info->{name};
	my $user = $host_info->{username};
	my $path = $host_info->{path};
	my @config_files = map { "config/" . $_ } @{$host_info->{configs}};
	my $make_args = $host_info->{make_args};

	remote_clean($host, $user, $path);
	remote_send_config_files($host, $user, $path, @config_files);
	remote_run($host, $user, $path, $make_args);
	remote_retreive_outputs($host, $user, $path);
}

sub remote_clean {
	my ($host, $user, $path) = @_;
	my $script = <<EOF;
		cd "$path"
		make clean
EOF
	open(my $fh, "|-", "ssh", "-l", $user, $host, "/bin/sh");
	print $fh $script;
	close($fh);
}

sub remote_send_config_files {
	my ($host, $user, $path, @files) = @_;
	open(my $fh, "-|", "scp", @files, $user . "@" . $host . ":\"" . $path . "/config\"");
	while (<$fh>) {
		printf "%s: %s", $host, $_;
	}
	close($fh);
}

sub remote_run {
	my ($host, $user, $path, $make_args) = @_;
	open(my $fh, "-|", "ssh", "-l", $user, $host, "--", "make", "-C", $path, @$make_args, "run-all");
	while (<$fh>) {
		printf "%s: %s", $host, $_;
	}
	close($fh);
}

sub remote_retreive_outputs {
	my ($host, $user, $path) = @_;
	open(my $fh, "-|", "scp", "-p", "-r", "${user}\@${host}:\"${path}/outputs\"/*", "outputs/");
	while (<$fh>) {
		printf "%s: %s", $host, $_;
	}
	printf "%s: copied outputs.\n", $host;
	close($fh);
}
