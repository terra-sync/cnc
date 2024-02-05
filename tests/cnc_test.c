#include <CUnit/Basic.h>

#include "cnc.h"
#include "config.h"

#include <stdlib.h>
#include <ini.h>

extern config_t *ini_config;

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

void test_backup_full(void)
{
	ini_config = (config_t *)malloc(sizeof(config_t));
	ini_config->postgres_config = (postgres_t *)malloc(sizeof(postgres_t));
	ini_config->smtp_config = (smtp_t *)malloc(sizeof(smtp_t));
	ini_parse_string("[postgres]\nbackupt_type=full", handler, ini_config);
	CU_ASSERT_EQUAL(ini_config->postgres_config->backup_type, FULL);
}

void test_backup_schema_only(void)
{
	ini_config = (config_t *)malloc(sizeof(config_t));
	ini_config->postgres_config = (postgres_t *)malloc(sizeof(postgres_t));
	ini_config->smtp_config = (smtp_t *)malloc(sizeof(smtp_t));
	ini_parse_string("[postgres]\nbackup_type=schema", handler, ini_config);
	CU_ASSERT_EQUAL(ini_config->postgres_config->backup_type, SCHEMA);
}

void test_invalid_backup_type(void)
{
	int result;
	ini_config = (config_t *)malloc(sizeof(config_t));
	ini_config->postgres_config = (postgres_t *)malloc(sizeof(postgres_t));
	ini_config->smtp_config = (smtp_t *)malloc(sizeof(smtp_t));
	result = ini_parse_string("[postgres]\nbackup_type=wrong_type", handler,
				  ini_config);
	CU_ASSERT_EQUAL(result, 2);
}

void test_empty_backup_type(void)
{
	int result;
	ini_config = (config_t *)malloc(sizeof(config_t));
	ini_config->postgres_config = (postgres_t *)malloc(sizeof(postgres_t));
	ini_config->smtp_config = (smtp_t *)malloc(sizeof(smtp_t));
	result = ini_parse_string("[postgres]\nbackup_type=", handler,
				  ini_config);
	CU_ASSERT_EQUAL(result, 2);
}

typedef struct CU_test_info {
	const char *test_name;
	void (*test_func)(void);

} CU_test_info;

CU_test_info test_cases[] = {
	{ "test_config", test_config },
	{ "test_missing_config", test_missing_config },
	{ "test_backup_full", test_backup_full },
	{ "test_backup_schema_only", test_backup_schema_only },
	{ "test_invalid_backup_type", test_invalid_backup_type },
	{ "test_empty_backup_type", test_empty_backup_type },
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
