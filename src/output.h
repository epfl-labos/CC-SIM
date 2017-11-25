#ifndef output_h
#define output_h

#include "common.h"

#define xfputs(str, f) \
	do { \
		if (verbose) fputs(str, stdout); \
		if (f) fputs(str, f); \
	} while (0)

#define xfputc(c, f) \
	do { \
		if (verbose) putc(c, stdout); \
		if (f) fputc(c, f); \
	} while (0)

#define xfprintf(f, fmt, ...) \
	do { \
		if (verbose) printf(fmt, ##__VA_ARGS__); \
		if (f) fprintf(f, fmt, ##__VA_ARGS__); \
	} while (0)

#define xfclose(f) \
	if (f) fclose(f)

void output_init(const char* output_dir);
FILE *output_open(const char *name, int append);
void output_log_startup(int argc, char **argv);
void output_log_rootsim_args(int argc, char **argv);
void output_log_lps(unsigned int num_lps);
int output_client_file_path(lpid_t lpid, replica_t replica,
		const char *suffix, char *buf, size_t size);
FILE *output_server_open(replica_t replica_id, partition_t partition_id);
FILE *output_server_open_suffix(replica_t replica, partition_t partition,
		char *suffix);
int output_server_file_path(replica_t replica, partition_t partition,
		const char *suffix, char *buf, size_t size);
const char *output_get_directory(void);

#endif
