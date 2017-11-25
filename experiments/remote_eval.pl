#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($Bin);
use lib "$Bin";
use EvaluateConfig;

EvaluateConfig::remotely_evaluate_configs($ARGV[0]);
