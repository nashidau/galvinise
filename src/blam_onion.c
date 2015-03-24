/* Helper library to manage array of iov */
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <talloc.h>

#include <onion/response.h>

#include "blam.h"

struct blam_internal {
	struct blam blam;
	onion_response *res;
};

static int blam_method_write(struct blam *, const void *data, size_t len);
static int blam_method_write_string(struct blam *, const char *data);
static int blam_method_flush(struct blam *);
static int blam_method_close(struct blam *);
static int blam_method_write_file(struct blam *, const char *filename);

struct blam *
blam_onion_init(void *ctx, onion_response *res) {
	struct blam_internal *blam;

 	if (!res) return NULL;	

	blam = talloc_zero(ctx, struct blam_internal);
	if (!blam) return NULL;

	blam->blam.write = blam_method_write;
	blam->blam.write_string = blam_method_write_string;
	blam->blam.flush = blam_method_flush;
	blam->blam.close = blam_method_close;
	blam->blam.write_file = blam_method_write_file;

	blam->res = res;

	return &blam->blam;
}

static int
blam_method_write(struct blam *b, const void *data, size_t len) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	return onion_response_write(blam->res, data, len);
}

static int
blam_method_write_string(struct blam *b, const char *data) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	return onion_response_write0(blam->res, data);
}

static int
blam_method_flush(struct blam *b) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	return onion_response_flush(blam->res);
}
static int
blam_method_close(struct blam *b) {
	/* Do nothing on a close */
	return 0;
}

static int
blam_method_write_file(struct blam *b, const char *file) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	int fd;
	struct stat st;
	void *addr;
	int n;

	fd = open(file, O_RDONLY);
	if (fd < 0) return -1;

	if (fstat(fd, &st) != 0) {
		close(fd);
		return -1;
	}

	addr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		return -1;
	}

	n = onion_response_write(blam->res, addr, st.st_size);
	// FIXME: Warning or error if not all written
	
	munmap(addr, st.st_size);
	close(fd);
	return n;
}
