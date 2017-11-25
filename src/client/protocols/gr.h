#ifndef client_protocols_gr_h
#define client_protocols_gr_h

#include "protocols.h"

#ifdef client_protocols_gr_c
#define SCLASS
#else
#define SCLASS extern const
#endif
client_functions_t gr_client_funcs;
#undef SCLASS

#endif
