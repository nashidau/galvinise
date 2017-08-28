#include <check.h>
#include <talloc.h>
#include "lua.h" // fixme, shouldn;t need this
#include "galvinise.h"

static lua_State *galvl;

static void setup_galv(void) {
	galvl = galvinise_init(NULL, NULL);
	lua_newtable(galvl);
}

static void release_galv(void) {
}

static void
push_string(lua_State *L, const char *key, const char *value) {
	lua_pushstring(L, key);
	lua_pushstring(L, value);
	lua_settable(L, -3);
}

START_TEST(test_galv_create) {
	lua_State *L = galvinise_init(NULL, NULL);
	ck_assert_ptr_ne(L, NULL);
} END_TEST

START_TEST(test_galv_env_get) {
	lua_State *L = galvinise_init(NULL, NULL);
	lua_State *L2 = galvinise_environment_get();
	ck_assert_ptr_eq(L, L2);
} END_TEST

START_TEST(test_galv_simple_string) {
	char *out = galvinise_buf("Hello", 5);
	ck_assert_str_eq("Hello", out);
} END_TEST

START_TEST(test_galv_long_string) {
	const char *str =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"function() { some js; }"
		"if (x > y) { x = y; y = x }";
	char *out = galvinise_buf(str, strlen(str));
	ck_assert_str_eq(str, out);
} END_TEST

START_TEST(test_galv_var) {
	const char *str = "$FOO";
	push_string(galvl, "FOO", "Elvis");
	char *out = galvinise_buf(str, strlen(str));
	ck_assert_str_eq("Elvis", out);
} END_TEST

START_TEST(test_galv_many_var) {
	const char *str = "$FOO $BAR $FOO $BAZ";
	push_string(galvl, "FOO", "Elvis");
	push_string(galvl, "BAR", "Is");
	push_string(galvl, "BAZ", "The King");
	char *out = galvinise_buf(str, strlen(str));
	ck_assert_str_eq("Elvis Is Elvis The King", out);
} END_TEST

START_TEST(test_galv_dollar_dollar) {
	char *out = galvinise_buf("$$", 2);
	ck_assert_str_eq("$", out);
} END_TEST

START_TEST(test_galv_function) {
	char *str = "{{ function x() do return 'hello world' end }} $x()";
	char *out = galvinise_buf(str, strlen(str));
	ck_assert_str_eq("hello world", out);
} END_TEST

START_TEST(test_galv_builtins) {

} END_TEST

Suite *galv_suite(void) {
	Suite *s;

	s = suite_create("Galv");
	
	{
		TCase *tc;
		
		tc = tcase_create("Init");
		suite_add_tcase(s, tc);

		tcase_add_test(tc, test_galv_create);
		tcase_add_test(tc, test_galv_env_get);
		// FIXME: Tests testing command line args
	}
	{
		TCase *tc;
		
		tc = tcase_create("Buf");
		suite_add_tcase(s, tc);
		tcase_add_checked_fixture(tc, setup_galv, release_galv);

		tcase_add_test(tc, test_galv_simple_string);
		tcase_add_test(tc, test_galv_long_string);
		tcase_add_test(tc, test_galv_var);
		tcase_add_test(tc, test_galv_many_var);
		tcase_add_test(tc, test_galv_dollar_dollar);
		tcase_add_test(tc, test_galv_function);
		tcase_add_test(tc, test_galv_builtins);
	}

	return s;
}


