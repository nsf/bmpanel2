#include "../array.h"
#include "suites.h"

static struct memory_source msrc = MEMSRC(
		"Test", 
		MEMSRC_DEFAULT_MALLOC, 
		MEMSRC_DEFAULT_FREE,
		MEMSRC_NO_FLAGS
);

START_TEST(test_array_init)
{
	DYN_ARRAY(int, array);
	DYN_ARRAY_NULLIFY(array);
	fail_unless(array == 0 && array_n == 0 && array_alloc == 0,
			"Array should be nullified correctly");
	DYN_ARRAY_ALLOCATE(array, 10, &msrc);
	fail_unless(array != 0 && array_n == 0 && array_alloc == 10,
			"Failed to allocate array correctly");
	DYN_ARRAY_FREE(array, &msrc);
	fail_unless(msrc.allocs == msrc.frees,
			"Memory leak");
}
END_TEST

Suite *array_suite()
{
	Suite *s = suite_create("Array");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, test_array_init);
	suite_add_tcase(s, tc_core);

	return s;
}
