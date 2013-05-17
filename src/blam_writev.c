/* Helper library to manage array of iov */
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <talloc.h>

#include "blam.h"

enum {
	NVEC_DEFAULT = 20
};

struct blam_internal {
	struct blam blam;

	int fd;

	int nvec;
	int next;
	struct iovec *vec;
};

static int blamv_method_write(struct blam *, const void *data, size_t len);
static int blamv_method_write_string(struct blam *, const char *data);
static int blamv_method_flush(struct blam *);
static int blamv_method_close(struct blam *);

static int blamv_flush_internal(struct blam_internal *blam);

struct blam *
blam_writev_init(void *ctx, int nvec, const char *file) {
	struct blam_internal *blam = talloc_zero(ctx, struct blam_internal);
	if (!blam) return NULL;

	if (nvec < 0) nvec = NVEC_DEFAULT;

	blam->nvec = nvec;
	blam->next = 0;
	blam->vec = talloc_array(blam, struct iovec, nvec);
	if (!blam->vec) {
		talloc_free(blam);
		return NULL;
	}

	blam->blam.write = blamv_method_write;
	blam->blam.write_string = blamv_method_write_string;
	blam->blam.flush = blamv_method_flush;
	blam->blam.close = blamv_method_close;

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
blamv_method_write(struct blam *b, const void *data, size_t len) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	blam->vec[blam->next].iov_base = (void *)data;
	blam->vec[blam->next].iov_len = len;
	blam->next ++;

	if (blam->next == blam->nvec)
		blamv_flush_internal(blam);


	return write(blam->fd, data, len);
}

static int
blamv_method_write_string(struct blam *b, const char *data) {
	// FIXME: A new blam generic which implements generic versions of this
	return b->write(b, data, strlen(data));
}

static int
blamv_method_flush(struct blam *b) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	if (blam->next != 0)
		return blamv_flush_internal(blam);
	else
		return 0;
}

static int
blamv_method_close(struct blam *b) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);

	if (blam->next != 0)
		blamv_flush_internal(blam);

	if (blam->fd != -1) {
		close(blam->fd);
		blam->fd = -1;
	}
	return 0;
}

static int
blamv_flush_internal(struct blam_internal *blam) {
	/* FIXME: Better error handling here */
	int n = writev(blam->fd, blam->vec, blam->next);
	if (n < 0) {
		printf("Failed to write vector\n");
		return 0;
	}

	blam->next = 0;
	return 0;
}
