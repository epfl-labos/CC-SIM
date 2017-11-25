#!/usr/bin/env perl
use strict;
use warnings;
use v5.20;

use Clone 'clone';
use File::Basename;
use List::Util qw(shuffle sum min max);
use Algorithm::CurveFit;
use Chart::Gnuplot;

use lib '../';
use Common;
use EvaluateConfig;
our $json;

my $config_file = $ARGV[0];
my $hc_params_file = $ARGV[1];

# Additional arguments to pass to make
my $make_args = "-j6";

# Whether to use ../EvaluateConfig.pm to run simulations on multiple hosts
my $parallel_remote_evaluation = 1;

# Whether to generate PNGs of the fittings
my $fitting_pngs = 0;

# Smart Hill Climbing parameters
my $initial_sample_size = 32;
my $next_sample_size = 64;
my $local_sample_size = 16;
my $local_region_size_factor = 0.015625;
my $local_region_shrink_factor = 0.50;
my $local_region_shrink_threshold = 0.0003125;
my $accept_new_point_factor = 0.8;
my $min_run_time = 10 * 60;

my $config_template = Common::read_json_file($config_file);
my $hc_params = Common::read_json_file($hc_params_file);

my $montage_count = 0;
my $evaluated_configurations = 0;

my @variant_params; # Configuration variants to evalue for all points

#smart_hill_climbing();
self_adaptive_step_size_search();

sub get_samples {
	my ($param, $samples) = @_;
	my $min = $param->{local_min} // $param->{min};
	my $max = $param->{local_max} // $param->{max};
	my $width = ($max - $min) / $samples;
	my @samples;
	for (1..$samples) {
		push @samples, $min + rand() * $width;
		$min += $width;
	}
	return @samples;
}

sub latin_hypercube_sampling {
	my ($parameters, $samples) = @_;
	my @intervals_indices;
	my @samples;
	for my $parameter (@$parameters) {
		$parameter->{samples} = [get_samples($parameter, $samples)];
	}
}

sub flatten_hc_params {
	my ($config, $hc_params) = @_;

	my $flatten_hc_params0;
	$flatten_hc_params0 = sub {
		my ($config, $hc_params, $path) = @_;
		my @ret = ();
		for my $k (keys %$hc_params) {
			next unless defined $hc_params->{$k};
			if (ref $hc_params->{$k} eq 'HASH') {
				if (ref $config->{$k} eq 'HASH') {
					# It's a hash in $config_file so it definitely contains other parameters
					push @ret, $flatten_hc_params0->($config->{$k}, $hc_params->{$k}, [@$path, $k]);
				} elsif (exists $hc_params->{$k}{min} and exists $hc_params->{$k}{max}) {
					# Range definition in $hc_params_file
					push @ret, {
						%{$hc_params->{$k}},
						keys => [@$path, $k],
					};
				} elsif (not exists $config->{$k}) {
					# Configuration object containing other parameters defined in
					# $hc_params_file only
					push @ret, $flatten_hc_params0->($config->{$k}, $hc_params->{$k}, [@$path, $k]);
				} else {
					die "$k is a configuration object in \"$hc_params_file\" but not in \"$config_file\"";
				}
			} elsif (ref $hc_params->{$k} eq 'ARRAY') {
				if (ref $config->{$k} eq '') {
					# It's an array with all values to be evaluated for each point
					push @variant_params, {
						keys => [@$path, $k],
						values => $hc_params->{$k},
					};
				} else {
					die "$k must not be a list in \"$hc_params_file\" as it is not a scalar in \"$config_file\"";
				}
			} elsif(not defined $hc_params->{$k}) {
				# Ignore parameters set to null
				next;
			} else {
				die "$k in \"$hc_params_file\" is expected to be an object or null";
			}
		}
		return @ret;
	};
	return $flatten_hc_params0->($config, $hc_params, []);
}

sub make_configs_from_samples {
	my ($config_template, $parameters) = @_;
	my @configs;
	while (@{$parameters->[0]{samples}}) {
		my $config = clone($config_template);
		for my $parameter (@$parameters) {
			my $ref = \$config;
			$ref = \$$ref->{$_} for (@{$parameter->{keys}});
			$$ref = shift @{$parameter->{samples}};
		}
		push @configs, $config;
	}
	delete $_->{samples} for @$parameters;
	return @configs;
}

sub set_center_from_result {
	my ($parameters, $result) = @_;
	for my $parameter (@$parameters) {
		my $ref = \$result->{config};
		$ref = \$$ref->{$_} for (@{$parameter->{keys}});
		$parameter->{center} = $$ref;
	}
}

sub is_better_than_fraction {
	my ($result, $fraction, $results) = @_;
	return 1 unless @$results;
	my $error = $result->{error};
	my $c = 0;
	$_->{error} > $error and ++$c for @$results;
	printf "best of new sample: %f, better than: %d/%d (%f)\n",
		$error, $c, scalar(@$results), ($c / @$results);
	return ($c / @$results) > $fraction;
}

sub is_best_result {
	my ($result, $results) = @_;
	for (@$results) {
		$_->{error} <= $result->{error} && printf " %f is better\n", $_->{error};
		return if $_->{error} <= $result->{error};
	}
	return 1;
}

sub sort_results {
	return sort { $a->{error} <=> $b->{error} } @_;
}

sub press_return {
	print "Press return to continue.";
	<STDIN>;
}

sub local_candidate_sample {
	my ($parameters, $local_results) = @_;
	my $formula = 'a * x^2 + b * x + c';
	my $variable = 'x';
	my @ydata = map { $_->{error} } @$local_results;
	for my $parameter (@$parameters) {
		my @xdata;
		for my $result (@$local_results) {
			my $ref = \$result->{config};
			$ref = \$$ref->{$_} for @{$parameter->{keys}};
			push @xdata, $$ref;
		}
		# FIXME set min_step to something > 0 to avoid doing many iterations
		my @curve_params = (
			['a', 1, 0],
			['b', 1, 0],
			['c', 1, 0],
		);
		my $square_residual = Algorithm::CurveFit->curve_fit(
			formula => $formula,
			params => \@curve_params,
			variable => $variable,
			xdata => \@xdata,
			ydata => \@ydata,
			maximum_iterations => 10,
		);

		my ($a, $b, $c) = map { $curve_params[$_][1] } 0..2;
		my $extrema_x = -$b / (2 * $a);
		my $candidate;
		if ($a > 0 ) {
			if ($extrema_x < $parameter->{local_min}) {
				$candidate = $parameter->{local_min};
			} elsif ($extrema_x > $parameter->{local_max}) {
				$candidate = $parameter->{local_max};
			} else {
				$candidate= $extrema_x;
			}
		} else {
			my $center = $parameter->{local_min}
				+ ($parameter->{local_max} - $parameter->{local_min}) / 2;
			if ($extrema_x > $center) {
				$candidate = $parameter->{local_min};
			} else {
				$candidate = $parameter->{local_max};
			}
		}
		$parameter->{samples} = [$candidate];

		next unless ($fitting_pngs);
		my $chart = Chart::Gnuplot->new(
			output => "fitting_" . $parameter->{keys}[$#{$parameter->{keys}}] . ".png",
			xtics => { labelfmt => '%.1e' },
			ytics => { labelfmt => '%.1e' },
		);
		my $data = Chart::Gnuplot::DataSet->new(
			xdata => \@xdata,
			ydata => \@ydata,
		);
		my $func = Chart::Gnuplot::DataSet->new(
			func => sprintf('%f * x**2 + %f * x + %f', $a, $b, $c),
		);
		my $range_data_set = Chart::Gnuplot::DataSet->new(
			xdata => [$parameter->{local_min}, $parameter->{center}, $parameter->{local_max}],
			ydata => [$ydata[0], $ydata[0], $ydata[0]],
		);
		my $extrema_y = $a * $candidate ** 2 + $b * $candidate + $c;
		my $candidate_data_set = Chart::Gnuplot::DataSet->new(
			xdata => [$candidate],
			ydata => [$extrema_y],
		);
		$chart->plot2d($data, $func, $candidate_data_set, $range_data_set);
	}

	return unless($fitting_pngs);
	my $count = sprintf "%03d", $montage_count;
	`montage fitting_*.png -label '%f' -geometry 600x600+5+5 -tile 3x fitting-$count.png`;
	$montage_count++;
	#press_return();
}

sub set_local_region {
	my ($parameters, $size_factor) = @_;
	for my $parameter (@$parameters) {
		my $radius = ($parameter->{max} - $parameter->{min}) * $size_factor / 2;
		my $min = $parameter->{center} - $radius;
		my $max = $parameter->{center} + $radius;
		my $shift = 0;
		if ($min < $parameter->{min}) {
		#	$shift = $parameter->{min} - $min;
			$min = $parameter->{min};
		} elsif ($max > $parameter->{max}) {
		#	$shift = $parameter->{max} - $max;
			$max = $parameter->{max};
		}
		$parameter->{local_min} = $min + $shift;
		$parameter->{local_max} = $max + $shift;
		#print join('->', @{$parameter->{keys}}) . " center: " . $parameter->{center} . " radius: " . $radius . "\n";
	}
}

sub local_search {
	my ($parameters, $results, $center) = @_;
	$parameters = clone($parameters);
	# Results are sorted, first one is the best one
	set_center_from_result($parameters, $center);
	my $size_factor = $local_region_size_factor;
	my @local_results;
	while (1) {
		print ">> Local search ($size_factor)\n";
		set_local_region($parameters, $size_factor);
		latin_hypercube_sampling($parameters, $local_sample_size);
		my @configs = make_configs_from_samples($config_template, $parameters);
		my @new_results = sort_results(evaluate_configs(@configs));
		push @local_results, @new_results;
		local_candidate_sample($parameters, \@new_results), ;
		my @local_candidate_config = make_configs_from_samples($config_template, $parameters);
		my $local_candidate = (evaluate_configs(@local_candidate_config))[0];
		my $candidate_is_best_local = is_best_result($local_candidate, \@local_results);
		@new_results = sort_results(@new_results, $local_candidate);
		push @local_results, $local_candidate;
		if ($candidate_is_best_local) {
			printf "   First candidate is best local (%f)\n", $local_candidate->{error};
			set_center_from_result($parameters, $local_candidate);
		} else {
			printf "   First candidate: %f, best of sample: %f\n", $local_candidate->{error}, $new_results[0]{error};
			local_candidate_sample($parameters, \@new_results);
			@local_candidate_config = make_configs_from_samples($config_template, $parameters);
			$local_candidate = (evaluate_configs(@local_candidate_config))[0];
			$candidate_is_best_local = is_best_result($local_candidate, \@local_results);
			push @local_results, $local_candidate;
			if ($candidate_is_best_local) {
				printf "   Second candidate is best local (%f)\n", $local_candidate->{error};
				set_center_from_result($parameters, $local_candidate);
			} else {
				printf "   Second candidate: %f, best of sample: %f\n", $local_candidate->{error}, $new_results[0]{error};
				print "   Narrowing local region (both candidate are worst than best local)\n";
				$size_factor *= $local_region_shrink_factor;
				if ($size_factor < $local_region_shrink_threshold) {
					print "   Local region reached size threshold\n";
					@local_results = sort_results(@local_results);
					push @$results, @local_results;
					@$results = sort_results(@$results);
					return $local_results[0];
				}
			}
		}
	}
}

# See http://dl.acm.org/citation.cfm?doid=988672.988711
sub smart_hill_climbing {
	my @parameters = flatten_hc_params($config_template, $hc_params);
	my $sample_size = $initial_sample_size;
	my @results;

	my $rounds = 0;
	my $start_time = time;
	while (1) {
		++$rounds;
		latin_hypercube_sampling(\@parameters, $sample_size);
		my @configs = make_configs_from_samples($config_template, \@parameters);
		my @new_results = sort_results(evaluate_configs(@configs));
		my $last_best = $results[0];
		my $local_best;

		if (is_better_than_fraction($new_results[0], $accept_new_point_factor, \@results)) {
			@results = sort_results(@results, @new_results);
			$local_best = local_search(\@parameters, \@results, $new_results[0]);
		}

		Common::write_json_file("current_best.json", $results[0]{config});
		if ($rounds >= 2 and (time - $start_time) > $min_run_time){
			my $exit;
			if (defined $local_best and $local_best->{error} > 0.98 * $last_best->{error}) {
				print ">> No significative improvement, exiting\n";
				$exit = 1;
			} elsif ($rounds > 8) {
				print ">> No new interesting starting point found, exiting\n";
				$exit = 1;
			}
			if ($exit) {
				printf ">> best error: %.2f (config in current_best.json)\n", $results[0]{error};
				exit 0;
			}
		}
		printf ">> Restart, ran for %ds so far, %d configurations evaluated, %d rounds done, current best is:\n",
			(time - $start_time), $evaluated_configurations, $rounds;
		print $json->encode($results[0]) . "\n";
		$sample_size = $next_sample_size;
	}
}

# See http://link.springer.com/chapter/10.1007/3-540-34783-6_56
sub self_adaptive_step_size_search {
	my @parameters = flatten_hc_params($config_template, $hc_params);
	my $sample_size = $initial_sample_size;

	# Initialize $sample_size points
	latin_hypercube_sampling(\@parameters, $sample_size);
	my @points = evaluate_configs(make_configs_from_samples($config_template, \@parameters));
	my $best_error = (sort_results(@points))[0]->{error};

	my $rounds = 0;
	my $start_time = time;
	while (1) {
		++$rounds;
		my @new_configs;
		foreach my $point (@points) {
			# Select another point at random
			my $other_point;
			do {
				$other_point = $points[rand(@points)];
			} while ($point == $other_point);
			# Walk a random distance toward the selected point
			push @new_configs, make_config_sass_step(\@parameters, $point, $other_point);
		}
		my @new_points = evaluate_configs(@new_configs);
		my $points_moved = 0;
		foreach my $i (0..$#points) {
			# replace the old point by the new one if it is better
			if ($points[$i]->{error} > $new_points[$i]->{error}) {
				$points[$i] = $new_points[$i];
				$points_moved += 1;
			}
		}
		my @sorted = sort_results(@points);
		my $error = $sorted[0]{error};
		Common::write_json_file("current_best.json", $sorted[0]{config});
		my $message = "";
		if ($error < $best_error) {
			rename_outputs($rounds, $sorted[0]);
			$message = sprintf ", improvement of %g (×%.2f) from last round", $best_error - $error, $error / $best_error;
			$best_error = $error;
		}
		printf ">> %d configurations evaluated, best error: %.3g (config in current_best.json), %d points improved%s\n",
				$evaluated_configurations, $error, $points_moved, $message;
	}
}


# Return a reference to a value stored in a nested hash structure
# Example: nested_hash_get_ref($hash, "foo", "bar") is the same has \$hash->{"foo"}{"bar"}
sub nested_hash_get_ref {
	my ($hash, @keys) = @_;
	my $ref = \$hash;
	$ref = \$$ref->{$_} for @keys;
	return $ref;
}
use Data::Dumper;
$Data::Dumper::Sortkeys = 1;

sub make_config_sass_step {
	my ($parameters, $a, $b) = @_;
	my $config = clone($a->{config});
	for my $parameter (@$parameters) {
		my @keys = @{$parameter->{keys}};
		my $val_a = ${nested_hash_get_ref($a->{config}, @keys)};
		my $val_b = ${nested_hash_get_ref($b->{config}, @keys)};
		my $ref = nested_hash_get_ref($config, @keys);
		my $new_val = $val_a + (rand(2) - 1) * ($val_b - $val_a);
		$new_val = max($parameter->{min}, $new_val);
		$new_val = min($parameter->{max}, $new_val);
		$$ref = $new_val;
	}
	return $config;
}

sub write_config_set {
	my ($set, $config) = @_;

	my $i = 0;
	my $write_config = sub {
		my $file = sprintf("config/config_%03d_%03d.json", $set, $i);
		Common::write_json_file($file, $config);
		$i += 1;
	};

	my $rec;
	$rec = sub {
		my $p = shift @_;
		if (defined $p) {
			foreach my $value (@{$p->{values}}) {
				my $ref = \$config;
				$ref = \$$ref->{$_} for (@{$p->{keys}});
				$$ref = $value;
				$rec->(@_);
			}
		} else {
			$write_config->();
		}
	};

	$rec->(@variant_params);
	return $i;
}

sub evaluate_configs {
	my @configs = @_;

	`make clean`;
	die "make clean failed" if $?;

	my $i = 0;
	my $num_simulations = 0;

	foreach my $config (@configs) {
		die unless defined $config;
		$num_simulations += write_config_set($i, $config);
		$i += 1;
	}
	printf "Simulations are running (%d) ...\n", $num_simulations;

	if ($parallel_remote_evaluation) {
		EvaluateConfig::remotely_evaluate_configs("experiments/hc");
	}

	`make $make_args errors.json`;
	die "make failed" if $?;

	my $errors = Common::read_json_file("errors.json");
	my %set_results;
	my %set_directories;
	for my $output_dir (keys %$errors) {
		my $set = 0 + (basename($output_dir) =~ s/.*config_(\d+)_.*/$1/r);
		$set_results{$set} //= [];
		push @{$set_results{$set}}, @{$errors->{$output_dir}};
		$set_directories{$set} //= [];
		push @{$set_directories{$set}}, $output_dir;
	}
	my @results;
	for my $set (keys %set_results) {
		push @results, {
			#error => sum(map { $_ ** 2 } @{$set_results{$set}}) / @{$set_results{$set}},
			error  => sum(map { abs $_ } @{$set_results{$set}}) / @{$set_results{$set}},
			errors => $set_results{$set},
			config => $configs[$set],
			output_dirs => $set_directories{$set},
		};
	}
	$evaluated_configurations += @configs;
	return @results;
}

# Saves the correct output directory only if $result was returned by the most recent
# call to evaluate_configs()
sub rename_outputs {
	my ($name, $result) = @_;
	mkdir "saved/";
	my $directory = "saved/" . time . "_" . $name;
	mkdir $directory or warn "Couldn't mkdir $directory: $!";
	for my $d (@{$result->{output_dirs}}) {
		rename $d, $directory . "/" . basename($d) or warn "Couldn't rename output directory";
	}
}
