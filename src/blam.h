

struct blam {
	// FIXME: Should also detect merging, so two writes next to each other
	// get merged.
	int (*write)(struct blam *, const void *datai, size_t len);
	int (*write_string)(struct blam *, const char *str);
	int (*flush)(struct blam *);
	int (*close)(struct blam *);
};
struct blam *blam_init(void *ctx, const char *file);

