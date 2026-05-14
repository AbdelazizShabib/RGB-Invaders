#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- All nine valid stages ---

void test_stage_0_returns_4(void) {
    TEST_ASSERT_EQUAL(4, getSimonStageLength(0));
}
void test_stage_1_returns_5(void) {
    TEST_ASSERT_EQUAL(5, getSimonStageLength(1));
}
void test_stage_2_returns_6(void) {
    TEST_ASSERT_EQUAL(6, getSimonStageLength(2));
}
void test_stage_3_returns_8(void) {
    TEST_ASSERT_EQUAL(8, getSimonStageLength(3));
}
void test_stage_4_returns_9(void) {
    TEST_ASSERT_EQUAL(9, getSimonStageLength(4));
}
void test_stage_5_returns_11(void) {
    TEST_ASSERT_EQUAL(11, getSimonStageLength(5));
}
void test_stage_6_returns_13(void) {
    TEST_ASSERT_EQUAL(13, getSimonStageLength(6));
}
void test_stage_7_returns_15(void) {
    TEST_ASSERT_EQUAL(15, getSimonStageLength(7));
}
void test_stage_8_returns_17(void) {
    TEST_ASSERT_EQUAL(17, getSimonStageLength(8));
}

// --- Out-of-range stages clamp to 17 ---

void test_negative_stage_returns_17(void) {
    TEST_ASSERT_EQUAL(17, getSimonStageLength(-1));
}
void test_stage_9_returns_17(void) {
    TEST_ASSERT_EQUAL(17, getSimonStageLength(9));
}
void test_stage_100_returns_17(void) {
    TEST_ASSERT_EQUAL(17, getSimonStageLength(100));
}

// --- Monotonically increasing sequence ---

void test_stages_increase_monotonically(void) {
    for (int i = 0; i < 8; i++) {
        // Each later stage requires more inputs than the previous.
        TEST_ASSERT_GREATER_THAN(getSimonStageLength(i),
                                  getSimonStageLength(i + 1));
    }
}

// --- Final stage length matches full sequence length in simonFullSequence ---

void test_final_stage_matches_full_sequence_length(void) {
    // simonFullSequence has 17 elements; stage 8 must consume all of them.
    TEST_ASSERT_EQUAL(17, getSimonStageLength(8));
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
