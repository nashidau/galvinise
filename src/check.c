#include <check.h>

Suite *blam_suite(void);
Suite *galv_suite(void);

typedef Suite *(*test_module)(void);

static test_module test_modules[] = {
	blam_suite,
	galv_suite
};
#define N_MODULES ((int)(sizeof(test_modules)/sizeof(test_modules[0])))

int main(int argc, char **argv) {
	SRunner *sr;
	int nfailed;
	int i;

	sr = srunner_create(NULL);
	for (i = 0 ; i < N_MODULES ; i ++) {
		srunner_add_suite(sr, test_modules[i]());
	}

	srunner_run_all(sr, CK_NORMAL);
	nfailed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return !!nfailed;
}
