#ifndef client_protocols_grv_h
#define client_protocols_grv_h

#include "protocols.h"

#ifdef client_protocols_grv_c
#define SCLASS
#else
#define SCLASS extern const
#endif
client_functions_t grv_client_funcs;
#undef SCLASS

#endif
