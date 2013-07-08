/**
 * Slurp a file in from disk into memory.
 *
 * The current implementation reads the whole file.  Future implementations may
 * try some fancy on demand loading or something.
 */

#include <errno.h>
#include <unistd.h>

#include <talloc.h>

const char *
diskslurp(int fd, size_t remaining) {
	char *buf, *cur;

	if (remaining < 1) return NULL;

	buf = talloc_size(NULL, remaining);
	if (!buf) return NULL;

	cur = buf;
	do {
		ssize_t thisread;

		errno = 0;
		thisread = read(fd, cur, remaining);
		if (thisread < 0) {
			// Thanks libc, don't know where things are now.
			talloc_free(buf);
			return NULL;
		}
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
			perror("read");
			talloc_free(buf);
		}

		cur += thisread;
		remaining -= thisread;
	} while (remaining > 0);

	return buf;
}

