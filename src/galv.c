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

#include "galv.h"
// FIXME: This should be in modules.h or similar
#include "modules/css/colours.h"
#include "diskslurp.h"

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
	{ "$", "$" },
};
#define N_PREDEF_SYMS ((int)(sizeof(predef_symbols)/sizeof(predef_symbols[0])))

static int galv_lua_include(lua_State *L);
static int galv_lua_outraw(lua_State *L);

static const luaL_Reg methods[] = {
	{ "include",	galv_lua_include },
	{ "Oraw",	galv_lua_outraw },
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
static const char *slurp_comment(const char *p, const char *end);

void galv_stack_dump(lua_State *lua,const char *msg,...);

int
main(int argc, char **argv) {
	int i;
	struct inputfile *cur = NULL, *first = NULL;

	L = lua_open();
	luaL_openlibs(L);
	init_symbols();

	colours_init(L);

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
		printf("Galvinising: %s -> %s\n", cur->name, cur->outfile);
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
	static bool useread = false;
	int fd;
	const char *inaddr, *end;
	const char *p;
	const char *test;
	int nbytes;
	bool iscall;
	struct stat st;

	fd = open(inputfile->name, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Unable to open '%s': %s\n", inputfile->name,
				strerror(errno));
		return -1;
	}

	fstat(fd, &st);
	
	inaddr = MAP_FAILED;
	if (!useread) {
		inaddr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (inaddr == MAP_FAILED) {
			if (useread) {
				printf("Unable to mmap %s: %s\n",
						inputfile->name,
						strerror(errno));
				printf("Falling back to read from here");
				useread = true;
			}
		}
	}
	if (inaddr == MAP_FAILED)
		inaddr = diskslurp(fd, st.st_size);

	close(fd);

	if (inaddr == NULL || inaddr == MAP_FAILED)
		return -1;

	p = inaddr;
	end = p + st.st_size;

	while (p < end) {
		nbytes = strcspn(p, "${");
		blam->write(blam, p, nbytes);
		p += nbytes;
		test = p;
		if (p == end) break;
		if (*p != '$' && strncmp(p, "{{", 2) && strncmp(p, "{-{", 3)) {
			blam->write(blam, p, 2);
			p += 2;
			continue;
		}

		/* FIXME: Test this throughly */
		if (strncmp(p, "{-{", 3) == 0) {
			p = slurp_comment(test + 3, end);
			if (p == NULL) p = end;
		} else if (*test == '{') {
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
			// Handle '$'
			p ++;
			if (*p == '$') {
				// $$ outputs $
				blam->write(blam, "$", 1);
				p ++;
				continue;
			}
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

	if (useread)
		talloc_free((void *)inaddr);
	else
		munmap((void *)inaddr, st.st_size);

	return 0;
}

/**
 *  Read in the length of a comment.
 *
 *  @p The opening byte of the comment.
 *  @returns The first byte after the comment or NULL
 * FIXME: Can still fall of the end if } is last character.
 *  Should use end - 3 or something.
 */
static const char *
slurp_comment(const char *p, const char *end) {
	while (true) {
		p = memchr(p, '}', end - p);
		if (p == NULL) {
			// Terminator not fou
			printf("End of comment not found\n");
			return NULL;
		}
		if (p[1] == '-' && p[2] == '}') {
			return p + 3;
		}
		p ++;
	}

	// FIXME: Check length of buffer.
	// FIXME: Handle this error
	assert(!"Unterminated comment");
	return p;
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

	if (lua_isnil(L, -1)) {
		printf("Could not find '%.*s'\n", len, sym);
		lua_pop(L, 1);
		return -1;
	}

	if (lua_isstring(L, -1)) {
		value = lua_tostring(L, -1);
	} else if (lua_isuserdata(L, -1)) {
		// FIXME: Other types
		luaL_callmeta(L, -1, "__tostring");
		value = lua_tostring(L, -1);
		lua_pop(L, 1);
	} else {
		value = "Can't handle this type";
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
	if (process_file(blam, &include)) {
		exit(1);
	}

	lua_pushboolean(L, 1);
	return 1;
}

/**
 * Send a string to blam to out to output.
 *
 */
static int
galv_lua_outraw(lua_State *L) {
	struct blam *blam;
	int i, n;
	size_t len;
	const char *str;

	n = lua_gettop(L);
	/* FIXME: I should use the registry for this */
	lua_getglobal(L, "blam"); // FIXME: check
	blam = lua_touserdata(L, -1);

	for (i = 0 ; i < n ; i ++) {
		str = lua_tolstring(L, n, &len);
		blam->write(blam, str, len);
	}

	// Make sure strings are valid
	blam->flush(blam);

	return 0;
}


int
galv_table_count(lua_State *lua, int table){
        int count = 0;
        if (!lua_istable(lua, table))
                return -1;
        lua_pushnil(lua);  /* first key */
        while (lua_next(lua, table) != 0) {
                count ++;
                lua_pop(lua, 1);
        }
        return count;
}




void
galv_stack_dump(lua_State *lua,const char *msg,...){
	int i,n,st;
	const char *str;
	va_list ap;

	if (!msg)
		printf("Current Lua Stack:");
	else {
		va_start(ap, msg);
		vprintf(msg, ap);
		va_end(ap);
		printf("\n");
	}
	n = lua_gettop(lua);
	if (n == 0){
		printf("\tEmpty stack\n");
		return;
	} else if (n < 0){
		printf("\tMassive stack corruption: Top is %d\n",n);
		assert(n >= 0);
		return;
	}
	printf("\n");
	for (i = 1; i <= n; i++){
		printf("%7d: %s ",i, lua_typename(lua,lua_type(lua,i)));
		switch (lua_type(lua,i)){
			case LUA_TNIL:
				printf(" nil\n");
				break;
			case LUA_TNUMBER:
				printf(" %lf\n",lua_tonumber(lua,i));
				break;
			case LUA_TBOOLEAN:
				printf(" %s\n", lua_toboolean(lua,i) ? "true":"false");
				break;
			case LUA_TSTRING:
				printf(" '%s'\n", lua_tostring(lua,i));
				break;
			case LUA_TLIGHTUSERDATA:
				printf(" %p\n", lua_touserdata(lua,i));
				break;
			case LUA_TTABLE:
				lua_pushstring(lua, "__mode");
				lua_rawget(lua,i);
				if (lua_isnil(lua, -1))
					lua_pop(lua,1);
				else {
					const char *m = lua_tostring(lua, -1);
					printf("Weak");
					if (strchr(m,'k')) printf(" keys");
					if (strchr(m,'v')) printf(" values");
					lua_pop(lua,1);
				}
				printf(" %d items (%d array items)",
						galv_table_count(lua,i),
						(int)lua_objlen(lua,i));
				if (luaL_callmeta(lua, i, "__tostring")){
					str = lua_tostring(lua,-1);
					printf("\t%s\n",str);
					lua_pop(lua,1);
				}
				printf("[%p]\n",lua_topointer(lua,i));
				lua_getfield(lua, i, "name");
				if (lua_isstring(lua,-1)){
					printf("\t\tQuest Name is %s\n",lua_tostring(lua,-1));
				}
				lua_pop(lua,1);
				break;
			case LUA_TTHREAD:
				st = lua_objlen(lua,i);
				if (st == 0) printf(" %d Okay",st);
				else if (st == LUA_YIELD) printf(" %d yeild\n",st);
				else if (st == LUA_ERRRUN) printf(" %d Error Run\n",st);
				else if (st == LUA_ERRSYNTAX)
					printf(" %d Error Syntax\n",st);
				else if (st == LUA_ERRMEM)
					printf(" %d Error Memory\n",st);
				else if (st == LUA_ERRERR)
					printf(" %d Error Error\n",st);
				break;
			case LUA_TFUNCTION:
				printf(" (%p)\n", lua_topointer(lua,i));
				break;
			default:
				if (luaL_callmeta(lua, i, "__tostring")){
					str = lua_tostring(lua,-1);
					lua_pop(lua,1);
					printf(" %s\n",str);
				} else {
					printf(" (no __tostring function)\n");
				}
		}
	}
}
