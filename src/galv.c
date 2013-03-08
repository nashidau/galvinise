#include <assert.h>
#include <ctype.h>
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

static int galv_lua_include(lua_State *L);

static const luaL_Reg methods[] = {
	{ "include",	galv_lua_include },
	{ NULL, NULL },
};

lua_State *L = NULL;

// Given a filename, generate a name for the output.
static char *get_name(void *ctx, const char *name);

static int process(struct inputfile *);
static int process_file(struct blam *, struct inputfile *);

static int extract_symbol(const char *start, bool *iscall);
static int eval_symbol(struct blam *blam, const char *sym, int len);
static int eval_inline(struct blam *blam, const char *sym, int len);
static int eval_lua(struct blam *blam, const char *sym, int len);
static int init_symbols(void);
static int walk_symbol(const char *sym, int len);

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
	}

	lua_getglobal(L, "_G");
	luaL_openlib(L, NULL, methods, 0);
	return 0;
}

static int
process(struct inputfile *inputfile) {
	struct blam *blam;

	blam = blam_init(inputfile, inputfile->outfile);
	// FIXME: Should be in the registry
	lua_pushlightuserdata(L, blam);
	lua_setglobal(L, "blam");

	process_file(blam, inputfile);

	lua_pushnil(L);
	lua_setglobal(L, "blam");

	blam->close(blam);
	return 0;
}



static int
process_file(struct blam *blam, struct inputfile *inputfile) {
	int fd;
	const char *inaddr, *end;
	const char *p;
	const char *test;
	int nbytes;
	bool iscall;
	struct stat st;

	fd = open(inputfile->name, O_RDONLY);
	fstat(fd, &st);
	inaddr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);


	p = inaddr;
	end = p + st.st_size;

	while (p < end) {
		nbytes = strcspn(p, "${");
		test = p + nbytes;
		blam->write(blam, p, nbytes);
		p += nbytes;
		if (p == end) break;
		if (*test == '{' && test[1] != '{') {
			// Just a brace, continue on
			blam->write(blam, "{", 1);
			p ++;
			continue;
		}
		if (*test == '{') {
			p += 2;
			test = p;
			nbytes = 0;
			do {
				nbytes += strcspn(test, "}");
				nbytes += 1;
				test = p + nbytes;
			} while (*test != '}');
			eval_lua(blam, p, nbytes - 1);
			p += 1;
			p += nbytes;
		} else {
			p ++;
			nbytes = extract_symbol(p, &iscall);
			if (nbytes == 0) {
				// Not a symbol
				blam->write(blam, "$", 1);
			} else if (iscall) {
				eval_inline(blam, p, nbytes);
			} else {
				eval_symbol(blam, p, nbytes);
			}
			p += nbytes;
		}
	}


	munmap((void *)inaddr, st.st_size);

	return 0;
}

/* start points at the first character */
/**
 * process a $foo style symbol
 */
static int
extract_symbol(const char *start, bool *iscall) {
	const char *p;
	int depth = 0;

	p = start;
	while (isalnum(*p) || *p == '.' || *p == '_') {
		p ++;
	}

	if (*p != '(') {
		if (iscall) *iscall = false;
		return p - start;
	}

	if (iscall) *iscall = true;

	while (*p != ')' || depth > 1) {
		if (*p == '(') depth ++;
		if (*p == ')') depth --;
		p ++;
	}
	p ++; // grab the last ')'
	return p - start;
}

/**
 * Eval a $ style symbol. The string passed in is the unprocessed symbol.
 * It may include .
 */
static int
eval_symbol(struct blam *blam, const char *sym, int len) {
	const char *value = NULL;
	char buf[len + 1];

	memcpy(buf, sym, len);
	buf[len] = 0;

	if (walk_symbol(sym, len) != 0) {
		printf("Error traversing %.*s\n", len, sym);
		return -1;
	}

	if (lua_isstring(L, -1)) {
		value = lua_tostring(L, -1);
	} else {
		// FIXME: This error bites
		printf("Could not find %.*s\n", len, sym);
	}
	lua_pop(L, 1);

	if (value)
		blam->write(blam, value, strlen(value));
	return 0;
}

/**
 * Evaluate an inline symbol of the form $foo(7)
 */
static int
eval_inline(struct blam *blam, const char *sym, int len) {
	char buf[len + 10]; // "return %s;"
	strcpy(buf, "return ");
	strncpy(buf + 7, sym, len);
	buf[7 + len] = 0;

	luaL_loadbuffer(L, buf, len + 7, buf);
	lua_pcall(L, 0, 1, 0); // FIXME: Use LUA_MULTRET
	if (lua_isstring(L, -1)) {
		blam->write_string(blam, lua_tostring(L, -1));
	} else {
		printf("Didn't get a string back from inline\n");
	}
	lua_pop(L, 1);

	return 0;
}

/**
 * Given a lua string of the form x.y.z, gets x, then y, then z.  Returns z on
 * top of the lua stack.  returns 0 on success.
 */
static int
walk_symbol(const char *sym, int len) {
	const char *p;
	const char *end = sym + len;
	int ind = LUA_GLOBALSINDEX;

	do {
		p = memchr(sym, '.', end - sym);
		if (p == NULL) p = end;
		lua_pushlstring(L, sym, p - sym);
		lua_gettable(L, ind);
		ind = lua_gettop(L);
		sym = p + 1;
	} while (p < end);

	return 0;
}

// FIXME: Should get the line number or something.
static int
eval_lua(struct blam *blam, const char *block, int len) {
	luaL_loadbuffer(L, block, len, "some block you know");
	lua_pcall(L, 0, 1, 0); // FIXME: Should use LUA_MULTRET
	if (lua_isstring(L, -1)) {
		blam->write_string(blam, lua_tostring(L, -1));
	}
	lua_pop(L, 1);

	return 0;
}


/**
 * Include another file at the current location.
 *
 * FIXME: Recursion.
 */
static int
galv_lua_include(lua_State *L) {
	struct blam *blam;
	const char *name;
	struct inputfile include;

	name = lua_tostring(L, lua_gettop(L));

	include.name = name;
	include.outfile = NULL;
	include.next = NULL;
	lua_getglobal(L, "blam"); // FIXME: check
	blam = lua_touserdata(L, -1);
	lua_pop(L, 1);
	process_file(blam, &include);

	// This is horribly wrong
	lua_pushstring(L, "");
	return 1;
}
