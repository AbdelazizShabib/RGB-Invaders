#include <unity.h>
#include <string.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- Rank R boundary ---

void test_zero_score_is_R(void) {
    TEST_ASSERT_EQUAL_STRING("R", getScoreRankFor(0));
}
void test_score_7999_is_R(void) {
    TEST_ASSERT_EQUAL_STRING("R", getScoreRankFor(7999));
}

// --- Rank C boundary ---

void test_score_8000_is_C(void) {
    TEST_ASSERT_EQUAL_STRING("C", getScoreRankFor(8000));
}
void test_score_8001_is_C(void) {
    TEST_ASSERT_EQUAL_STRING("C", getScoreRankFor(8001));
}
void test_score_19999_is_C(void) {
    TEST_ASSERT_EQUAL_STRING("C", getScoreRankFor(19999));
}

// --- Rank B boundary ---

void test_score_20000_is_B(void) {
    TEST_ASSERT_EQUAL_STRING("B", getScoreRankFor(20000));
}
void test_score_39999_is_B(void) {
    TEST_ASSERT_EQUAL_STRING("B", getScoreRankFor(39999));
}

// --- Rank A boundary ---

void test_score_40000_is_A(void) {
    TEST_ASSERT_EQUAL_STRING("A", getScoreRankFor(40000));
}
void test_score_69999_is_A(void) {
    TEST_ASSERT_EQUAL_STRING("A", getScoreRankFor(69999));
}

// --- Rank S boundary ---

void test_score_70000_is_S(void) {
    TEST_ASSERT_EQUAL_STRING("S", getScoreRankFor(70000));
}
void test_score_99999_is_S(void) {
    TEST_ASSERT_EQUAL_STRING("S", getScoreRankFor(99999));
}

// --- Rank S+ boundary ---

void test_score_100000_is_Splus(void) {
    TEST_ASSERT_EQUAL_STRING("S+", getScoreRankFor(100000));
}
void test_score_100001_is_Splus(void) {
    TEST_ASSERT_EQUAL_STRING("S+", getScoreRankFor(100001));
}
void test_score_max_int_is_Splus(void) {
    TEST_ASSERT_EQUAL_STRING("S+", getScoreRankFor(2147483647));
}

// --- Negative score falls through to R (no crash) ---

void test_negative_score_is_R(void) {
    TEST_ASSERT_EQUAL_STRING("R", getScoreRankFor(-1));
    TEST_ASSERT_EQUAL_STRING("R", getScoreRankFor(-100000));
}

// --- Return value is never NULL ---

void test_return_is_never_null(void) {
    TEST_ASSERT_NOT_NULL(getScoreRankFor(0));
    TEST_ASSERT_NOT_NULL(getScoreRankFor(50000));
    TEST_ASSERT_NOT_NULL(getScoreRankFor(100000));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_score_is_R);
    RUN_TEST(test_score_7999_is_R);
    RUN_TEST(test_score_8000_is_C);
    RUN_TEST(test_score_8001_is_C);
    RUN_TEST(test_score_19999_is_C);
    RUN_TEST(test_score_20000_is_B);
    RUN_TEST(test_score_39999_is_B);
    RUN_TEST(test_score_40000_is_A);
    RUN_TEST(test_score_69999_is_A);
    RUN_TEST(test_score_70000_is_S);
    RUN_TEST(test_score_99999_is_S);
    RUN_TEST(test_score_100000_is_Splus);
    RUN_TEST(test_score_100001_is_Splus);
    RUN_TEST(test_score_max_int_is_Splus);
    RUN_TEST(test_negative_score_is_R);
    RUN_TEST(test_return_is_never_null);
    return UNITY_END();
}
