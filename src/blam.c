/* Helper library to manage array of iov */
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <talloc.h>

#include "blam.h"

struct blam_internal {
	struct blam blam;
	int fd;
};

static int blam_method_write(struct blam *, const void *data, int len);
static int blam_method_write_string(struct blam *, const char *data);
static int blam_method_flush(struct blam *);
static int blam_method_close(struct blam *);

struct blam *
blam_init(void *ctx, const char *file) {
	struct blam_internal *blam = talloc_zero(ctx, struct blam_internal);
	if (!blam) return NULL;

	blam->blam.write = blam_method_write;
	blam->blam.write_string = blam_method_write_string;
	blam->blam.flush = blam_method_flush;
	blam->blam.close = blam_method_close;

	if (!file) {
		blam->fd = STDOUT_FILENO;
	} else {
		blam->fd = creat(file, 0666);
		if (blam->fd == -1) {
			perror("creat()");
			return NULL;
		}
	}
	return &blam->blam;
}

static int
blam_method_write(struct blam *b, const void *data, int len) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	return write(blam->fd, data, len);
}

static int
blam_method_write_string(struct blam *b, const char *data) {
	return b->write(b, data, strlen(data));
}

static int
blam_method_flush(struct blam *x) {
	return 0;
}
static int
blam_method_close(struct blam *b) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	if (blam->fd != -1) {
		close(blam->fd);
		blam->fd = -1;
	}
	return 0;
}
