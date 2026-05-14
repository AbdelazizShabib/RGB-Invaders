#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// getLedColorId — classification tests
// ---------------------------------------------------------------------------

void test_pure_black_is_0(void) {
    TEST_ASSERT_EQUAL(0, getLedColorId(0, 0, 0));
}
void test_near_black_under_threshold_is_0(void) {
    TEST_ASSERT_EQUAL(0, getLedColorId(19, 19, 19));
}
void test_exactly_at_black_threshold_is_0(void) {
    // threshold is < 20 on all channels
    TEST_ASSERT_EQUAL(0, getLedColorId(10, 10, 10));
}

void test_pure_blue_is_1(void) {
    TEST_ASSERT_EQUAL(1, getLedColorId(0, 0, 255));
}
void test_pure_red_is_2(void) {
    TEST_ASSERT_EQUAL(2, getLedColorId(255, 0, 0));
}
void test_pure_green_is_3(void) {
    TEST_ASSERT_EQUAL(3, getLedColorId(0, 255, 0));
}

void test_pure_yellow_is_4(void) {
    // r>120, g>120, b<100
    TEST_ASSERT_EQUAL(4, getLedColorId(255, 255, 0));
}
void test_pure_magenta_is_5(void) {
    // r>120, b>120, g<100
    TEST_ASSERT_EQUAL(5, getLedColorId(255, 0, 255));
}
void test_pure_cyan_is_6(void) {
    // g>120, b>120, r<100
    TEST_ASSERT_EQUAL(6, getLedColorId(0, 255, 255));
}

void test_pure_white_is_7(void) {
    // all > 180
    TEST_ASSERT_EQUAL(7, getLedColorId(255, 255, 255));
}
void test_near_white_is_7(void) {
    TEST_ASSERT_EQUAL(7, getLedColorId(200, 200, 200));
}
void test_exactly_at_white_threshold_is_7(void) {
    TEST_ASSERT_EQUAL(7, getLedColorId(181, 181, 181));
}

// ---------------------------------------------------------------------------
// getColorComponents — canonical value tests
// ---------------------------------------------------------------------------

void test_code0_gives_black(void) {
    uint8_t r, g, b;
    getColorComponents(0, r, g, b);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
}
void test_code8_gives_black(void) {
    uint8_t r, g, b;
    getColorComponents(8, r, g, b);
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_EQUAL(0, g);
    TEST_ASSERT_EQUAL(0, b);
}
void test_code1_gives_blue(void) {
    uint8_t r, g, b;
    getColorComponents(1, r, g, b);
    TEST_ASSERT_EQUAL(  0, r);
    TEST_ASSERT_EQUAL(  0, g);
    TEST_ASSERT_EQUAL(255, b);
}
void test_code7_gives_white(void) {
    uint8_t r, g, b;
    getColorComponents(7, r, g, b);
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
    TEST_ASSERT_EQUAL(1, getLedColorId(r, g, b));
}
void test_roundtrip_code2(void) {
    uint8_t r, g, b;
    getColorComponents(2, r, g, b);
    TEST_ASSERT_EQUAL(2, getLedColorId(r, g, b));
}
void test_roundtrip_code3(void) {
    uint8_t r, g, b;
    getColorComponents(3, r, g, b);
    TEST_ASSERT_EQUAL(3, getLedColorId(r, g, b));
}
void test_roundtrip_code4(void) {
    uint8_t r, g, b;
    getColorComponents(4, r, g, b);
    TEST_ASSERT_EQUAL(4, getLedColorId(r, g, b));
}
void test_roundtrip_code5(void) {
    uint8_t r, g, b;
    getColorComponents(5, r, g, b);
    TEST_ASSERT_EQUAL(5, getLedColorId(r, g, b));
}
void test_roundtrip_code6(void) {
    uint8_t r, g, b;
    getColorComponents(6, r, g, b);
    TEST_ASSERT_EQUAL(6, getLedColorId(r, g, b));
}
void test_roundtrip_code7(void) {
    uint8_t r, g, b;
    getColorComponents(7, r, g, b);
    TEST_ASSERT_EQUAL(7, getLedColorId(r, g, b));
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
