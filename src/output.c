#include "output.h"
#include "common.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef REVISION
#define REVISION unknown
#endif

static const char *output_dir;

static void xfputv(int argc, char **argv, FILE *f)
{
	int i;
	for (i = 0; i < argc - 1; ++i) {
		xfputs(argv[i], f);
		xfputc(' ', f);
	}
	if (i < argc) xfputs(argv[i], f);
}

FILE *output_open(const char *name, int append)
{
	if (output_dir == NULL) return NULL;
	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s/%s", output_dir, name);
	FILE *f = fopen(path, append ? "a" : "w");
	if (f == NULL) {
		fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
		return NULL;
	}
	return f;
}

void output_init(const char* _output_dir)
{
	output_dir = _output_dir;
	if (output_dir == NULL) return;
	if (mkdir(output_dir, 0777)) {
		if (errno != EEXIST) {
			fprintf(stderr, "Cannot use %s as output directory: %s\n",
					output_dir, strerror(errno));
			exit(1);
		}
	}
}

void output_log_startup(int argc, char **argv)
{
	FILE *f = output_open("startup", 0);

	// Write command line
	xfputv(argc, argv, f);
	xfputc('\n', f);

	char date_buf[20];
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	strftime(date_buf, 20, "%Y-%m-%d %H:%M:%S", tm);
	xfprintf(f, "Started on %s revision %s\n", date_buf, STRINGIFY(REVISION));
	xfclose(f);
}

void output_log_rootsim_args(int argc, char **argv)
{
	FILE *f = output_open("startup", 1);
	xfprintf(f, "Arguments for ROOT-Sim: ");
	xfputv(argc, argv, f);
	xfputc('\n', f);
	xfclose(f);
}

void output_log_lps(unsigned int num_lps)
{
	FILE *f = output_open("lps", 0);
	if (f == NULL) return;
	for (lpid_t i = 0; i < num_lps; ++i) {
		fprintf(f, "%d: %s\n", i, lp_name(i));
	}
}

int output_client_file_path(lpid_t lpid, replica_t replica,
		const char *suffix, char *buf, size_t size)
{
	if (suffix != NULL && suffix[0] != '\0') {
		return snprintf(buf, size, "%s/r%d_c%d_%s",
				output_dir, replica, lpid, suffix);
	} else {
		return snprintf(buf, size, "%s/r%d_c%d",
				output_dir, replica, lpid);
	}
}

static int output_name_server_suffix(replica_t replica, partition_t partition,
		const char *suffix, char *buf, size_t size)
{
	if (suffix != NULL && suffix[0] != '\0') {
		return snprintf(buf, size, "r%d_p%d%s", replica, partition, suffix);
	} else {
		return snprintf(buf, size, "r%d_p%d", replica, partition);
	}
}

int output_server_file_path(replica_t replica, partition_t partition,
		const char *suffix, char *buf, size_t size)
{
	size_t output_dir_len = strlen(output_dir);
	assert(output_dir_len < size + 1);
	strcpy(buf, output_dir);
	buf[output_dir_len] = '/';
	++output_dir_len;
	return output_name_server_suffix(replica, partition, suffix,
			buf + output_dir_len, size - output_dir_len);
}

FILE *output_server_open_suffix(replica_t replica, partition_t partition,
		char *suffix)
{
	size_t buf_size = 64;
	char buf[buf_size];
	output_name_server_suffix(replica, partition, suffix, buf, buf_size);
	return output_open(buf, 0);
}

FILE *output_server_open(replica_t replica, partition_t partition)
{
	return output_server_open_suffix(replica, partition, NULL);
}

const char *output_get_directory(void)
{
	return output_dir;
}
