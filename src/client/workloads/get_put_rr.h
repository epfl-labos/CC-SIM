#ifndef client_workloads_get_put_rr_h
#define client_workloads_get_put_rr_h

#include "client/workloads.h"

#if defined client_workloads_get_put_rr_c
#define SCLASS extern
#else
#define SCLASS
#endif
SCLASS workload_functions_t workload_get_put_rr_funcs;
#undef SCLASS

#endif
