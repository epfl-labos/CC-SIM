#include "parameters.h"
#include "output.h"
#include "protocols.h"
#include <assert.h>
#include <errno.h>
#include <json.h>
#include <limits.h>
#include <string.h>
#include <time.h>

// FIXME can this be generalised to a useful settings?
simtime_t cpu_stats_interval = 0.1;

#define config_error(fmt, ...) \
	do { \
		fprintf(stderr, "Configuration error: " fmt "\n", ##__VA_ARGS__); \
		exit(1); \
	} while (0)

struct timing_distribution_info {
	const char *name;
	double (*func)(double);
};

static double (*timing_distribution_func)(double) = NULL;

double timing_distribution(double t)
{
	if (timing_distribution_func != NULL) {
		return timing_distribution_func(t);
	} else {
		return t;
	}
}

static DEFINE_PARAMETER_FUNC(normal_mu, double, "timing")
static DEFINE_PARAMETER_FUNC(normal_sigma, double, "timing")
static double normal_timing_distribution(double t)
{
	double f = (Normal() * normal_sigma()) + normal_mu() ;
	if (f < 0.5) {
		f = 0.5;
	} else if (f > 1.5) {
		f = 1.5;
	}
	return t * f;
}

static double exponential_timing_distribution(double t)
{
	return Expent(t);
}

#define TIMING_DISTRIBUTIONS_SIZE 3
static const struct timing_distribution_info
timing_distributions[TIMING_DISTRIBUTIONS_SIZE] = {
	{"constant", NULL},
	{"normal", normal_timing_distribution},
	{"exponential", exponential_timing_distribution},
};

DEFINE_TIMING_FUNC(build_struct_per_byte_time);

static double param_get_double_default(struct json_object *obj, const char *name,
		double default_value);

static void set_application_parameters(struct app_parameters *app)
{
	/* "application" object and app_params global variable */
	json_object *application_obj = param_get_object_root("application");
	app->stop_after_simulated_seconds = param_get_double_default(
			application_obj, "stop_after_simulated_seconds", 0);
	double stop_after_real_time_seconds = param_get_double_default(
			application_obj, "stop_after_real_time_seconds", 0);
	if (stop_after_real_time_seconds > 0) {
		app->stop_after_real_time =
			time(NULL) + (unsigned int) stop_after_real_time_seconds;
	} else {
		app->stop_after_real_time = 0;
	}
	app->ignore_initial_seconds = param_get_double_default(
			application_obj, "ignore_initial_seconds", 0);

	const char *protocol = param_get_string(application_obj, "protocol");
	for (int i = 0; ; ++i) {
		protocol_functions_t *e = &(protocols[i]);
		if (e->name == NULL) {
			config_error("protocol \"%s\" is unknown", protocol);
		} else if (!strcmp(protocol, e->name)) {
			client_funcs = e->client;
			server_funcs = e->server;
			return;
		}
	}
}

static void update_application_object(json_object *jobj)
{
	json_object *application_obj = NULL;
	if (!json_object_object_get_ex(jobj, "application", &application_obj)) {
		application_obj = json_object_new_object();
		json_object_object_add(jobj, "application", application_obj);
	}

	/* Add Git revision */
	json_object *revision = json_object_new_string(STRINGIFY(REVISION));
	json_object_object_add(application_obj, "revision", revision);
}

static void set_timing_distribution(void)
{
	struct json_object *timing_obj = param_get_object_root("timing");
	const char *name = param_get_string(timing_obj, "timing_distribution");
	for (int i = 0; i < TIMING_DISTRIBUTIONS_SIZE; ++i) {
		if (!strcmp(name, timing_distributions[i].name)) {
			timing_distribution_func = timing_distributions[i].func;
			return;
		}
	}
	config_error("timing distribution \"%s\" is unkonwn", name);
}

void parameters_read_from_file(const char *path,
		struct app_parameters *app) {
	void __real_free(void *ptr);
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "Couldn't open '%s': %s\n", path, strerror(errno));
		exit(1);
	}

	/* Parse the json file */
	char *line = NULL;
	size_t line_size = 0;
	ssize_t line_length = 0;
	json_object *jobj = NULL;
	int length;
	struct json_tokener *jtok = json_tokener_new();
	enum json_tokener_error jerr;
	do {
		line_length = getline(&line, &line_size, f);
		assert(line_length < INT_MAX);
		length = (int) line_length;
		jobj = json_tokener_parse_ex(jtok, line, length);
		jerr = json_tokener_get_error(jtok);
	} while (jerr == json_tokener_continue);
	__real_free(line);
	json_tokener_free(jtok);
	jtok = NULL;
	if (jerr != json_tokener_success) {
		fprintf(stderr, "Error while parsing JSON object in %s: %s\n",
				path, json_tokener_error_desc(jerr));
		exit(1);
	}
	fclose(f);
	app->json_config = jobj;

	/* Process globally known parameters from "application" and "timing" objects */
	set_application_parameters(app);
	set_timing_distribution();
	update_application_object(jobj);

	/* Save configuration to output directory */
	const char *output_dir = output_get_directory();
	if (output_dir != NULL) {
		size_t filename_size = 12;
		size_t buf_size = strlen(output_dir) + 12 + 1;
		char buf[buf_size];
		strcpy(buf, output_dir);
		strncat(buf, "/config.json", filename_size);
		if (json_object_to_file_ext(buf, jobj, JSON_C_TO_STRING_PRETTY)) {
			fprintf(stderr, "Couldn't write configuration: %s\n", strerror(errno));
		}
	}
}

static int is_numeric_type(struct json_object *obj)
{
	enum json_type type = json_object_get_type(obj);
	return type == json_type_int || type == json_type_double;
}

struct json_object *param_get(struct json_object *obj, const char *name)
{
	struct json_object *value;
	assert(json_object_is_type(obj, json_type_object));
	if (!json_object_object_get_ex(obj, name, &value)) {
		config_error("\"%s\" is not defined", name);
	}
	return value;
}

struct json_object *param_get_object(struct json_object *obj, const char *name)
{
	struct json_object *value = param_get(obj, name);
	if (!json_object_is_type(obj, json_type_object)) {
		config_error("\"%s\" is not an object", name);
	}
	return value;
}

struct json_object *param_get_object_root(const char *name)
{
	return param_get_object(app_params.json_config, name);
}

struct json_object *param_get_workload_object(const char *name)
{
	struct json_object *obj = param_get_object_root("workload");
	if (!json_object_object_get_ex(obj, name, &obj)) {
		config_error("no \"%s\" object found in \"workload\"", name);
	}
	if (!json_object_is_type(obj, json_type_object)) {
		config_error("\"%s\" in \"workload\" is not an object", name);
	}
	return obj;
}

double param_get_double(struct json_object *obj, const char *name)
{
	struct json_object *value = param_get(obj, name);
	if (!is_numeric_type(value)) {
		config_error("\"%s\" is not a double value", name);
	}
	return json_object_get_double(value);
}

static double param_get_double_default(struct json_object *obj, const char *name,
		double default_value)
{
	struct json_object *value;
	assert(json_object_is_type(obj, json_type_object));
	if (!json_object_object_get_ex(obj, name, &value)) {
		return default_value;
	}
	if (!is_numeric_type(value)) {
		config_error("\"%s\" is not a double value", name);
	}
	return json_object_get_double(value);
}

int param_get_int(struct json_object *obj, const char *name)
{
	struct json_object *value = param_get(obj, name);
	if (!json_object_is_type(value, json_type_int)) {
		config_error("\"%s\" is not a int value", name);
	}
	return json_object_get_int(value);
}

unsigned int param_get_uint(struct json_object *obj, const char *name)
{
	int value = param_get_int(obj, name);
	if (value < 0) {
		config_error("\"%s\" must be >= 0", name);
	}
	return (unsigned int) value;
}

const char *param_get_string(struct json_object *obj, const char *name)
{
	struct json_object *value = param_get(obj, name);
	if (!json_object_is_type(value, json_type_string)) {
		config_error("\"%s\" is not a string", name);
	}
	return json_object_get_string(value);
}

struct json_object *param_get_double_matrix(struct json_object *obj,
		const char *name, unsigned int width, unsigned int height)
{
	assert(width <= INT_MAX);
	assert(height <= INT_MAX);
	struct json_object *matrix = param_get(obj, name);
	if (!json_object_is_type(matrix, json_type_array)) {
		config_error("\"%s\" is not an array", name);
	}

	int length = json_object_array_length(matrix);
	if (length != (int) height) {
		config_error("In \"%s\", found %d elements while %d where expected",
				name, length, height);
	}

	for (unsigned int i = 0; i < height; ++i) {
		json_object *entries = json_object_array_get_idx(matrix, (int) i);
		if (!json_object_is_type(entries, json_type_array)) {
			config_error("In \"%s\", entry %d is not an array", name, i);
		}
		length = json_object_array_length(entries);
		if (length != (int) width) {
			config_error("In \"%s\", entry %d contains %d elements"
					" while %d where expected", name, i, length, width);
		}
		for (unsigned int j = 0; j < width; ++j) {
			json_object *entry = json_object_array_get_idx(entries, (int) j);
			if (!is_numeric_type(entry)) {
				config_error("In \"%s\", entry at row %d and column %d"
						" is not a double", name, i, j);
			}
		}
	}

	return matrix;
}

double param_get_double_matrix_element(struct json_object *obj,
		unsigned int column, unsigned int row)
{
	assert(column <= INT_MAX);
	assert(row <= INT_MAX);
	assert(json_object_is_type(obj, json_type_array));
	assert(json_object_array_length(obj) > (int) row);
	struct json_object *line = json_object_array_get_idx(obj, (int) row);
	assert(json_object_array_length(line) > (int) column);
	struct json_object *element = json_object_array_get_idx(line, (int) column);
	assert(is_numeric_type(element));
	return json_object_get_double(element);
}
