/** Colours CSS Module for Galvinise
 */
#include <stdio.h>
#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../../galv.h"
#include "colours.h"

#define MODULE_NAME	"colours"

struct colour {
	uint8_t r,g,b;
	float a;
};

static int colours_new_colour(lua_State *L);

static int colours_tostring(lua_State *L);
static int colours_average(lua_State *L);
static int colours_lightness(lua_State *L);
static int colours_luminosity(lua_State *L);

static struct luaL_Reg colour_methods[] = {
	{ "__tostring",  colours_tostring },	
	{ "average",	colours_average },
	{ "lightness",	colours_lightness },
	{ "luminosity",	colours_luminosity },
};

int
colours_init(lua_State *L) {
	// Into the global namespace
	lua_register(L, "rgb", colours_new_colour);
	lua_register(L, "rgba", colours_new_colour);

	// Create our metatable
	luaL_newmetatable(L, MODULE_NAME);

	// Set it as it's own index field
	lua_pushvalue(L, -1); /* duplicates the metatable */
	lua_setfield(L, -2, "__index");

	luaL_register(L, NULL, colour_methods);
	lua_pop(L, 1);

	return 0;
}

/*
 * Helper, allocate a new colour and set it's meta table.
 * It's RGB values are undefined.
 *
 * The object will be on top the stack.
 */
static struct colour *
colour_alloc(lua_State *L) {
	struct colour *c = lua_newuserdata(L, sizeof(struct colour));
	luaL_getmetatable(L, MODULE_NAME);
	lua_setmetatable(L, -2);
	return c;
}

static int
colours_new_colour(lua_State *L) {
	struct colour *c;
	int n;

	c = colour_alloc(L);

	n = lua_gettop(L);
	if (n < 1) {
		lua_pushnil(L);
		lua_pushstring(L, "Must pass an argument");
		return 0;
	}

	switch (n) {
	case 1:
		c->r = c->g = c->b = lua_tointeger(L, 1);
		c->a = 1;
		break;
	case 2:
		c->r = c->g = c->b = lua_tointeger(L, 1);
		c->a = lua_tonumber(L, 2);
		break;
	case 3:
		c->r = lua_tointeger(L, 1);
		c->g = lua_tointeger(L, 2);
		c->b = lua_tointeger(L, 3);
		c->a = 1;
	default:
		// 4 or more
		c->r = lua_tointeger(L, 1);
		c->g = lua_tointeger(L, 2);
		c->b = lua_tointeger(L, 3);
		c->a = lua_tonumber(L, 4);
	}

	return 1;
}

static int
colours_tostring(lua_State *L) {
	char buf[100];
	int len;
	struct colour *c = luaL_checkudata(L, 1, MODULE_NAME);
	if (!c) {
		return luaL_error(L, "Not a colour");
	}
	if (c->a >= 1) {
		lua_pushfstring(L, "rgb(%d,%d,%d)", c->r, c->g, c->b);
	} else {
		len = snprintf(buf,sizeof(buf),"rgba(%d,%d,%d,%0.2g)",
				c->r,c->g,c->b,c->a);
		lua_pushlstring(L, buf, len);
	}
	return 1;
}

/*
 * Returns a new colour whose values are the average of the previous one.
 * Essentially converts to grey scale.
 *
 * See also luminosity and lightness
 */
static int
colours_average(lua_State *L) {
	struct colour *c = luaL_checkudata(L, 1, MODULE_NAME);
	struct colour *nc;
	int val;
	if (!c) {
		return luaL_error(L, "Not a colour");
	}

	nc = colour_alloc(L);

	val = (c->r + c->g + c->b) / 3;
	nc->r = nc->g = nc->b = val;
	nc->a = c->a;

	return 1;
}

/*
 * Returns a new colour whose values are the average of the previous one.
 * Essentially converts to grey scale.
 *
 * See also luminosity and lightness
 */
static int
colours_lightness(lua_State *L) {
	struct colour *c = luaL_checkudata(L, 1, MODULE_NAME);
	struct colour *nc;
	int val,r,g,b;
	if (!c) {
		return luaL_error(L, "Not a colour");
	}

	nc = colour_alloc(L);

	r = c->r;
	g = c->g;
	b = c->b;
	val = (MAX(r,MAX(g,b)) + MIN(r,MIN(g,b))) / 2;
	nc->r = nc->g = nc->b = val;
	nc->a = c->a;

	return 1;
}
/*
 * Returns a new colour whose values are the average of the previous one.
 * Essentially converts to grey scale.
 *
 * See also luminosity and lightness
 */
static int
colours_luminosity(lua_State *L) {
	struct colour *c = luaL_checkudata(L, 1, MODULE_NAME);
	struct colour *nc;
	int val;
	if (!c) {
		return luaL_error(L, "Not a colour");
	}

	nc = colour_alloc(L);

	val = 0.21 * c->r + 0.72 * c->g + 0.7 * c->b;
	nc->r = nc->g = nc->b = val;
	nc->a = c->a;

	return 1;
}
