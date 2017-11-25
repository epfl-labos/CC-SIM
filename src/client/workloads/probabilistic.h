#ifndef client_workloads_probabilistic_h
#define client_workloads_probabilistic_h

#include "../client.h"

#if defined client_workloads_probabilistic_c
#define SCLASS extern
#else
#define SCLASS
#endif
SCLASS workload_functions_t workload_probabilistic_funcs;
#undef SCLASS

#endif
