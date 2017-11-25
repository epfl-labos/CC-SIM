#define client_workloads_c
#include "client/workloads.h"
#include "client/workloads/get_put_rr.h"
#include "client/workloads/probabilistic.h"
#include "client/workloads/rotx_put_rr.h"

/** .. c:var:: workload_definitions
 *
 * The list of workloads. Each entry consists of a name and a pointer to the
 * :c:type:`workload_functions_t` structure of the workload.
 */
static struct workload workload_definitions[] = {
	{"get_put_rr",            &workload_get_put_rr_funcs},
	{"probabilistic",         &workload_probabilistic_funcs},
	{"rotx_put_rr",           &workload_rotx_put_rr_funcs},
	{NULL, NULL} // Must be the last entry
};

struct workload *workloads = &workload_definitions[0];
