#ifndef GALVINISE_H
#define GALVINISE_H 1

struct galv_file {
	struct galv_file *next;
	const char *name;
	const char *outfile;
};

int galvinise_init(int *argc, char **argv);

int galvinise(struct galv_file *file);

#endif  // GALVINISE_H
