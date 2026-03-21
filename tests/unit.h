#ifndef N64_UNIT_H
#define N64_UNIT_H

#include <stdio.h>
#include <log.h>

static int tests_failed = 0;

#define failed(message,...) if (1) { \
    printf(COLOR_RED "[FAILED] ");\
    printf(message "\n" COLOR_END, ##__VA_ARGS__);\
    tests_failed++;}

#define passed(message,...) if (1) { \
    printf(COLOR_GREEN "[PASSED] ");\
    printf(message "\n" COLOR_END, ##__VA_ARGS__);}

#define ASSERT_TRUE(cond, msg, ...) do { \
    if (cond) { passed(msg, ##__VA_ARGS__); } \
    else { failed(msg, ##__VA_ARGS__); } \
    } while(0)

#define ASSERT_FALSE(cond, msg, ...) ASSERT_TRUE(!(cond), msg, ##__VA_ARGS__)

#define ASSERT_EQ(actual, expected, msg, ...) do { \
    if ((actual) == (expected)) { passed(msg, ##__VA_ARGS__); } \
    else { failed(msg " (expected %llu, got %llu)", ##__VA_ARGS__, \
        (unsigned long long)(expected), (unsigned long long)(actual)); } \
    } while(0)

#endif //N64_UNIT_H
