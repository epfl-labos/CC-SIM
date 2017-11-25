#define protocols_c
#include "client/protocols/gr.h"
#include "client/protocols/grv.h"
#include "server/protocols/gr/gr.h"
#include "server/protocols/grv/grv.h"
#include "protocols.h"

/** .. c:var:: protocol_definitions
 *
 *  This defines the protocol names recognized by the application and which
 *  functions are implementing them.
 */
protocol_functions_t protocol_definitions[] = {
	{"gr",  &gr_client_funcs,  &gr_server_funcs},
	{"grv", &grv_client_funcs, &grv_server_funcs},
	{NULL, NULL, NULL}, // Must be the last entry
};

protocol_functions_t *protocols = &protocol_definitions[0];
client_functions_t *client_funcs;
server_functions_t *server_funcs;
