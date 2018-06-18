#include "minunit.h"

char *test_sample()
{

    if (0) return "Example return";

    return NULL;
}



char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_sample);

    return NULL;
}

RUN_TESTS(all_tests);
