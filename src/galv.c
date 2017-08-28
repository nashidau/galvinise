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
#include "galvinise.h"
// FIXME: This should be in modules.h or similar
#include "modules/css/colours.h"
#include "diskslurp.h"

int DEBUG_LEVEL = 1;

// Lua environment 
static int gref;

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

static int process_file(struct blam *, const char *infilename);
static int process_internal(const char *in, int len, struct blam *out);

static int extract_symbol(const char *start, bool *iscall);
static int eval_symbol(struct blam *blam, const char *sym, int len);
static int eval_inline(struct blam *blam, const char *sym, int len);
static int eval_lua(struct blam *blam, const char *sym, int len);
static int init_symbols(void);
static int walk_symbol(const char *sym, int len);
static const char *slurp_comment(const char *p, const char *end);
static int write_object(struct blam *blam, lua_State *L, int index);

void galv_stack_dump(lua_State *lua,const char *msg,...);


static const luaL_Reg methods[] = {
	{ "include",	galv_lua_include },
	{ "Oraw",	galv_lua_outraw },
	{ NULL, NULL },
};

lua_State *L = NULL;

lua_State *galvinise_init(int *argc, char **argv) {
	L = lua_open();
	assert(L);
	luaL_openlibs(L);
	init_symbols();

	colours_init(L);

	return L;
}

/**
 * Get the Lua environment galvinise uses.
 *
 * You can pass your own values to the template from here.
 * Any symbol starting with galv or blam is reserved.
 */
lua_State *galvinise_environment_get(void) {
	return L;
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

	lua_pop(L, 1);
	return 0;
}

int
galvinise(struct galv_file *inputfile) {
	struct blam *blam;

	if (lua_gettop(L) < 1 || !lua_istable(L, -1)) {
		lua_newtable(L);
	}

	// Set the metatable __index field to be the _G table
	// FIXME: I could reuse this table for speed/memory
	lua_newtable(L);
	lua_getglobal(L, "_G");
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L,  -2);
	gref = luaL_ref(L, LUA_REGISTRYINDEX);

	blam = blam_init(inputfile, inputfile->outfile);
	// FIXME: Should be in the registry
	lua_pushlightuserdata(L, blam);
	lua_setglobal(L, "blam");

	process_file(blam, inputfile->name);

	lua_pushnil(L);
	lua_setglobal(L, "blam");

	blam->close(blam);
	return 0;
}

int
galvinise_onion(const char *input, struct onion_response_t *res) {
	struct blam *blam;

	// Should have a value on top of the stack which is an
	// table, which will be the current environment
	if (lua_gettop(L) < 1 || !lua_istable(L, -1)) {
		lua_newtable(L);
	}

	lua_newtable(L);
	lua_getglobal(L, "_G");
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	gref = luaL_ref(L, LUA_REGISTRYINDEX);

	// FIXME: Should be in the registry
	blam = blam_onion_init(NULL, res);
	lua_pushlightuserdata(L, blam);
	lua_setglobal(L, "blam");

	process_file(blam, input);

	lua_pushnil(L);
	lua_setglobal(L, "blam");
	talloc_free(blam);

	luaL_unref(L, LUA_REGISTRYINDEX, gref);

	return 0;
}


char *
galvinise_buf(const char *buf, size_t len) {
	struct blam *blam;
	char *out;

	// Should have a value on top of the stack which is an
	// table, which will be the current environment
	if (lua_gettop(L) < 1 || !lua_istable(L, -1)) {
		lua_newtable(L);
	}

	lua_newtable(L);
	lua_getglobal(L, "_G");
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	gref = luaL_ref(L, LUA_REGISTRYINDEX);

	// FIXME: Should be in the registry
	blam = blam_buf_init(NULL);
	lua_pushlightuserdata(L, blam);
	lua_setglobal(L, "blam");

	if (len == 0) {
		len = strlen(buf);
	}
	process_internal(buf, len, blam);

	lua_pushnil(L);
	lua_setglobal(L, "blam");

	luaL_unref(L, LUA_REGISTRYINDEX, gref);

	out = blam_buf_get(blam, NULL);

	talloc_free(blam);

	return out;
}

int
process_file(struct blam *blam, const char *infilename) {
	const char *inaddr;
	static bool useread = false;
	int fd;
	struct stat st;

	fd = open(infilename, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Unable to open '%s': %s\n", infilename,
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
						infilename,
						strerror(errno));
				printf("Falling back to read from here");
				useread = true;
			}
		}
	}
	if (inaddr == MAP_FAILED)
		inaddr = diskslurp(fd, st.st_size);

	close(fd);

	if (inaddr == NULL || inaddr == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	process_internal(inaddr, st.st_size, blam);

	if (useread)
		talloc_free((void *)inaddr);
	else
		munmap((void *)inaddr, st.st_size);

	return 0;
}


static int process_internal(const char *in, int len, struct blam *blam) {
	const char *end;
	const char *p;
	const char *test;
	int nbytes;
	bool iscall;

	p = in;
	end = p + len;

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

	if (iscall) *iscall = false;

	p = start;
	if (isdigit(*p)) {
		// We don't allow symbols to start with digits - probably
		// a price,  return..
		return 0;
	}

	while (isalnum(*p) || *p == ']' || *p == '[' ||
			*p == '.' || *p == ':' || *p == '_') {
		if (*p == '[' && iscall) *iscall = true;
		p ++;
	}

	if (*p != '(') {
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
	char buf[len + 1];

	memcpy(buf, sym, len);
	buf[len] = 0;

	if (walk_symbol(sym, len) != 0) {
		printf("eval_symbol: Error traversing %.*s\n", len, sym);
		return -1;
	}

	if (lua_isnil(L, -1)) {
		printf("eval_symbol: Could not find '%.*s'\n", len, sym);
		lua_pop(L, 1);
		return -1;
	}

	write_object(blam, L, -1);

	lua_pop(L, 1);

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

	// FIXME: write a wrapper around loadbuffer/setfenv
	luaL_loadbuffer(L, buf, len + 7, buf);

	lua_rawgeti(L, LUA_REGISTRYINDEX, gref);
	if (lua_setfenv(L, -2) != 1) {
		// FIXME: Shoudl barf.
		galv_stack_dump(L, "eval_inline failed\n");
		printf("eval_inline: Setenv failed\n");
	}
	lua_pcall(L, 0, 1, 0); // FIXME: Use LUA_MULTRET
	write_object(blam, L, -1);
	lua_pop(L, 1);

	return 0;
}

static int
write_object(struct blam *blam, lua_State *L, int index) {
	const char *value = NULL;
	size_t len;

	switch (lua_type(L, index)) {
	case LUA_TNIL:
		value = "nil"; len = 3;
		break;
	case LUA_TNUMBER:
	case LUA_TSTRING:
		value = lua_tolstring(L, index, &len);
		break;
	case LUA_TBOOLEAN:
		if (lua_toboolean(L, index)) {
			value = "true";
			len = 4;
		} else {
			value = "false";
			len = 5;
		}
		break;
	case LUA_TLIGHTUSERDATA:
		value = "user data";
		len = 9;
		break;
	case LUA_TUSERDATA:
		luaL_callmeta(L, index, "__tostring");
		value = lua_tolstring(L, -1, &len);
		lua_pop(L, 1);
		break;
	case LUA_TTABLE:
	case LUA_TTHREAD:
	default:
		// FIXME: Call __tostring on table maybe?
		value = "???";
		len = 3;
		break;
	}

	if (value)
		blam->write(blam, value, len);
	return !!value;
}

/**
 * Given a lua string of the form x.y.z, gets x, then y, then z.  Returns z on
 * top of the lua stack.  returns 0 on success.
 */
static int
walk_symbol(const char *sym, int len) {
	const char *p;
	const char *end = sym + len;
	int ind;

	lua_rawgeti(L, LUA_REGISTRYINDEX, gref);
	ind = lua_gettop(L);

	do {
		p = memchr(sym, '.', end - sym);
		if (p == NULL) p = end;
		lua_pushlstring(L, sym, p - sym);
		lua_gettable(L, ind);
		ind = lua_gettop(L);
		sym = p + 1;
	} while (p < end);

	// Remove the environtable we just grabbed.
	lua_remove(L, -2);

	return 0;
}

// FIXME: Should get the line number or something.
static int
eval_lua(struct blam *blam, const char *block, int len) {
	luaL_loadbuffer(L, block, len, "eval_lua");
	lua_rawgeti(L, LUA_REGISTRYINDEX, gref);
	if (lua_setfenv(L, -2) != 1) {
		// FIXME: Shoudl barf.
		galv_stack_dump(L, "eval_inline failed\n");
		printf("eval lua: Setenv failed\n");
		printf("[[%.*s]]\n", len, block);
		exit(1);
	}

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
	struct galv_file include;

	name = lua_tostring(L, lua_gettop(L));

	include.name = name;
	include.outfile = NULL;
	include.next = NULL;
	lua_getglobal(L, "blam"); // FIXME: check
	blam = lua_touserdata(L, -1);
	lua_pop(L, 1);
	if (process_file(blam, include.name)) {
		fprintf(stderr, "Process file failed\n");
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

	n = lua_gettop(L);
	/* FIXME: I should use the registry for this */
	lua_getglobal(L, "blam"); // FIXME: check
	blam = lua_touserdata(L, -1);
	lua_pop(L, 1);

	for (i = 1 ; i <= n ; i ++) {
		write_object(blam, L, i);
	}
	// Make sure strings are valid
	blam->flush(blam);

	return 0;
}

static int
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
	}
	n = lua_gettop(lua);
	if (n == 0){
		printf("\n\tEmpty stack\n");
		return;
	} else if (n < 0){
		printf("\n\tMassive stack corruption: Top is %d\n",n);
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
					printf("\t\tName is %s\n",lua_tostring(lua,-1));
				}
				lua_pop(lua,1);
				if (lua_getmetatable(L, i)) {
					printf("\t\tHas metatable\n");
					lua_pop(L, 1);
				}
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
