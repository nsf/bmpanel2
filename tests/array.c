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
	DYN_ARRAY_ALLOCATE(array, 10, &msrc);
	fail_unless(array != 0 && array_n == 0 && array_alloc == 10,
			"Failed to allocate array correctly");
	DYN_ARRAY_FREE(array, &msrc);
}
END_TEST

START_TEST(test_array_ensure_add_capacity)
{
	DYN_ARRAY(int, array);
	DYN_ARRAY_ALLOCATE(array, 5, &msrc);

	DYN_ARRAY_ENSURE_ADD_CAPACITY(array, 10, &msrc);
	fail_unless(array_alloc >= 10,
			"Should be enough space");

	DYN_ARRAY_FREE(array, &msrc);
}
END_TEST

START_TEST(test_array_shrink)
{
	DYN_ARRAY(int, array);
	DYN_ARRAY_ALLOCATE(array, 5, &msrc);

	DYN_ARRAY_SHRINK(array, &msrc);
	fail_unless(array == 0 && array_n == 0 && array_alloc == 0,
			"Should be freed");
}
END_TEST

Suite *array_suite()
{
	Suite *s = suite_create("Array");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, test_array_init);
	tcase_add_test(tc_core, test_array_ensure_add_capacity);
	suite_add_tcase(s, tc_core);

	return s;
}
