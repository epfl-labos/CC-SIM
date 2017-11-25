# Hill-climbing

Use a variation of [Smart Hill Climbing](https://dl.acm.org/citation.cfm?doid=988672.988711)
without importance sampling to find a good simulator configuration.

## Usage

`perl smart_hill_climbing.pl config.json hc_params.json`

`config.json` is used as a template together with the parameter ranges found in
`hc_params.json` to generate the configuration file for the simulator. The
parameters for the hill-climbing algorithm (like number of samples and how long
to run for) is at the top of `smart_hill_climbing.pl`.

`compute_errors.pl` is responsible for comparing the simulation results with
real results and yields the error value of the configuration. The hill-climbing
tries to minimise this error.
