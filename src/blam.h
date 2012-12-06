

struct blam {
	int (*write)(struct blam *, const void *data, int len);
	int (*flush)(struct blam *);
	int (*close)(struct blam *);
};
struct blam *blam_init(void *ctx, const char *file);

