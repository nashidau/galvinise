#include <check.h>
#include <stdarg.h>
#include <talloc.h>

#include "galv_check.h"
#include "blam_check.h"

Suite *blam_suite(void *ctx);

typedef Suite *(*test_module)(void *ctx);

static test_module test_modules[] = {
	blam_suite,
	galv_suite
};
#define N_MODULES ((int)(sizeof(test_modules)/sizeof(test_modules[0])))

int main(int argc, char **argv) {
	void *ctx;
	SRunner *sr;
	int nfailed;
	int i;

	ctx = talloc_init("Test Context");

	sr = srunner_create(NULL);
	for (i = 0 ; i < N_MODULES ; i ++) {
		srunner_add_suite(sr, test_modules[i](ctx));
	}

	srunner_run_all(sr, CK_VERBOSE);
	nfailed = srunner_ntests_failed(sr);
	srunner_free(sr);
	talloc_free(ctx);
	return !!nfailed;
}
