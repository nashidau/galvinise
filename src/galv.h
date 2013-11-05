
#include <lua.h>

#include "debug.h"
#include "blam.h"

#define MAX(a,b) ({	\
		__typeof__ (a) _a = (a); \
		__typeof__ (b) _b = (b); \
		_a > _b ? _a : _b;	\
		})
#define MIN(a,b) ({	\
		__typeof__ (a) _a = (a); \
		__typeof__ (b) _b = (b); \
		_a < _b ? _a : _b;	\
		})

extern int DEBUG_LEVEL;

extern lua_State *L;
