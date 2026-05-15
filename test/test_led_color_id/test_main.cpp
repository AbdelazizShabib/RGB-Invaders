#include <stdio.h>
#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// getLedColorId — classification tests
// ---------------------------------------------------------------------------

void test_pure_black_is_0(void) {
    int id = getLedColorId(0, 0, 0);
    printf("  getLedColorId(0, 0, 0) = %d\n", id);
    TEST_ASSERT_EQUAL(0, id);
}
void test_near_black_under_threshold_is_0(void) {
    int id = getLedColorId(19, 19, 19);
    printf("  getLedColorId(19, 19, 19) = %d  (under threshold)\n", id);
    TEST_ASSERT_EQUAL(0, id);
}
void test_exactly_at_black_threshold_is_0(void) {
    int id = getLedColorId(10, 10, 10);
    printf("  getLedColorId(10, 10, 10) = %d  (threshold < 20)\n", id);
    TEST_ASSERT_EQUAL(0, id);
}

void test_pure_blue_is_1(void) {
    int id = getLedColorId(0, 0, 255);
    printf("  getLedColorId(0, 0, 255) = %d  [Blue]\n", id);
    TEST_ASSERT_EQUAL(1, id);
}
void test_pure_red_is_2(void) {
    int id = getLedColorId(255, 0, 0);
    printf("  getLedColorId(255, 0, 0) = %d  [Red]\n", id);
    TEST_ASSERT_EQUAL(2, id);
}
void test_pure_green_is_3(void) {
    int id = getLedColorId(0, 255, 0);
    printf("  getLedColorId(0, 255, 0) = %d  [Green]\n", id);
    TEST_ASSERT_EQUAL(3, id);
}

void test_pure_yellow_is_4(void) {
    int id = getLedColorId(255, 255, 0);
    printf("  getLedColorId(255, 255, 0) = %d  [Yellow]\n", id);
    TEST_ASSERT_EQUAL(4, id);
}
void test_pure_magenta_is_5(void) {
    int id = getLedColorId(255, 0, 255);
    printf("  getLedColorId(255, 0, 255) = %d  [Magenta]\n", id);
    TEST_ASSERT_EQUAL(5, id);
}
void test_pure_cyan_is_6(void) {
    int id = getLedColorId(0, 255, 255);
    printf("  getLedColorId(0, 255, 255) = %d  [Cyan]\n", id);
    TEST_ASSERT_EQUAL(6, id);
}

void test_pure_white_is_7(void) {
    int id = getLedColorId(255, 255, 255);
    printf("  getLedColorId(255, 255, 255) = %d  [White]\n", id);
    TEST_ASSERT_EQUAL(7, id);
}
void test_near_white_is_7(void) {
    int id = getLedColorId(200, 200, 200);
    printf("  getLedColorId(200, 200, 200) = %d  [near white]\n", id);
    TEST_ASSERT_EQUAL(7, id);
}
void test_exactly_at_white_threshold_is_7(void) {
    int id = getLedColorId(181, 181, 181);
    printf("  getLedColorId(181, 181, 181) = %d  [at white threshold]\n", id);
    TEST_ASSERT_EQUAL(7, id);
}

// ---------------------------------------------------------------------------
// getColorComponents — canonical value tests
// ---------------------------------------------------------------------------

void test_code0_gives_black(void) {
    uint8_t r, g, b;
    getColorComponents(0, r, g, b);
    printf("  getColorComponents(0) = (R=%u, G=%u, B=%u)  [Black]\n", r, g, b);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
}
void test_code8_gives_black(void) {
    uint8_t r, g, b;
    getColorComponents(8, r, g, b);
    printf("  getColorComponents(8) = (R=%u, G=%u, B=%u)  [out-of-range -> Black]\n", r, g, b);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
}
void test_code1_gives_blue(void) {
    uint8_t r, g, b;
    getColorComponents(1, r, g, b);
    printf("  getColorComponents(1) = (R=%u, G=%u, B=%u)  [Blue]\n", r, g, b);
    TEST_ASSERT_EQUAL(  0, r);
    TEST_ASSERT_EQUAL(  0, g);
    TEST_ASSERT_EQUAL(255, b);
}
void test_code7_gives_white(void) {
    uint8_t r, g, b;
    getColorComponents(7, r, g, b);
    printf("  getColorComponents(7) = (R=%u, G=%u, B=%u)  [White]\n", r, g, b);
    TEST_ASSERT_EQUAL(255, r);
    TEST_ASSERT_EQUAL(255, g);
    TEST_ASSERT_EQUAL(255, b);
}

// ---------------------------------------------------------------------------
// Round-trip: getColorComponents then getLedColorId must return original code
// ---------------------------------------------------------------------------

void test_roundtrip_code1(void) {
    uint8_t r, g, b;
    getColorComponents(1, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 1: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(1, id);
}
void test_roundtrip_code2(void) {
    uint8_t r, g, b;
    getColorComponents(2, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 2: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(2, id);
}
void test_roundtrip_code3(void) {
    uint8_t r, g, b;
    getColorComponents(3, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 3: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(3, id);
}
void test_roundtrip_code4(void) {
    uint8_t r, g, b;
    getColorComponents(4, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 4: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(4, id);
}
void test_roundtrip_code5(void) {
    uint8_t r, g, b;
    getColorComponents(5, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 5: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(5, id);
}
void test_roundtrip_code6(void) {
    uint8_t r, g, b;
    getColorComponents(6, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 6: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(6, id);
}
void test_roundtrip_code7(void) {
    uint8_t r, g, b;
    getColorComponents(7, r, g, b);
    int id = getLedColorId(r, g, b);
    printf("  round-trip code 7: (R=%u,G=%u,B=%u) -> id=%d\n", r, g, b, id);
    TEST_ASSERT_EQUAL(7, id);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_pure_black_is_0);
    RUN_TEST(test_near_black_under_threshold_is_0);
    RUN_TEST(test_exactly_at_black_threshold_is_0);
    RUN_TEST(test_pure_blue_is_1);
    RUN_TEST(test_pure_red_is_2);
    RUN_TEST(test_pure_green_is_3);
    RUN_TEST(test_pure_yellow_is_4);
    RUN_TEST(test_pure_magenta_is_5);
    RUN_TEST(test_pure_cyan_is_6);
    RUN_TEST(test_pure_white_is_7);
    RUN_TEST(test_near_white_is_7);
    RUN_TEST(test_exactly_at_white_threshold_is_7);
    RUN_TEST(test_code0_gives_black);
    RUN_TEST(test_code8_gives_black);
    RUN_TEST(test_code1_gives_blue);
    RUN_TEST(test_code7_gives_white);
    RUN_TEST(test_roundtrip_code1);
    RUN_TEST(test_roundtrip_code2);
    RUN_TEST(test_roundtrip_code3);
    RUN_TEST(test_roundtrip_code4);
    RUN_TEST(test_roundtrip_code5);
    RUN_TEST(test_roundtrip_code6);
    RUN_TEST(test_roundtrip_code7);
    return UNITY_END();
}
