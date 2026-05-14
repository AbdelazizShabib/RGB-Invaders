#include <unity.h>
#include "game_logic.h"

// Matches CONFIG_NUM_LEDS = 100 in the firmware.
#define NUM_LEDS 100

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Helper: targetTime for a normal wave level (bossType=0).
//   targetTime = 3000 + numLeds*15 + entityCount*300
// ---------------------------------------------------------------------------
static unsigned long normalTargetTime(int entityCount) {
    return 3000UL + (unsigned long)(NUM_LEDS * 15)
                  + (unsigned long)(entityCount * 300);
}

// ---------------------------------------------------------------------------
// maxPossible is always basePoints * 4
// ---------------------------------------------------------------------------
void test_max_possible_is_four_times_base(void) {
    ScoreResult r = computeLevelScore(1, 1000UL, 10, 0, NUM_LEDS);
    int base = 10 * 100 * 1;
    TEST_ASSERT_EQUAL(base * 4, r.maxPossible);
}

void test_max_possible_level5_entitycount35(void) {
    // base = 35 * 100 * 5 = 17500  →  maxPossible = 70000
    ScoreResult r = computeLevelScore(5, 1000UL, 35, 0, NUM_LEDS);
    TEST_ASSERT_EQUAL(70000, r.maxPossible);
}

// ---------------------------------------------------------------------------
// Perfect time → achieved == maxPossible (base + 3*base = 4*base)
// ---------------------------------------------------------------------------
void test_perfect_time_achieved_equals_max_possible(void) {
    int entityCount = 15;
    unsigned long target = normalTargetTime(entityCount);
    ScoreResult r = computeLevelScore(1, target, entityCount, 0, NUM_LEDS);
    TEST_ASSERT_EQUAL(r.maxPossible, r.achieved);
}

void test_faster_than_target_still_full_bonus(void) {
    int entityCount = 15;
    unsigned long target = normalTargetTime(entityCount);
    // Well under target
    ScoreResult r = computeLevelScore(1, target / 2, entityCount, 0, NUM_LEDS);
    TEST_ASSERT_EQUAL(r.maxPossible, r.achieved);
}

// ---------------------------------------------------------------------------
// Specific value checks for level 1, 15 enemies
//   base = 15*100*1 = 1500
//   targetTime = 3000 + 1500 + 4500 = 9000 ms
//   maxTimeBonus = 4500
// ---------------------------------------------------------------------------
void test_level1_15enemies_exact_base(void) {
    ScoreResult r = computeLevelScore(1, 1000UL, 15, 0, NUM_LEDS);
    // achieved = 1500 + 4500 = 6000 (full bonus, well under target)
    TEST_ASSERT_EQUAL(6000, r.achieved);
}

void test_level1_15enemies_double_target_time(void) {
    // duration = 18000, target = 9000  →  ratio = 0.5
    // timeBonus = (int)(4500 * 0.5) = 2250
    // achieved  = 1500 + 2250 = 3750
    ScoreResult r = computeLevelScore(1, 18000UL, 15, 0, NUM_LEDS);
    TEST_ASSERT_EQUAL(3750, r.achieved);
}

void test_slow_completion_reduces_score_below_max(void) {
    ScoreResult fast = computeLevelScore(1, 1000UL,  15, 0, NUM_LEDS);
    ScoreResult slow = computeLevelScore(1, 99999UL, 15, 0, NUM_LEDS);
    // fast.achieved > slow.achieved
    TEST_ASSERT_GREATER_THAN(slow.achieved, fast.achieved);
}

// ---------------------------------------------------------------------------
// Level multiplier scales both base and bonus proportionally
// ---------------------------------------------------------------------------
void test_higher_level_produces_higher_max(void) {
    ScoreResult lv1 = computeLevelScore(1, 1000UL, 20, 0, NUM_LEDS);
    ScoreResult lv5 = computeLevelScore(5, 1000UL, 20, 0, NUM_LEDS);
    TEST_ASSERT_GREATER_THAN(lv1.maxPossible, lv5.maxPossible);
}

// ---------------------------------------------------------------------------
// Boss 3 — time bonus is always the full maximum regardless of duration
// ---------------------------------------------------------------------------
void test_boss3_fast_and_slow_produce_same_score(void) {
    ScoreResult fast = computeLevelScore(10, 100UL,    45, 3, NUM_LEDS);
    ScoreResult slow = computeLevelScore(10, 999999UL, 45, 3, NUM_LEDS);
    TEST_ASSERT_EQUAL(fast.achieved, slow.achieved);
}

void test_boss3_achieved_equals_base_plus_max_bonus(void) {
    int level = 10, entityCount = 45;
    int base         = entityCount * 100 * level; // 45000
    int maxTimeBonus = base * 3;                  // 135000
    ScoreResult r = computeLevelScore(level, 5000UL, entityCount, 3, NUM_LEDS);
    TEST_ASSERT_EQUAL(base + maxTimeBonus, r.achieved); // 180000
}

// ---------------------------------------------------------------------------
// Boss 2 — fixed target time of 36 000 ms
// ---------------------------------------------------------------------------
void test_boss2_under_36s_gets_full_bonus(void) {
    int base = 45 * 100 * 6; // 27000
    ScoreResult r = computeLevelScore(6, 10000UL, 45, 2, NUM_LEDS);
    TEST_ASSERT_EQUAL(base * 4, r.achieved);
}

void test_boss2_over_36s_gets_reduced_bonus(void) {
    // duration = 72000, target = 36000  →  ratio = 0.5
    int base = 45 * 100 * 6; // 27000
    int maxTimeBonus = base * 3; // 81000
    // timeBonus = (int)(81000 * 0.5) = 40500
    // achieved  = 27000 + 40500 = 67500
    ScoreResult r = computeLevelScore(6, 72000UL, 45, 2, NUM_LEDS);
    TEST_ASSERT_EQUAL(67500, r.achieved);
}

void test_boss2_faster_beats_slower(void) {
    ScoreResult fast = computeLevelScore(6, 10000UL, 45, 2, NUM_LEDS);
    ScoreResult slow = computeLevelScore(6, 72000UL, 45, 2, NUM_LEDS);
    TEST_ASSERT_GREATER_THAN(slow.achieved, fast.achieved);
}

// ---------------------------------------------------------------------------
// Boundary: zero entity count → zero score
// ---------------------------------------------------------------------------
void test_zero_entities_produces_zero_score(void) {
    ScoreResult r = computeLevelScore(1, 1000UL, 0, 0, NUM_LEDS);
    TEST_ASSERT_EQUAL(0, r.achieved);
    TEST_ASSERT_EQUAL(0, r.maxPossible);
}

// ---------------------------------------------------------------------------
// achieved never exceeds maxPossible (at best they are equal)
// ---------------------------------------------------------------------------
void test_achieved_never_exceeds_max_possible(void) {
    // Perfect time: achieved = 4*base = maxPossible
    ScoreResult r = computeLevelScore(3, 100UL, 25, 0, NUM_LEDS);
    TEST_ASSERT_LESS_OR_EQUAL(r.maxPossible, r.achieved);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_max_possible_is_four_times_base);
    RUN_TEST(test_max_possible_level5_entitycount35);
    RUN_TEST(test_perfect_time_achieved_equals_max_possible);
    RUN_TEST(test_faster_than_target_still_full_bonus);
    RUN_TEST(test_level1_15enemies_exact_base);
    RUN_TEST(test_level1_15enemies_double_target_time);
    RUN_TEST(test_slow_completion_reduces_score_below_max);
    RUN_TEST(test_higher_level_produces_higher_max);
    RUN_TEST(test_boss3_fast_and_slow_produce_same_score);
    RUN_TEST(test_boss3_achieved_equals_base_plus_max_bonus);
    RUN_TEST(test_boss2_under_36s_gets_full_bonus);
    RUN_TEST(test_boss2_over_36s_gets_reduced_bonus);
    RUN_TEST(test_boss2_faster_beats_slower);
    RUN_TEST(test_zero_entities_produces_zero_score);
    RUN_TEST(test_achieved_never_exceeds_max_possible);
    return UNITY_END();
}
