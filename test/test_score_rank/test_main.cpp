#include <stdio.h>
#include <unity.h>
#include <string.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- Rank R boundary ---

void test_zero_score_is_R(void) {
    const char *rank = getScoreRankFor(0);
    printf("  score=0 -> rank=\"%s\"\n", rank);
    TEST_ASSERT_EQUAL_STRING("R", rank);
}
void test_score_7999_is_R(void) {
    const char *rank = getScoreRankFor(7999);
    printf("  score=7999 -> rank=\"%s\"  (R ceiling)\n", rank);
    TEST_ASSERT_EQUAL_STRING("R", rank);
}

// --- Rank C boundary ---

void test_score_8000_is_C(void) {
    const char *rank = getScoreRankFor(8000);
    printf("  score=8000 -> rank=\"%s\"  (C floor)\n", rank);
    TEST_ASSERT_EQUAL_STRING("C", rank);
}
void test_score_8001_is_C(void) {
    const char *rank = getScoreRankFor(8001);
    printf("  score=8001 -> rank=\"%s\"\n", rank);
    TEST_ASSERT_EQUAL_STRING("C", rank);
}
void test_score_19999_is_C(void) {
    const char *rank = getScoreRankFor(19999);
    printf("  score=19999 -> rank=\"%s\"  (C ceiling)\n", rank);
    TEST_ASSERT_EQUAL_STRING("C", rank);
}

// --- Rank B boundary ---

void test_score_20000_is_B(void) {
    const char *rank = getScoreRankFor(20000);
    printf("  score=20000 -> rank=\"%s\"  (B floor)\n", rank);
    TEST_ASSERT_EQUAL_STRING("B", rank);
}
void test_score_39999_is_B(void) {
    const char *rank = getScoreRankFor(39999);
    printf("  score=39999 -> rank=\"%s\"  (B ceiling)\n", rank);
    TEST_ASSERT_EQUAL_STRING("B", rank);
}

// --- Rank A boundary ---

void test_score_40000_is_A(void) {
    const char *rank = getScoreRankFor(40000);
    printf("  score=40000 -> rank=\"%s\"  (A floor)\n", rank);
    TEST_ASSERT_EQUAL_STRING("A", rank);
}
void test_score_69999_is_A(void) {
    const char *rank = getScoreRankFor(69999);
    printf("  score=69999 -> rank=\"%s\"  (A ceiling)\n", rank);
    TEST_ASSERT_EQUAL_STRING("A", rank);
}

// --- Rank S boundary ---

void test_score_70000_is_S(void) {
    const char *rank = getScoreRankFor(70000);
    printf("  score=70000 -> rank=\"%s\"  (S floor)\n", rank);
    TEST_ASSERT_EQUAL_STRING("S", rank);
}
void test_score_99999_is_S(void) {
    const char *rank = getScoreRankFor(99999);
    printf("  score=99999 -> rank=\"%s\"  (S ceiling)\n", rank);
    TEST_ASSERT_EQUAL_STRING("S", rank);
}

// --- Rank S+ boundary ---

void test_score_100000_is_Splus(void) {
    const char *rank = getScoreRankFor(100000);
    printf("  score=100000 -> rank=\"%s\"  (S+ floor)\n", rank);
    TEST_ASSERT_EQUAL_STRING("S+", rank);
}
void test_score_100001_is_Splus(void) {
    const char *rank = getScoreRankFor(100001);
    printf("  score=100001 -> rank=\"%s\"\n", rank);
    TEST_ASSERT_EQUAL_STRING("S+", rank);
}
void test_score_max_int_is_Splus(void) {
    const char *rank = getScoreRankFor(2147483647);
    printf("  score=2147483647 (INT_MAX) -> rank=\"%s\"\n", rank);
    TEST_ASSERT_EQUAL_STRING("S+", rank);
}

// --- Negative score falls through to R (no crash) ---

void test_negative_score_is_R(void) {
    const char *r1 = getScoreRankFor(-1);
    const char *r2 = getScoreRankFor(-100000);
    printf("  score=-1 -> \"%s\"  |  score=-100000 -> \"%s\"\n", r1, r2);
    TEST_ASSERT_EQUAL_STRING("R", r1);
    TEST_ASSERT_EQUAL_STRING("R", r2);
}

// --- Return value is never NULL ---

void test_return_is_never_null(void) {
    const char *r0 = getScoreRankFor(0);
    const char *r1 = getScoreRankFor(50000);
    const char *r2 = getScoreRankFor(100000);
    printf("  getScoreRankFor(0)=\"%s\"  getScoreRankFor(50000)=\"%s\"  getScoreRankFor(100000)=\"%s\"  (all non-NULL)\n",
           r0, r1, r2);
    TEST_ASSERT_NOT_NULL(r0);
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
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
