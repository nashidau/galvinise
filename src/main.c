//#define _GNU_SOURCE 1

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <talloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "galvinise.h"

struct blam;

// Given a filename, generate a name for the output.
static char *get_name(void *ctx, const char *name);

int process_file(struct blam *, struct galv_file *);

int
usage(const char *command) {
	printf("Galvinise: A tin-plating, err, templating system\n");
	printf("\n");
	printf("Usage: %s <input files>\n", command);
	printf(" Writes data to file without the .gvz on the end\n");
	printf(" If input filename contains .gvz, the .gvz is removed\n");
	printf(" Otherwise output is written to the same name .out\n");
	printf("\n");
	printf(" -d VAR=value Define a value\n");
	printf(" -l <Lua> Lua code to be executed\n");
	printf(" -o <outfile> Where to write the output\n");
	printf(" -h Help\n");

	exit(0);
}

int
main(int argc, char **argv) {
	struct galv_file *cur = NULL, *first = NULL;
	static const struct option long_options[] = {
		{ "define", required_argument, 0, 'd' },
		{ "lua", required_argument, 0, 'l' },
		{ "output", required_argument, 0, 'o' },
		{ "verbose", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' },
		{ NULL, 0, 0, 0 },
	};
	const char *optstr = "D:d:l:o:hv";
	int opt;
	lua_State *L;
	const char *outfile;
	
	L = galvinise_init(&argc, argv);

	/* Get options */
	while ((opt = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
		switch (opt) {
		case 'd':
		case 'D':
			galvinise_set_value_str(L, optarg);
			break;
		case 'l':
			printf("Lua script\n");
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'v':
			printf("Verbose\n");
			break;
		case 'h':
			usage(argv[0]);
			break;
		}
	}

	for ( ; argv[optind] ; optind ++) {
		if (cur == NULL) { // First file
			cur = talloc(NULL, struct galv_file);
			first = cur;
		} else {
			cur->next = talloc(cur, struct galv_file);
			cur = cur->next;
		}
		cur->next = NULL;
		cur->name = argv[optind];
		if (outfile) {
			cur->outfile = strdup(outfile);
			outfile = NULL;
		} else {
			cur->outfile = get_name(cur, cur->name);
		}
	}

	if (cur == NULL) {
		fprintf(stderr, "Galvinise: No input files\n");
		exit(1);
	}

	for (cur = first ; cur ; cur = cur->next) {
		lua_newtable(L);
		printf("Galvinising: %s -> %s\n", cur->name, cur->outfile);
		galvinise(cur);
	}

	exit(0);
}

static char *
get_name(void *ctx, const char *name) {
	char *outfile, *p, *q;

	if ((p = strstr(name, ".gvz"))) {
		outfile = talloc_size(ctx, strlen(name) - 3);
		strncpy(outfile, name, p - name);
		q = outfile + (p - name);
		p += 4; /* sizeof(.gvz) */
		if (*p == 0)
			*q = 0;
		else {
			strcpy(q, p);
		}
	} else {
		outfile = talloc_asprintf(ctx, "%s.out", name);
	}
	return outfile;
}
