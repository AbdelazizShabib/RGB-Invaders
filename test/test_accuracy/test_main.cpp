#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- Zero / invalid denominator ---

void test_zero_max_returns_0(void) {
    TEST_ASSERT_EQUAL(0, computeAccuracy(0, 0));
}
void test_negative_max_returns_0(void) {
    TEST_ASSERT_EQUAL(0, computeAccuracy(500, -1));
    TEST_ASSERT_EQUAL(0, computeAccuracy(500, -9999));
}

// --- Standard computations ---

void test_full_score_returns_100(void) {
    TEST_ASSERT_EQUAL(100, computeAccuracy(1000, 1000));
}
void test_half_score_returns_50(void) {
    TEST_ASSERT_EQUAL(50, computeAccuracy(500, 1000));
}
void test_quarter_score_returns_25(void) {
    TEST_ASSERT_EQUAL(25, computeAccuracy(250, 1000));
}
void test_zero_achieved_returns_0(void) {
    TEST_ASSERT_EQUAL(0, computeAccuracy(0, 1000));
}

// --- Overflow clamping ---

void test_achieved_greater_than_max_clamps_to_100(void) {
    // e.g. bonus points pushed score above theoretical max
    TEST_ASSERT_EQUAL(100, computeAccuracy(999999, 1));
    TEST_ASSERT_EQUAL(100, computeAccuracy(200,    100));
}

// --- Clamp guarantees ---

void test_result_never_exceeds_100(void) {
    TEST_ASSERT_LESS_OR_EQUAL(100, computeAccuracy(100000, 1));
    TEST_ASSERT_LESS_OR_EQUAL(100, computeAccuracy(1, 1));
}
void test_result_never_below_0(void) {
    TEST_ASSERT_GREATER_OR_EQUAL(0, computeAccuracy(0, 1000));
    TEST_ASSERT_GREATER_OR_EQUAL(0, computeAccuracy(-100, 1000));
}

// --- Edge: 1/1 = 100% ---

void test_one_out_of_one_is_100(void) {
    TEST_ASSERT_EQUAL(100, computeAccuracy(1, 1));
}

// --- Typical in-game values ---

void test_typical_level1_perfect_score(void) {
    // achieved == maxPossible after a fast level-1 clear
    TEST_ASSERT_EQUAL(100, computeAccuracy(6000, 6000));
}
void test_typical_half_performance(void) {
    TEST_ASSERT_EQUAL(50, computeAccuracy(3000, 6000));
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
