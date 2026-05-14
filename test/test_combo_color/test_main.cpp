#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// --- No buttons ---

void test_no_buttons_returns_0(void) {
    TEST_ASSERT_EQUAL(0, getComboColor(false, false, false));
}

// --- Single buttons map to primary colors ---

void test_blue_only_returns_1(void) {
    TEST_ASSERT_EQUAL(1, getComboColor(true, false, false));
}
void test_red_only_returns_2(void) {
    TEST_ASSERT_EQUAL(2, getComboColor(false, true, false));
}
void test_green_only_returns_3(void) {
    TEST_ASSERT_EQUAL(3, getComboColor(false, false, true));
}

// --- Two-button combos map to secondary colors ---

void test_red_green_returns_4(void) {
    // Yellow
    TEST_ASSERT_EQUAL(4, getComboColor(false, true, true));
}
void test_red_blue_returns_5(void) {
    // Magenta
    TEST_ASSERT_EQUAL(5, getComboColor(true, true, false));
}
void test_green_blue_returns_6(void) {
    // Cyan
    TEST_ASSERT_EQUAL(6, getComboColor(true, false, true));
}

// --- All three buttons map to white ---

void test_all_three_returns_7(void) {
    TEST_ASSERT_EQUAL(7, getComboColor(true, true, true));
}

// --- Results are in valid range ---

void test_all_combos_in_valid_range(void) {
    // Exhaustive check: 8 possible states
    TEST_ASSERT_EQUAL(0, getComboColor(false, false, false));
    TEST_ASSERT_EQUAL(1, getComboColor(true,  false, false));
    TEST_ASSERT_EQUAL(2, getComboColor(false, true,  false));
    TEST_ASSERT_EQUAL(3, getComboColor(false, false, true));
    TEST_ASSERT_EQUAL(4, getComboColor(false, true,  true));
    TEST_ASSERT_EQUAL(5, getComboColor(true,  true,  false));
    TEST_ASSERT_EQUAL(6, getComboColor(true,  false, true));
    TEST_ASSERT_EQUAL(7, getComboColor(true,  true,  true));
}

// --- Non-zero results are all unique (no two combinations produce same color) ---

void test_non_zero_results_are_unique(void) {
    int colors[7];
    colors[0] = getComboColor(true,  false, false); // 1
    colors[1] = getComboColor(false, true,  false); // 2
    colors[2] = getComboColor(false, false, true);  // 3
    colors[3] = getComboColor(false, true,  true);  // 4
    colors[4] = getComboColor(true,  true,  false); // 5
    colors[5] = getComboColor(true,  false, true);  // 6
    colors[6] = getComboColor(true,  true,  true);  // 7

    for (int i = 0; i < 7; i++) {
        for (int j = i + 1; j < 7; j++) {
            TEST_ASSERT_NOT_EQUAL(colors[i], colors[j]);
        }
    }
}

// --- Priority: three-button beats two-button beats one-button ---

void test_three_button_has_highest_priority(void) {
    // If all three are pressed, must return 7 regardless of order
    TEST_ASSERT_EQUAL(7, getComboColor(true, true, true));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_no_buttons_returns_0);
    RUN_TEST(test_blue_only_returns_1);
    RUN_TEST(test_red_only_returns_2);
    RUN_TEST(test_green_only_returns_3);
    RUN_TEST(test_red_green_returns_4);
    RUN_TEST(test_red_blue_returns_5);
    RUN_TEST(test_green_blue_returns_6);
    RUN_TEST(test_all_three_returns_7);
    RUN_TEST(test_all_combos_in_valid_range);
    RUN_TEST(test_non_zero_results_are_unique);
    RUN_TEST(test_three_button_has_highest_priority);
    return UNITY_END();
}
