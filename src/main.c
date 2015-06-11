#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
main(int argc, char **argv) {
	int i;
	struct galv_file *cur = NULL, *first = NULL;
	lua_State *L;
	
	L = galvinise_init(&argc, argv);

	/* Get options */
	/* FIXME: Use getopt? */
	for (i = 1 ; i < argc ; i ++) {
		if (*argv[i] == '-') {
			printf("Found option: %s\n", argv[i] + 1);
		} else {
			if (cur == NULL) { // First file
				cur = talloc(NULL, struct galv_file);
				first = cur;
			} else {
				cur->next = talloc(cur, struct galv_file);
				cur = cur->next;
			}
			cur->next = NULL;
			cur->name = argv[i];
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
