#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw "$Bin";
use lib "$Bin";
use Common;

my $doc = Common::read_json_file($ARGV[0]);
print $Common::json->encode($doc);
