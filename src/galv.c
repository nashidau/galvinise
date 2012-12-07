#include <assert.h>
#include <fcntl.h>
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

#include "galv.h"

int DEBUG_LEVEL = 1;

struct inputfile {
	struct inputfile *next;
	const char *name;
	const char *outfile;
};

static struct predef_symbols {
	const char *symbol;
	const char *value;
} predef_symbols[] = {
	{ "GALVINISE", "Galvinise 0.1" },
	{ "GALVINISE_VERSION", "0.1" },
	{ "PANTS", "On" },
};
#define N_PREDEF_SYMS ((int)(sizeof(predef_symbols)/sizeof(predef_symbols[0])))

lua_State *L = NULL;

// Given a filename, generate a name for the output.
static char *get_name(void *ctx, const char *name);

static int process(struct inputfile *);

static int eval_symbol(struct blam *blam, const char *sym, int len);
static int init_symbols(void);

int
main(int argc, char **argv) {
	int i;
	struct inputfile *cur = NULL, *first = NULL;

	L = lua_open();
	luaL_openlibs(L);
	init_symbols();

	/* Get options */
	/* FIXME: Use getopt? */
	for (i = 1 ; i < argc ; i ++) {
		if (*argv[i] == '-') {
			printf("Found option: %s\n", argv[i] + 1);
		} else {
			if (cur == NULL) { // First file
				cur = talloc(NULL, struct inputfile);
				first = cur;
			} else {
				cur->next = talloc(cur, struct inputfile);
				cur = cur->next;
			}
			cur->name = argv[i];
			cur->outfile = get_name(cur, cur->name);
		}		
	}

	if (cur == NULL) {
		fprintf(stderr, "Galvinise: No input files\n");
		exit(1);
	}

	for (cur = first ; cur ; cur = cur->next) {
		printf("%s -> %s\n", cur->name, cur->outfile);
		process(cur);
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

/**
 * Initialise the predefined symbols.  At the moment we have the galvanise ones
 * only.
 */
static int
init_symbols(void) {
	int i;
	assert(L);
	if (!L) return -1;
	
	for (i = 0 ; i < N_PREDEF_SYMS ; i ++) {
		lua_pushstring(L, predef_symbols[i].value);
		lua_setglobal(L, predef_symbols[i].symbol);
		lua_pop(L, 1);
	}
	return 0;
}

static int
process(struct inputfile *inputfile) {
	int fd;
	const char *inaddr, *end;
	const char *p;
	int nbytes;
	struct stat st;
	struct blam *blam;

	fd = open(inputfile->name, O_RDONLY);
	fstat(fd, &st);
	inaddr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	
	blam = blam_init(inputfile, inputfile->outfile);

	p = inaddr;
	end = p + st.st_size;

	while (p < end) {
		nbytes = strcspn(p, "$");
		blam->write(blam, p, nbytes);
		p += nbytes;
		if (p == end) break;
		p ++;
		// FIXME: Currently allow all non-whitespace
		nbytes = strcspn(p, " \n\t");
		eval_symbol(blam, p, nbytes);
		p += nbytes;
	}	

	munmap((void *)inaddr, st.st_size);
	blam->close(blam);

	return 0;	
}

static int
eval_symbol(struct blam *blam, const char *sym, int len) {
	const char *value = NULL;
	char buf[len + 1];
	
	memcpy(buf, sym, len);
	buf[len] = 0;

	lua_getglobal(L, buf);
	if (lua_isstring(L, -1)) {
		value = lua_tostring(L, -1);
	} else {
		printf("Could not find %.*s\n", len, sym);
	}
	lua_pop(L, 1);

	if (value)
		blam->write(blam, value, strlen(value));
	return 0;
}

