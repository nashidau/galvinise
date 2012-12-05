

struct blobctx;

struct blobctx *blob_init(void *ctx, int size);

int blob_write(struct blobctx *, void *buf, int len);
int blob_flush(struct blobctx *);

