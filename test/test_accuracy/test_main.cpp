#include <stdio.h>
#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- Zero / invalid denominator ---

void test_zero_max_returns_0(void) {
    int result = computeAccuracy(0, 0);
    printf("  computeAccuracy(0, 0) = %d\n", result);
    TEST_ASSERT_EQUAL(0, result);
}
void test_negative_max_returns_0(void) {
    int r1 = computeAccuracy(500, -1);
    int r2 = computeAccuracy(500, -9999);
    printf("  computeAccuracy(500, -1) = %d  |  computeAccuracy(500, -9999) = %d\n", r1, r2);
    TEST_ASSERT_EQUAL(0, r1);
    TEST_ASSERT_EQUAL(0, r2);
}

// --- Standard computations ---

void test_full_score_returns_100(void) {
    int result = computeAccuracy(1000, 1000);
    printf("  computeAccuracy(1000, 1000) = %d%%\n", result);
    TEST_ASSERT_EQUAL(100, result);
}
void test_half_score_returns_50(void) {
    int result = computeAccuracy(500, 1000);
    printf("  computeAccuracy(500, 1000) = %d%%\n", result);
    TEST_ASSERT_EQUAL(50, result);
}
void test_quarter_score_returns_25(void) {
    int result = computeAccuracy(250, 1000);
    printf("  computeAccuracy(250, 1000) = %d%%\n", result);
    TEST_ASSERT_EQUAL(25, result);
}
void test_zero_achieved_returns_0(void) {
    int result = computeAccuracy(0, 1000);
    printf("  computeAccuracy(0, 1000) = %d%%\n", result);
    TEST_ASSERT_EQUAL(0, result);
}

// --- Overflow clamping ---

void test_achieved_greater_than_max_clamps_to_100(void) {
    int r1 = computeAccuracy(999999, 1);
    int r2 = computeAccuracy(200, 100);
    printf("  computeAccuracy(999999, 1) = %d  |  computeAccuracy(200, 100) = %d  (clamped to 100)\n", r1, r2);
    TEST_ASSERT_EQUAL(100, r1);
    TEST_ASSERT_EQUAL(100, r2);
}

// --- Clamp guarantees ---

void test_result_never_exceeds_100(void) {
    int r1 = computeAccuracy(100000, 1);
    int r2 = computeAccuracy(1, 1);
    printf("  computeAccuracy(100000, 1) = %d  |  computeAccuracy(1, 1) = %d  (both <= 100)\n", r1, r2);
    TEST_ASSERT_LESS_OR_EQUAL(100, r1);
    TEST_ASSERT_LESS_OR_EQUAL(100, r2);
}
void test_result_never_below_0(void) {
    int r1 = computeAccuracy(0, 1000);
    int r2 = computeAccuracy(-100, 1000);
    printf("  computeAccuracy(0, 1000) = %d  |  computeAccuracy(-100, 1000) = %d  (both >= 0)\n", r1, r2);
    TEST_ASSERT_GREATER_OR_EQUAL(0, r1);
    TEST_ASSERT_GREATER_OR_EQUAL(0, r2);
}

// --- Edge: 1/1 = 100% ---

void test_one_out_of_one_is_100(void) {
    int result = computeAccuracy(1, 1);
    printf("  computeAccuracy(1, 1) = %d%%\n", result);
    TEST_ASSERT_EQUAL(100, result);
}

// --- Typical in-game values ---

void test_typical_level1_perfect_score(void) {
    int result = computeAccuracy(6000, 6000);
    printf("  computeAccuracy(6000, 6000) = %d%%  [level-1 perfect clear]\n", result);
    TEST_ASSERT_EQUAL(100, result);
}
void test_typical_half_performance(void) {
    int result = computeAccuracy(3000, 6000);
    printf("  computeAccuracy(3000, 6000) = %d%%  [half performance]\n", result);
    TEST_ASSERT_EQUAL(50, result);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_max_returns_0);
    RUN_TEST(test_negative_max_returns_0);
    RUN_TEST(test_full_score_returns_100);
    RUN_TEST(test_half_score_returns_50);
    RUN_TEST(test_quarter_score_returns_25);
    RUN_TEST(test_zero_achieved_returns_0);
    RUN_TEST(test_achieved_greater_than_max_clamps_to_100);
    RUN_TEST(test_result_never_exceeds_100);
    RUN_TEST(test_result_never_below_0);
    RUN_TEST(test_one_out_of_one_is_100);
    RUN_TEST(test_typical_level1_perfect_score);
    RUN_TEST(test_typical_half_performance);
    return UNITY_END();
}
