#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <talloc.h>

#include "galv.h"

int DEBUG_LEVEL = 1;

struct inputfile {
	struct inputfile *next;
	const char *name;
	const char *outfile;
};

// Given a filename, generate a name for the output.
static char *get_name(void *ctx, const char *name);

static int process(struct inputfile *);

static int eval_symbol(struct blam *blam, const char *sym, int len);

int
main(int argc, char **argv) {
	int i;
	struct inputfile *cur = NULL, *first = NULL;

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
	}	

	munmap((void *)inaddr, st.st_size);
	blam->close(blam);

	return 0;	
}

static int
eval_symbol(struct blam *blam, const char *sym, int len) {
	const char *value = NULL;

	if (len == 9 && strncmp(sym, "GALVINISE", len) == 0) {
		value  = "Galvinise 0.1";
	} else if (len == 15 && strncmp(sym, "GALVINISE_PANTS", len) == 0) {
		value = "Pants On!";
	} else {
		printf("Value not found\n");
	}

	if (value)
		blam->write(blam, value, strlen(value));
	return 0;
}

