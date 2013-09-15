/** Colours CSS Module for Galvinise
 */
#include <stdint.h>

#include "../../galv.h"

struct colour {
	uint8_t r,g,b,a;
};

int
colours_init(void) {
	return 0;
}

int
colours_new_colour(lua_State *L) {
	
	// For now, require 4 args

	return 0;
}

