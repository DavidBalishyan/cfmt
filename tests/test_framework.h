#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_run = 0;

static int tests_failed = 0;
#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  TEST: %-40s ", #name); \
    fflush(stdout); \
    name(); \
    printf("PASSED\n"); \
} while(0)
#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("\n  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        tests_failed++; \
        return; \
    } \
} while(0)
#define ASSERT_EQ(a, b) do { \
    long long _a = (long long)(a); \
    long long _b = (long long)(b); \
    if (_a != _b) { \
        printf("\n  FAIL: %s:%d: expected %lld, got %lld\n", __FILE__, __LINE__, _b, _a); \
        tests_failed++; \
        return; \
    } \
} while(0)
#define ASSERT_STR_EQ(a, b) do { \
    const char *_a = (a); \
    const char *_b = (b); \
    if (strcmp(_a, _b) != 0) { \
        printf("\n  FAIL: %s:%d:\n    expected: \"%s\"\n    got:      \"%s\"\n", \
               __FILE__, __LINE__, _b, _a); \
        tests_failed++; \
        return; \
    } \
} while(0)
#define ASSERT_NE(a, b) do { \
    long long _a = (long long)(a); \
    long long _b = (long long)(b); \
    if (_a == _b) { \
        printf("\n  FAIL: %s:%d: expected not equal to %lld\n", __FILE__, __LINE__, _b); \
        tests_failed++; \
        return; \
    } \
} while(0)
#define TEST_SUMMARY() do { \
    printf("\n%d tests run, %d passed, %d failed\n", \
           tests_run, tests_run - tests_failed, tests_failed); \
} while(0)
#endif /* TEST_FRAMEWORK_H */