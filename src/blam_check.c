/**
 * Tests for Blam, in particlar, blam_buf
 */
#include <stdlib.h>
#include <talloc.h>
#include <check.h>

#include "blam.h"

START_TEST(test_buf_create) {
	struct blam *blam;
	blam = blam_buf_init(NULL);
	ck_assert_ptr_ne(blam, NULL);
	talloc_free(blam);
} END_TEST

START_TEST(test_buf_write_single) {
	struct blam *blam;
	char *str;
	blam = blam_buf_init(NULL);
	blam->write_string(blam, "Hello");
	str = blam_buf_get(blam, blam);
	ck_assert_str_eq(str, "Hello");
	talloc_free(blam);
} END_TEST

START_TEST(test_buf_write_multiple) {
	struct blam *blam;
	char *str;
	blam = blam_buf_init(NULL);
	blam->write_string(blam, "Hello ");
	blam->write_string(blam, "World");
	str = blam_buf_get(blam, blam);
	ck_assert_str_eq(str, "Hello World");
	talloc_free(blam);
} END_TEST

START_TEST(test_buf_write_short) {
	struct blam *blam;
	char *str;
	blam = blam_buf_init(NULL);
	blam->write_string(blam, "I ");
	blam->write(blam, "Don't", 2);
	blam->write(blam, " Love Ponies and Unicorns", 12);
	str = blam_buf_get(blam, blam);
	ck_assert_str_eq(str, "I Do Love Ponies");
	talloc_free(blam);
} END_TEST

Suite *blam_suite(void) {
	Suite *s;

	s = suite_create("Blam");
	
	{
		TCase *tc;
		
		tc = tcase_create("Buf");
		suite_add_tcase(s, tc);

		tcase_add_test(tc, test_buf_create);
		tcase_add_test(tc, test_buf_write_single);
		tcase_add_test(tc, test_buf_write_multiple);
		tcase_add_test(tc, test_buf_write_short);
	}

	return s;
}


