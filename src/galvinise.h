#ifndef GALVINISE_H
#define GALVINISE_H 1

#include <lua.h>

struct onion_response_t;

struct galv_file {
	struct galv_file *next;
	const char *name;
	const char *outfile;
};

lua_State *galvinise_init(int *argc, char **argv);
lua_State *galvinise_environment_get(void);

int galvinise_set_value_str(lua_State *L, const char *str);

int galvinise(struct galv_file *file);
char *galvinise_buf(const char *buf, size_t len);
int galvinise_onion(const char *input, struct onion_response_t *res);

#endif  // GALVINISE_H
