#include <stdlib.h>
#include <check.h>
#include "suites.h"

int main(int argc, char **argv)
{
	int number_failed;

	Suite *s = array_suite();

	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
