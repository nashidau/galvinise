

struct blam {
	// FIXME: Should also detect merging, so two writes next to each other
	// get merged.
	int (*write)(struct blam *, const void *datai, size_t len);
	int (*write_string)(struct blam *, const char *str);
	int (*flush)(struct blam *);
	int (*close)(struct blam *);
};

#define blam_init(ctx, file)	blam_direct_init(ctx, file)
struct blam *blam_direct_init(void *ctx, const char *file);
struct blam *blam_writev_init(void *ctx, int nvec, const char *file);
/*FIXME: Ifdef around onion */
struct onion_response_t;
struct blam *blam_onion_init(void *ctx, struct onion_response_t *res);
