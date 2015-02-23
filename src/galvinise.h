#ifndef GALVINISE_H
#define GALVINISE_H 1

struct galv_file {
	struct galv_file *next;
	const char *name;
	const char *outfile;
};

int galvinise_init(int *argc, char **argv);
lua_State *galvinise_environment_get(void);

int galvinise(struct galv_file *file);
int galvinise_onion(const char *input, struct onion_response_t *res);

#endif  // GALVINISE_H
