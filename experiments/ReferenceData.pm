package ReferenceData;
use strict;
use warnings;

use Common;
use File::Basename;

my $module_path = File::Basename::dirname( eval { ( caller() )[1] } );

sub reference_stats {
    my ($config) = @_;

    if ($config->{client}{workload} eq "rotx_put_rr") {
        return reference_stats_rotx_put_rr($config);
    } else {
        die sprintf("Subroutine to load referece data for %s not implemented yet.",
                    $config->{client}{workload});
    }

}

sub index_rotx_put_rr_data {
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

my $indexed_rotx_pur_rr_data;
sub reference_stats_rotx_put_rr {
    my ($config) = @_;

    unless (defined $indexed_rotx_pur_rr_data) {
        my $data = Common::read_json_file("$module_path/ec2_rotx_data.json");
        $indexed_rotx_pur_rr_data = index_rotx_put_rr_data($data);
    }

    my $data = $indexed_rotx_pur_rr_data
                ->{$config->{application}{protocol}}
                ->{$config->{cluster}{partitions_per_replica}}
                ->{$config->{cluster}{clients_per_partition}}
                ->{$config->{workload}{rotx_put_rr}{"values per rotx"}};

    return {
        cluster => {
            throughput => $data->{throughput},
        },
        client => {
            "put latency" => $data->{put_latency},
            "rotx latency" => $data->{rotx_latency},
        }
    };
}

1;
