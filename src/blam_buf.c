/* Helper library to manage array of iov */
#include <string.h>

#include <talloc.h>

#include "blam.h"

struct buf;

struct blam_internal {
	struct blam blam;
	int len;
	struct buf *first, *last;
};

struct buf {
	struct buf *next;
	int len;
	char *str;
};

static int blam_method_write(struct blam *, const void *data, size_t len);
static int blam_method_write_string(struct blam *, const char *data);
static int blam_method_flush(struct blam *);
static int blam_method_close(struct blam *);

struct blam *
blam_buf_init(void *ctx) {
	struct blam_internal *blam = talloc_zero(ctx, struct blam_internal);
	if (!blam) return NULL;

	blam->blam.write = blam_method_write;
	blam->blam.write_string = blam_method_write_string;
	blam->blam.flush = blam_method_flush;
	blam->blam.close = blam_method_close;

	blam->first = blam->last = NULL;

	return &blam->blam;
}


char *
blam_buf_get(struct blam *b, void *ctx) {
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);
	char *out, *c;
	struct buf *buf;

	out = talloc_array(ctx, char, blam->len + 1);
	if (!out) return NULL;
	
	c = out;
	for (buf = blam->first ; buf ; buf = buf->next) {
		memcpy(c, buf->str, buf->len);
		c += buf->len;
	}
	c[blam->len] = 0;

	return out;
}

static int
blam_method_write(struct blam *b, const void *data, size_t len) {
	struct buf *buf;
	struct blam_internal *blam = talloc_get_type(b, struct blam_internal);

	if (!blam) return -1;
	if (data == NULL) return 0;

	buf = talloc(b, struct buf);

	if (len == 0) {
		len = strlen(data);
	}

	buf->str = talloc_strndup(b, data, len);
	buf->len = len;
	blam->len += len;

	if (blam->first == NULL)
		blam->first = buf;
	buf->next = NULL;
	if (blam->last)
		blam->last->next = buf;
	blam->last = buf;
	
	return 0;
}

static int
blam_method_write_string(struct blam *b, const char *data) {
	return b->write(b, data, 0);
}

static int
blam_method_flush(struct blam *x) {
	return 0;
}
static int
blam_method_close(struct blam *b) {
	return 0;
}
