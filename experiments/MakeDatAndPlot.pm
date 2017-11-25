package MakeDatAndPlot;
use strict;
use warnings;
use Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(make_dat_and_plot);

use File::Glob ':bsd_glob';

use Common;

sub make_dat_and_plot {
	my %args = @_;

	my %table_rows_by_protocol;

	for my $d (bsd_glob($args{outputs_dir} . "/*/")) {
		my $info = {
			config => Common::read_json_file($d . "config.json"),
			stats  => Common::read_json_file($d . "stats.json"),
		};
		my $protocol = $info->{config}{application}{protocol};
		my @row;
		foreach my $column (@{$args{columns}}) {
			my $ref = $info;
			$ref = $ref->{$_} foreach @{$column->{path}};
			push @row, $ref;
		}
		push @{$table_rows_by_protocol{$protocol}}, \@row;
	}

	my @plots;

	my @column_names = map { $_->{name} } @{$args{columns}};

	for my $protocol (keys %table_rows_by_protocol) {
		# Write .dat files with values from the simulator runs
		my $file = $protocol . ".dat";
		my $header = "Summary for $protocol";
		my $rows = $table_rows_by_protocol{$protocol};
		unshift @$rows, \@column_names;
		write_dat_file($file, $header, $rows);

		# Write _ref.dat files with values from the real system
		my $ref_file = $protocol . "_ref.dat";
		$header = "Reference data from real system for $protocol";
		if (defined $args{ref_data}->{$protocol}) {
			write_ref_dat_file($ref_file, $header, $args{ref_data}->{$protocol},
					$args{x_axis_column_name});
		}

		# Populate @plots
		for my $column (@{$args{columns}}) {
			next if $column->{name} eq $args{x_axis_column_name};
			my %plot = (
				title => $protocol,
				x_label => $args{x_axis_column_name},
				y_label => $column->{name},
				lines => [{
					title => "sim",
					file => $file,
					y_column => $column->{name},
					x_column => $args{x_axis_column_name},
					color => "red",
				}],
			);
			if (defined $column->{ref_name}) {
				push @{$plot{lines}}, {
					title => "ref",
					file => $ref_file,
					y_column => $column->{ref_name},
					x_column => $args{x_axis_column_name},
					color => "blue",
				}
			}
			push @plots, \%plot;
		}
	}

	write_plots_to_tex_file("plots_fragment.tex", \@plots);
}

sub format_dat_row {
	my ($columns) = @_;
	return join("\t", @{$columns});
}

sub write_dat_file {
	my ($file, $header, $table_rows) = @_;
	open(my $fh, ">", $file) or die "Couldn't open $file: $!";
	printf $fh "# %s\n", $header;
	foreach my $row (@{$table_rows}) {
		printf $fh "%s\n", format_dat_row($row);
	}
}

sub write_ref_dat_file {
	my ($file, $header, $json, $x_axis_column_name) = @_;
	open(my $fh, ">", $file) or die "Couldn't open $file: $!";
	printf $fh "# %s\n", $header;

	# Lookup any configuration to get the column names
	my @names = sort(keys(%{(values(%{$json}))[0]}));

	printf $fh "%s\n", format_dat_row([$x_axis_column_name, @names]);
	foreach my $partitions (sort { $a <=> $b } keys %{$json}) {
		my @columns = ( $partitions );
		push @columns, $json->{$partitions}{$_} foreach @names;
		printf $fh "%s\n", format_dat_row(\@columns);
	}
}

sub format_pgf_addplots {
	my ($lines) = @_;
	my $s;
	foreach my $line (@{$lines}) {
		$s .= <<EOF;

\\addplot [
  $line->{color},
  mark=*,
  mark size=1pt,
] table [
  x expr=\\thisrow{$line->{x_column}},
  y expr=\\thisrow{$line->{y_column}},
] {$line->{file}};
\\addlegendentry{$line->{title}}
EOF
	}
	return $s;
}

# This currently doesn't excape everything that may need to be escaped
sub latex_escape {
	return map { $_ =~ s/_/\\_/r } @_;
}

sub format_figure {
	my ($plot) = @_;
	my $figure = <<EOF;
\\begin{figure}[ht]
\\begin{center}
\\begin{tikzpicture}
\\pgfplotsset{every axis legend/.append style={
    at={(1.0,-0.1)},
    anchor=north east}}
\\begin{axis}[
  title=${\(latex_escape($plot->{title}))},
  xlabel=${\(latex_escape($plot->{x_label}))},
  ylabel=${\(latex_escape($plot->{y_label}))},
  width=.9\\textwidth,
  legend columns=6,
]
${\(format_pgf_addplots($plot->{lines}))}
\\end{axis}
\\end{tikzpicture}
\\end{center}
\\end{figure}
EOF
	return $figure;
}

sub write_plots_to_tex_file {
	my ($file, $plots) = @_;
	open(my $fh, ">", $file) or die "Couldn't open $file: $!";
	foreach my $plot (@$plots) {
		print $fh format_figure($plot);
	}
}

