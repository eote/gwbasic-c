/* GW-BASIC C Port - Test Framework */
/* Simple assert macros for unit testing */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global test statistics */
extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;

#define TEST_ASSERT(cond, msg) \
    do { \
        g_tests_run++; \
        if (!(cond)) { \
            g_tests_failed++; \
            fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); \
        } else { \
            g_tests_passed++; \
            printf("  PASS: %s\n", msg); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_INT(actual, expected, msg) \
    do { \
        g_tests_run++; \
        if ((actual) != (expected)) { \
            g_tests_failed++; \
            fprintf(stderr, "  FAIL [%s:%d] %s: got %d, expected %d\n", \
                    __FILE__, __LINE__, msg, (actual), (expected)); \
        } else { \
            g_tests_passed++; \
            printf("  PASS: %s\n", msg); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_STR(actual, expected, msg) \
    do { \
        g_tests_run++; \
        if (strcmp((actual), (expected)) != 0) { \
            g_tests_failed++; \
            fprintf(stderr, "  FAIL [%s:%d] %s: got \"%s\", expected \"%s\"\n", \
                    __FILE__, __LINE__, msg, (actual), (expected)); \
        } else { \
            g_tests_passed++; \
            printf("  PASS: %s\n", msg); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_FLOAT(actual, expected, msg) \
    do { \
        g_tests_run++; \
        if ((actual) != (expected)) { \
            g_tests_failed++; \
            fprintf(stderr, "  FAIL [%s:%d] %s: got %f, expected %f\n", \
                    __FILE__, __LINE__, msg, (actual), (expected)); \
        } else { \
            g_tests_passed++; \
            printf("  PASS: %s\n", msg); \
        } \
    } while (0)

#define TEST_ASSERT_EQ_WORD(actual, expected, msg) \
    do { \
        g_tests_run++; \
        if ((actual) != (expected)) { \
            g_tests_failed++; \
            fprintf(stderr, "  FAIL [%s:%d] %s: got %u, expected %u\n", \
                    __FILE__, __LINE__, msg, (actual), (expected)); \
        } else { \
            g_tests_passed++; \
            printf("  PASS: %s\n", msg); \
        } \
    } while (0)

#define TEST_PASS(msg) \
    do { \
        printf("  PASS: %s\n", msg); \
    } while (0)

typedef void (*test_func_t)(void);

int run_test(const char *name, test_func_t test_func);
void print_test_summary(void);

#endif