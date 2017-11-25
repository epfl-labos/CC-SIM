/* gentle_rain.{c,h}
 *
 * Gentle Rain-related structures, types and functions.
 *
 * The payload of a ROOT-Sim event must be a contiguous region of memory. To
 * comply with this restriction, some of the following structures have the
 * arrays trailing them.
 */

#ifndef gentle_rain_h
#define gentle_rain_h

#include "common.h"
#include <ROOT-Sim.h>
#include <limits.h>
#include <stdint.h>

#define GR_VALUE_SIZE              1
#define GR_SIMULATED_VALUE_SIZE   64

#define NO_PROXY_LPID UINT_MAX

#endif
