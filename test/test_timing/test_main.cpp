#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <esp_timer.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// esp_timer resolution sanity
// ---------------------------------------------------------------------------

void test_esp_timer_advances(void)
{
    int64_t t0 = esp_timer_get_time();
    vTaskDelay(pdMS_TO_TICKS(10));
    int64_t t1 = esp_timer_get_time();
    TEST_ASSERT_GREATER_THAN(0, t1 - t0);
}

void test_esp_timer_10ms_delay_accuracy(void)
{
    int64_t t0 = esp_timer_get_time();
    vTaskDelay(pdMS_TO_TICKS(10));
    int64_t elapsed_us = esp_timer_get_time() - t0;
    // 10 ms = 10000 µs; allow +5 ms jitter on either side
    TEST_ASSERT_GREATER_OR_EQUAL(8000,  elapsed_us);
    TEST_ASSERT_LESS_OR_EQUAL   (15000, elapsed_us);
}

void test_esp_timer_50ms_delay_accuracy(void)
{
    int64_t t0 = esp_timer_get_time();
    vTaskDelay(pdMS_TO_TICKS(50));
    int64_t elapsed_us = esp_timer_get_time() - t0;
    TEST_ASSERT_GREATER_OR_EQUAL(45000, elapsed_us);
    TEST_ASSERT_LESS_OR_EQUAL   (60000, elapsed_us);
}

// ---------------------------------------------------------------------------
// Semaphore wake latency
// Give a binary semaphore from a dedicated task and measure how quickly
// the waiting task wakes up.
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_latency_sem;
static volatile int64_t  s_give_time_us;
static volatile int64_t  s_take_time_us;

static void latency_giver(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(20)); // brief warmup
    s_give_time_us = esp_timer_get_time();
    xSemaphoreGive(s_latency_sem);
    vTaskDelete(NULL);
}

void test_semaphore_wake_latency_under_1ms(void)
{
    s_latency_sem  = xSemaphoreCreateBinary();
    s_give_time_us = 0;
    s_take_time_us = 0;

    xTaskCreatePinnedToCore(latency_giver, "giver", 2048, NULL, 6, NULL, 1);

    // Block on core 0; giver fires from core 1
    xSemaphoreTake(s_latency_sem, portMAX_DELAY);
    s_take_time_us = esp_timer_get_time();

    int64_t latency_us = s_take_time_us - s_give_time_us;
    // Cross-core wake should complete under 1 ms (1000 µs) on ESP32-S3
    TEST_ASSERT_GREATER_OR_EQUAL(0,    latency_us);
    TEST_ASSERT_LESS_OR_EQUAL   (1000, latency_us);

    vSemaphoreDelete(s_latency_sem);
}

// ---------------------------------------------------------------------------
// Queue delivery latency
// Measure time from xQueueSend (producer task) to xQueueReceive (main task).
// ---------------------------------------------------------------------------

static QueueHandle_t     s_latency_q;
static volatile int64_t  s_send_time_us;

static void queue_producer(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(20));
    s_send_time_us = esp_timer_get_time();
    uint32_t val   = 0xBEEFu;
    xQueueSend(s_latency_q, &val, 0);
    vTaskDelete(NULL);
}

void test_queue_delivery_latency_under_1ms(void)
{
    s_latency_q    = xQueueCreate(2, sizeof(uint32_t));
    s_send_time_us = 0;

    xTaskCreatePinnedToCore(queue_producer, "qprod", 2048, NULL, 6, NULL, 1);

    uint32_t recv;
    xQueueReceive(s_latency_q, &recv, portMAX_DELAY);
    int64_t recv_time_us = esp_timer_get_time();

    int64_t latency_us = recv_time_us - s_send_time_us;
    TEST_ASSERT_EQUAL_UINT32(0xBEEFu, recv);
    TEST_ASSERT_GREATER_OR_EQUAL(0,    latency_us);
    TEST_ASSERT_LESS_OR_EQUAL   (1000, latency_us);

    vQueueDelete(s_latency_q);
}

// ---------------------------------------------------------------------------
// vTaskDelayUntil period drift — run 10 iterations, check cumulative drift
// ---------------------------------------------------------------------------

void test_task_delay_until_low_drift(void)
{
    const TickType_t PERIOD  = pdMS_TO_TICKS(16); // ~60 FPS target
    const int        ITERS   = 20;
    int64_t expected_us = 16000LL * ITERS;

    TickType_t last_wake = xTaskGetTickCount();
    int64_t    t0        = esp_timer_get_time();
    for (int i = 0; i < ITERS; i++) {
        vTaskDelayUntil(&last_wake, PERIOD);
    }
    int64_t elapsed_us = esp_timer_get_time() - t0;

    // Tolerate ±10 % drift over 20 iterations
    int64_t tolerance = expected_us / 10;
    TEST_ASSERT_INT_WITHIN(tolerance, expected_us, elapsed_us);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_esp_timer_advances);
    RUN_TEST(test_esp_timer_10ms_delay_accuracy);
    RUN_TEST(test_esp_timer_50ms_delay_accuracy);
    RUN_TEST(test_semaphore_wake_latency_under_1ms);
    RUN_TEST(test_queue_delivery_latency_under_1ms);
    RUN_TEST(test_task_delay_until_low_drift);
    UNITY_END();
}

void loop() {}
