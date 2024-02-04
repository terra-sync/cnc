#include <CUnit/Basic.h>

#include "cnc.h"

int init_suite(void)
{
	return 0;
}
int clean_suite(void)
{
	return 0;
}

void test_config(void)
{
	char *args[] = {
		"./cnc",
		"--config-file",
		"configs/test.ini",
	};
	int argc = sizeof(args) / sizeof(args[0]);

	int result = process_args(argc, args);
	CU_ASSERT_EQUAL(result, 0);
}

void test_missing_config(void)
{
	char *args[] = {
		"./cnc",
		"--config-file",
		"configs/non-existent.ini",
	};
	int argc = sizeof(args) / sizeof(args[0]);

	int result = process_args(argc, args);
	CU_ASSERT_EQUAL(result, -1);
}

typedef struct CU_test_info {
	const char *test_name;
	void (*test_func)(void);

} CU_test_info;

CU_test_info test_cases[] = {
	{ "test_config", test_config },
	{ "test_missing_config", test_missing_config },
	{ NULL, NULL } // Array must be NULL terminated
};

int add_tests(CU_pSuite pSuite, CU_test_info test_cases[])
{
	int i = 0;
	while (test_cases[i].test_name != NULL &&
	       test_cases[i].test_func != NULL) {
		if (NULL == CU_add_test(pSuite, test_cases[i].test_name,
					test_cases[i].test_func)) {
			CU_cleanup_registry();
			return CU_get_error();
		}
		i++;
	}

	return CUE_SUCCESS;
}

int main()
{
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	CU_pSuite p_suite = CU_add_suite("Suite_1", init_suite, clean_suite);
	if (NULL == p_suite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the tests to the suite */
	if (CUE_SUCCESS != add_tests(p_suite, test_cases)) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
