#ifndef parameters_h
#define parameters_h

#include "gentle_rain.h"
#include <ROOT-Sim.h>
#include <sys/types.h>

simtime_t cpu_stats_interval;

simtime_t build_struct_per_byte_time(void);

/** .. c:macro:: DEFINE_PROTOCOL_TIMING_FUNC
 *
 *  The macro ``DEFINE_PROTOCOL_TIMING_FUNC(name, protocol)`` defines a
 *  function ``double name(void)`` that return a value drawn from the
 *  configured timing distribution for the value of `timing/protocol/name` in
 *  the configuration file. To define the function `static`, prepend `static`
 *  to the macro call.
 */
#define DEFINE_PROTOCOL_TIMING_FUNC(name, protocol) \
	double name(void) \
	{ \
		static simtime_t value; \
		static simtime_t valid; \
		if (!valid) { \
			struct json_object *obj = param_get_object_root("timing"); \
			obj = param_get_object(obj, protocol); \
			value = param_get_double(obj, STRINGIFY(name)); \
			valid = 1; \
		} \
		return timing_distribution(value); \
	}

/** .. c:macro:: DEFINE_TIMING_FUNC
 *
 *  The macro ``DEFINE_TIMING_FUNC(name)`` defines a function ``double
 *  name(void)`` that returns a value drawn from the configured timing
 *  distribution for the value of `timing/name` in the configuration file.
 *  To define the function `static`, prepend `static` to the macro call.
 *
 *  Protocol implementation should use :c:macro:`DEFINE_PROTOCOL_TIMING_FUNC`
 *  instead.
 */
#define DEFINE_TIMING_FUNC(name) \
	double name(void) \
	{ \
		static simtime_t value; \
		static simtime_t valid; \
		if (!valid) { \
			struct json_object *timing_obj = param_get_object_root("timing"); \
			value = param_get_double(timing_obj, STRINGIFY(name)); \
			valid = 1; \
		} \
		return timing_distribution(value); \
	}

/** .. c:macro:: DEFINE_PARAMETER_FUNC
 *
 *  The macro ``DEFINE_PARAMETER_FUNC(name, type, lv1)`` defines a function
 *  ``type name(void)`` returning the value of the parameter `lv1/name`. The
 *  function looks up the JSON once and caches the value for subsequent calls.
 *  To define the function `static`, prepend `static` to the macro call.
 *
 */
#define DEFINE_PARAMETER_FUNC(name, type, lv1) \
	type name(void) \
	{ \
		static simtime_t value; \
		static simtime_t valid; \
		if (!valid) { \
			struct json_object *obj = param_get_object_root(lv1); \
			value = param_get_##type(obj, STRINGIFY(name)); \
			valid = 1; \
		} \
		return value; \
	}

/** .. c:macro:: DEFINE_PARAMETER_FUNC_2
 *
 *  The macro ``DEFINE_PARAMETER_FUNC_2(name, type, lv1, lv2)`` defines a function
 *  ``type name(void)`` returning the value of the parameter `lv1/lv2/name`. The
 *  function looks up the JSON once and caches the value for subsequent calls.
 *  To define the function `static`, prepend `static` to the macro call.
 *
 *  Te define a protocol parameter, use
 *  :c:macro:`DEFINE_PROTOCOL_PARAMETER_FUNC` instead.
 */
#define DEFINE_PARAMETER_FUNC_2(name, type, lv1, lv2) \
	type name(void) \
	{ \
		static simtime_t value; \
		static simtime_t valid; \
		if (!valid) { \
			struct json_object *obj = param_get_object_root(lv1); \
			obj = param_get_object(obj, lv2); \
			value = param_get_##type(obj, STRINGIFY(name)); \
			valid = 1; \
		} \
		return value; \
	}

/** .. c:macro:: DEFINE_PROTOCOL_PARAMETER_FUNC
 *
 *  The macro ``DEFINE_PROTOCOL_PARAMETER_FUNC(name, type, protocol_name)``
 *  defines a function ``type name(void)`` returning the value of the parameter
 *  `protocol/protocol_name/name`. The function looks up the JSON once and
 *  caches the value for subsequent calls.
 *
 *  To define the function `static`, prepend `static` to the macro call.
 */
#define DEFINE_PROTOCOL_PARAMETER_FUNC(name, type, protocol) \
	DEFINE_PARAMETER_FUNC_2(name, type, "protocol", protocol)

struct app_parameters {
	struct json_object *json_config;
	simtime_t stop_after_simulated_seconds;
	time_t stop_after_real_time;
	simtime_t ignore_initial_seconds;
};

void parameters_read_from_file(const char *path,
		struct app_parameters *app_params);

/** .. c:function:: double timing_distribution(double value)
 *
 * Draw a timing parameters with the given value from the configured timing
 * distribution (see :ref:`common_timing_parameters`). You almost certainly want to use
 * :c:macro:`DEFINE_PROTOCOL_TIMING_FUNC` instead.
 */
double timing_distribution(double value);

struct json_object *param_get(struct json_object *obj, const char *name);
struct json_object *param_get_object(struct json_object *obj, const char *name);
struct json_object *param_get_object_root(const char *name);
struct json_object *param_get_workload_object(const char *name);
struct json_object *param_get_timing_protocol_object(const char *protocol);
double param_get_double(struct json_object *obj, const char *name);
int param_get_int(struct json_object *obj, const char *name);
unsigned int param_get_uint(struct json_object *obj, const char *name);
const char *param_get_string(struct json_object *obj, const char *name);
struct json_object *param_get_double_matrix(struct json_object *obj,
		const char *name, unsigned int width, unsigned int height);
double param_get_double_matrix_element(struct json_object *obj,
		unsigned int column, unsigned int row);

#endif
