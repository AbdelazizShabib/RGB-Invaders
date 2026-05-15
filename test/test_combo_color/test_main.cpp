#include <stdio.h>
#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

static const char* colorName(int id) {
    switch(id) {
        case 0: return "None";
        case 1: return "Blue";
        case 2: return "Red";
        case 3: return "Green";
        case 4: return "Yellow";
        case 5: return "Magenta";
        case 6: return "Cyan";
        case 7: return "White";
        default: return "Unknown";
    }
}

// --- No buttons ---

void test_no_buttons_returns_0(void) {
    int result = getComboColor(false, false, false);
    printf("  getComboColor(B=0,R=0,G=0) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(0, result);
}

// --- Single buttons map to primary colors ---

void test_blue_only_returns_1(void) {
    int result = getComboColor(true, false, false);
    printf("  getComboColor(B=1,R=0,G=0) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(1, result);
}
void test_red_only_returns_2(void) {
    int result = getComboColor(false, true, false);
    printf("  getComboColor(B=0,R=1,G=0) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(2, result);
}
void test_green_only_returns_3(void) {
    int result = getComboColor(false, false, true);
    printf("  getComboColor(B=0,R=0,G=1) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(3, result);
}

// --- Two-button combos map to secondary colors ---

void test_red_green_returns_4(void) {
    int result = getComboColor(false, true, true);
    printf("  getComboColor(B=0,R=1,G=1) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(4, result);
}
void test_red_blue_returns_5(void) {
    int result = getComboColor(true, true, false);
    printf("  getComboColor(B=1,R=1,G=0) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(5, result);
}
void test_green_blue_returns_6(void) {
    int result = getComboColor(true, false, true);
    printf("  getComboColor(B=1,R=0,G=1) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(6, result);
}

// --- All three buttons map to white ---

void test_all_three_returns_7(void) {
    int result = getComboColor(true, true, true);
    printf("  getComboColor(B=1,R=1,G=1) = %d (%s)\n", result, colorName(result));
    TEST_ASSERT_EQUAL(7, result);
}

// --- Results are in valid range ---

void test_all_combos_in_valid_range(void) {
    int ids[8];
    ids[0] = getComboColor(false, false, false);
    ids[1] = getComboColor(true,  false, false);
    ids[2] = getComboColor(false, true,  false);
    ids[3] = getComboColor(false, false, true);
    ids[4] = getComboColor(false, true,  true);
    ids[5] = getComboColor(true,  true,  false);
    ids[6] = getComboColor(true,  false, true);
    ids[7] = getComboColor(true,  true,  true);
    printf("  All 8 combos: [%d, %d, %d, %d, %d, %d, %d, %d] => [%s, %s, %s, %s, %s, %s, %s, %s]\n",
           ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], ids[6], ids[7],
           colorName(ids[0]), colorName(ids[1]), colorName(ids[2]), colorName(ids[3]),
           colorName(ids[4]), colorName(ids[5]), colorName(ids[6]), colorName(ids[7]));
    for (int i = 0; i < 8; i++) TEST_ASSERT_EQUAL(i, ids[i]);
}

// --- Non-zero results are all unique (no two combinations produce same color) ---

void test_non_zero_results_are_unique(void) {
    int colors[7];
    colors[0] = getComboColor(true,  false, false);
    colors[1] = getComboColor(false, true,  false);
    colors[2] = getComboColor(false, false, true);
    colors[3] = getComboColor(false, true,  true);
    colors[4] = getComboColor(true,  true,  false);
    colors[5] = getComboColor(true,  false, true);
    colors[6] = getComboColor(true,  true,  true);
    printf("  7 non-zero color IDs: [%d, %d, %d, %d, %d, %d, %d] — all unique\n",
           colors[0], colors[1], colors[2], colors[3], colors[4], colors[5], colors[6]);
    for (int i = 0; i < 7; i++) {
        for (int j = i + 1; j < 7; j++) {
            TEST_ASSERT_NOT_EQUAL(colors[i], colors[j]);
        }
    }
}

// --- Priority: three-button beats two-button beats one-button ---

void test_three_button_has_highest_priority(void) {
    int result = getComboColor(true, true, true);
    printf("  getComboColor(B=1,R=1,G=1) = %d (%s) — highest priority combo\n", result, colorName(result));
    TEST_ASSERT_EQUAL(7, result);
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
