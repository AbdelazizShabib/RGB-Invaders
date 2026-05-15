#include <stdio.h>
#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- All nine valid stages ---

void test_stage_0_returns_4(void) {
    int len = getSimonStageLength(0);
    printf("  stage 0 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(4, len);
}
void test_stage_1_returns_5(void) {
    int len = getSimonStageLength(1);
    printf("  stage 1 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(5, len);
}
void test_stage_2_returns_6(void) {
    int len = getSimonStageLength(2);
    printf("  stage 2 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(6, len);
}
void test_stage_3_returns_8(void) {
    int len = getSimonStageLength(3);
    printf("  stage 3 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(8, len);
}
void test_stage_4_returns_9(void) {
    int len = getSimonStageLength(4);
    printf("  stage 4 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(9, len);
}
void test_stage_5_returns_11(void) {
    int len = getSimonStageLength(5);
    printf("  stage 5 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(11, len);
}
void test_stage_6_returns_13(void) {
    int len = getSimonStageLength(6);
    printf("  stage 6 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(13, len);
}
void test_stage_7_returns_15(void) {
    int len = getSimonStageLength(7);
    printf("  stage 7 -> length=%d\n", len);
    TEST_ASSERT_EQUAL(15, len);
}
void test_stage_8_returns_17(void) {
    int len = getSimonStageLength(8);
    printf("  stage 8 -> length=%d  (max / full sequence)\n", len);
    TEST_ASSERT_EQUAL(17, len);
}

// --- Out-of-range stages clamp to 17 ---

void test_negative_stage_returns_17(void) {
    int len = getSimonStageLength(-1);
    printf("  stage=-1 (out-of-range) -> length=%d  (clamped to max)\n", len);
    TEST_ASSERT_EQUAL(17, len);
}
void test_stage_9_returns_17(void) {
    int len = getSimonStageLength(9);
    printf("  stage=9 (out-of-range) -> length=%d  (clamped to max)\n", len);
    TEST_ASSERT_EQUAL(17, len);
}
void test_stage_100_returns_17(void) {
    int len = getSimonStageLength(100);
    printf("  stage=100 (out-of-range) -> length=%d  (clamped to max)\n", len);
    TEST_ASSERT_EQUAL(17, len);
}

// --- Monotonically increasing sequence ---

void test_stages_increase_monotonically(void) {
    printf("  Stage lengths: ");
    for (int i = 0; i <= 8; i++) {
        printf("%d", getSimonStageLength(i));
        if (i < 8) printf(", ");
    }
    printf("  (strictly increasing)\n");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_GREATER_THAN(getSimonStageLength(i),
                                  getSimonStageLength(i + 1));
    }
}

// --- Final stage length matches full sequence length in simonFullSequence ---

void test_final_stage_matches_full_sequence_length(void) {
    int len = getSimonStageLength(8);
    printf("  getSimonStageLength(8) = %d  (matches full simonFullSequence length)\n", len);
    TEST_ASSERT_EQUAL(17, len);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_stage_0_returns_4);
    RUN_TEST(test_stage_1_returns_5);
    RUN_TEST(test_stage_2_returns_6);
    RUN_TEST(test_stage_3_returns_8);
    RUN_TEST(test_stage_4_returns_9);
    RUN_TEST(test_stage_5_returns_11);
    RUN_TEST(test_stage_6_returns_13);
    RUN_TEST(test_stage_7_returns_15);
    RUN_TEST(test_stage_8_returns_17);
    RUN_TEST(test_negative_stage_returns_17);
    RUN_TEST(test_stage_9_returns_17);
    RUN_TEST(test_stage_100_returns_17);
    RUN_TEST(test_stages_increase_monotonically);
    RUN_TEST(test_final_stage_matches_full_sequence_length);
    return UNITY_END();
}
