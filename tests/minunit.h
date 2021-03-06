#undef NDEBUG
#ifndef _minunit_h
#define _minunit_h

#include <stdio.h>
#include <dbg.h>
#include <stdlib.h>

#define mu_suite_start() char *message = NULL

#define mu_assert(test, message) {                          \
    if (!(test)) { log_err(message); return message; }      \
}

#define mu_run_test(test) {                                 \
    Debug("\n-----%s", " " #test);                          \
    message = test(); tests_run++;                          \
    if (message) return message;                            \
}

#define RUN_TESTS(test) int main(int argc, char **argv) {   \
    if((argc = 1)) {                                        \
        Debug("----- RUNNING: %s", argv[0]);                \
        printf("----\nRUNNING: %s\n", argv[0]);             \
        char *result = test();                              \
        if (result)                                         \
            printf("FAILED: %s\n", result);                 \
        else                                                \
            printf("ALL TESTS PASSED\n");                   \
        printf("Tests run: %d\n", tests_run);               \
        exit(result != 0);                                  \
    }                                                       \
}


int tests_run;

#endif
