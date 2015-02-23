/* Helper library to manage array of iov */
#include <fcntl.h>
#include <string.h>
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
