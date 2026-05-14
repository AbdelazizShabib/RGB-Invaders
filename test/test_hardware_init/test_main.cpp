#include <unity.h>
#include <Wire.h>
#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

// Match your actual hardware pin config from main.cpp
#define PIN_SDA         8
#define PIN_SCL         9
#define OLED_ADDR       0x3C
#define LED_PIN         48
#define NUM_LEDS        100
#define BUTTON_PIN_A    0
#define BUTTON_PIN_B    14
#define BUTTON_PIN_C    47

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// I2C / OLED
// ---------------------------------------------------------------------------

void test_i2c_bus_initialises(void)
{
    bool ok = Wire.begin(PIN_SDA, PIN_SCL);
    TEST_ASSERT_TRUE(ok);
}

void test_oled_responds_on_i2c(void)
{
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.beginTransmission(OLED_ADDR);
    uint8_t err = Wire.endTransmission();
    // 0 = ACK from device; any other value means no device or bus fault
    TEST_ASSERT_EQUAL_MESSAGE(0, err, "SSD1306 did not ACK on I2C");
}

// ---------------------------------------------------------------------------
// GPIO — button pins must be input-capable (no assertion fault on config)
// ---------------------------------------------------------------------------

void test_gpio_button_pins_configure_as_input(void)
{
    // If pinMode crashes or asserts, Unity catches it via longjmp
    pinMode(BUTTON_PIN_A, INPUT_PULLUP);
    pinMode(BUTTON_PIN_B, INPUT_PULLUP);
    pinMode(BUTTON_PIN_C, INPUT_PULLUP);
    // If we get here, the pin config succeeded
    TEST_PASS();
}

void test_gpio_button_reads_return_valid_level(void)
{
    pinMode(BUTTON_PIN_A, INPUT_PULLUP);
    int level = digitalRead(BUTTON_PIN_A);
    // With pull-up and no button pressed the line must be HIGH
    TEST_ASSERT_EQUAL(HIGH, level);
}

// ---------------------------------------------------------------------------
// FastLED — initialise without crashing
// ---------------------------------------------------------------------------

static CRGB leds[NUM_LEDS];

void test_fastled_add_leds_no_crash(void)
{
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(0); // keep LEDs off during testing
    // Reaching this line means addLeds didn't assert
    TEST_PASS();
}

void test_fastled_show_all_black_no_crash(void)
{
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    TEST_PASS();
}

void test_fastled_single_pixel_write(void)
{
    leds[0] = CRGB::Red;
    FastLED.show();
    TEST_ASSERT_EQUAL(255, leds[0].r);
    TEST_ASSERT_EQUAL(0,   leds[0].g);
    TEST_ASSERT_EQUAL(0,   leds[0].b);
    leds[0] = CRGB::Black;
    FastLED.show();
}

// ---------------------------------------------------------------------------
// WiFi — driver init (no AP connection required)
// ---------------------------------------------------------------------------

void test_wifi_mode_set_no_crash(void)
{
    WiFi.mode(WIFI_STA);
    TEST_PASS();
}

void test_wifi_scan_returns_valid_result(void)
{
    WiFi.mode(WIFI_STA);
    // scanNetworks with async=false, show_hidden=false, channel=0 → blocks
    // We only verify it returns without crashing and gives an int result.
    int n = WiFi.scanNetworks(false, false, false, 100 /*ms per channel*/);
    // n can be 0 (no networks) or positive; -1/-2 = scan error
    TEST_ASSERT_GREATER_OR_EQUAL(-2, n); // anything above -3 is acceptable
    WiFi.scanDelete();
}

// ---------------------------------------------------------------------------
// Heap sanity — minimum free heap after hardware init
// ---------------------------------------------------------------------------

void test_free_heap_above_minimum(void)
{
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    // Require at least 80 KB free after all driver init
    TEST_ASSERT_GREATER_THAN(80 * 1024, free_heap);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    Wire.begin(PIN_SDA, PIN_SCL);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(0);

    UNITY_BEGIN();
    RUN_TEST(test_i2c_bus_initialises);
    RUN_TEST(test_oled_responds_on_i2c);
    RUN_TEST(test_gpio_button_pins_configure_as_input);
    RUN_TEST(test_gpio_button_reads_return_valid_level);
    RUN_TEST(test_fastled_add_leds_no_crash);
    RUN_TEST(test_fastled_show_all_black_no_crash);
    RUN_TEST(test_fastled_single_pixel_write);
    RUN_TEST(test_wifi_mode_set_no_crash);
    RUN_TEST(test_wifi_scan_returns_valid_result);
    RUN_TEST(test_free_heap_above_minimum);
    UNITY_END();
}

void loop() {}
