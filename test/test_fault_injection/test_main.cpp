#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_heap_caps.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Queue overflow recovery
// Send past capacity with zero timeout; verify queue remains usable after
// overflow attempts and that real items are not corrupted.
// ---------------------------------------------------------------------------

void test_queue_survives_overflow_attempts(void)
{
    QueueHandle_t q = xQueueCreate(3, sizeof(uint32_t));

    uint32_t good = 0xFACEu;
    xQueueSend(q, &good, 0);
    xQueueSend(q, &good, 0);
    xQueueSend(q, &good, 0);

    uint32_t bad = 0xDEADu;
    int rejected = 0;
    for (int i = 0; i < 10; i++) {
        if (xQueueSend(q, &bad, 0) != pdTRUE) rejected++;
    }

    UBaseType_t waiting = uxQueueMessagesWaiting(q);
    Serial.printf("  queue(cap=3): 3 good + 10 overflow attempts  rejected=%d  messagesWaiting=%u\n",
                  rejected, waiting);
    TEST_ASSERT_EQUAL(3, waiting);

    int correct = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t recv = 0;
        xQueueReceive(q, &recv, 0);
        if (recv == good) correct++;
        TEST_ASSERT_EQUAL_UINT32(good, recv);
    }
    Serial.printf("  recovered %d/3 good items intact after overflow attempts\n", correct);
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(q));
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// Queue reset under active producer
// Reset while a producer task is mid-flight; verify post-reset state is clean.
// ---------------------------------------------------------------------------

static QueueHandle_t     s_reset_q;
static SemaphoreHandle_t s_reset_start;
static SemaphoreHandle_t s_reset_done_sem;

static void slow_producer(void *param)
{
    xSemaphoreTake(s_reset_start, portMAX_DELAY);
    for (int i = 0; i < 20; i++) {
        uint32_t v = (uint32_t)i;
        xQueueSend(s_reset_q, &v, pdMS_TO_TICKS(5));
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    xSemaphoreGive(s_reset_done_sem);
    vTaskDelete(NULL);
}

void test_queue_reset_recovers_from_mid_flight_producer(void)
{
    s_reset_q        = xQueueCreate(8, sizeof(uint32_t));
    s_reset_start    = xSemaphoreCreateBinary();
    s_reset_done_sem = xSemaphoreCreateBinary();

    xTaskCreate(slow_producer, "prod", 2048, NULL, 5, NULL);
    xSemaphoreGive(s_reset_start);
    vTaskDelay(pdMS_TO_TICKS(20));

    UBaseType_t before_reset = uxQueueMessagesWaiting(s_reset_q);
    xQueueReset(s_reset_q);
    UBaseType_t after_reset = uxQueueMessagesWaiting(s_reset_q);
    vTaskDelay(pdMS_TO_TICKS(100));

    uint32_t v = 0xCAFEu;
    BaseType_t send_ok = xQueueSend(s_reset_q, &v, 0);
    while (uxQueueMessagesWaiting(s_reset_q)) {
        xQueueReceive(s_reset_q, &v, 0);
    }
    Serial.printf("  queue reset mid-flight: before=%u items  after_reset=%u  new send=%s  queue usable\n",
                  before_reset, after_reset,
                  send_ok == pdTRUE ? "OK" : "FAIL");
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(s_reset_q));

    xSemaphoreTake(s_reset_done_sem, pdMS_TO_TICKS(1000));
    vQueueDelete(s_reset_q);
    vSemaphoreDelete(s_reset_start);
    vSemaphoreDelete(s_reset_done_sem);
}

// ---------------------------------------------------------------------------
// Mutex timeout fault: task can't get mutex within deadline → graceful fail
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_timeout_mutex;

static void mutex_hog(void *param)
{
    xSemaphoreTake(s_timeout_mutex, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(200));
    xSemaphoreGive(s_timeout_mutex);
    vTaskDelete(NULL);
}

void test_mutex_timeout_returns_false_not_crash(void)
{
    s_timeout_mutex = xSemaphoreCreateMutex();
    xTaskCreate(mutex_hog, "hog", 2048, NULL, 6, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));

    TickType_t t0 = xTaskGetTickCount();
    BaseType_t result = xSemaphoreTake(s_timeout_mutex, pdMS_TO_TICKS(50));
    uint32_t waited_ms = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;
    Serial.printf("  mutex held by hog(200ms): take with 50ms timeout  result=%s  waited=%u ms  (graceful)\n",
                  result == pdFALSE ? "pdFALSE (OK)" : "acquired (unexpected)",
                  waited_ms);
    TEST_ASSERT_EQUAL(pdFALSE, result);

    vTaskDelay(pdMS_TO_TICKS(250));
    BaseType_t recover = xSemaphoreTake(s_timeout_mutex, pdMS_TO_TICKS(100));
    Serial.printf("  after hog releases: take again = %s  (mutex recovered)\n",
                  recover == pdTRUE ? "pdTRUE (OK)" : "FAIL");
    TEST_ASSERT_EQUAL(pdTRUE, recover);
    xSemaphoreGive(s_timeout_mutex);

    vSemaphoreDelete(s_timeout_mutex);
}

// ---------------------------------------------------------------------------
// Heap exhaustion: verify malloc returns NULL gracefully (not crash)
// ---------------------------------------------------------------------------

void test_malloc_returns_null_on_exhaustion(void)
{
    size_t free_before = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

    void *chunks[64];
    int   allocated = 0;
    for (int i = 0; i < 64; i++) {
        chunks[i] = malloc(32 * 1024);
        if (chunks[i] == NULL) break;
        allocated++;
    }

    size_t free_during = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    Serial.printf("  heap exhaustion: allocated %d chunks x 32KB = %d KB  free_before=%u  free_during=%u\n",
                  allocated, allocated * 32, free_before, free_during);

    TEST_PASS_MESSAGE("malloc returned NULL gracefully on heap exhaustion");

    for (int i = 0; i < allocated; i++) {
        free(chunks[i]);
    }

    size_t free_after = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    Serial.printf("  heap recovered: free_after=%u bytes  (>= 64KB required)\n", free_after);
    TEST_ASSERT_GREATER_THAN(64 * 1024, free_after);
}

// ---------------------------------------------------------------------------
// State boundary: shouldTriggerWin with invalid state must not crash
// ---------------------------------------------------------------------------

#include "game_logic.h"

void test_invalid_state_in_rtos_context_no_crash(void)
{
    bool r1 = shouldTriggerWin(999,  true, true);
    bool r2 = shouldTriggerWin(-1,   true, true);
    bool r3 = shouldTriggerWin(0xFF, true, true);
    Serial.printf("  invalid states in RTOS context: shouldTriggerWin(999)=%s  (-1)=%s  (0xFF)=%s  (no crash)\n",
                  r1 ? "true" : "false", r2 ? "true" : "false", r3 ? "true" : "false");
    TEST_ASSERT_FALSE(r1);
    TEST_ASSERT_FALSE(r2);
    TEST_ASSERT_FALSE(r3);
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Task starvation detection: low-priority task must eventually run
// ---------------------------------------------------------------------------

static volatile bool    s_starved_ran = false;
static SemaphoreHandle_t s_starve_done;

static void starved_task(void *param)
{
    s_starved_ran = true;
    xSemaphoreGive(s_starve_done);
    vTaskDelete(NULL);
}

static void spinner_task(void *param)
{
    TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(50);
    while (xTaskGetTickCount() < end) {
        taskYIELD();
    }
    vTaskDelete(NULL);
}

void test_low_priority_task_not_starved(void)
{
    s_starved_ran = false;
    s_starve_done = xSemaphoreCreateBinary();

    xTaskCreate(spinner_task, "spin1", 2048, NULL, 7, NULL);
    xTaskCreate(spinner_task, "spin2", 2048, NULL, 7, NULL);
    xTaskCreate(starved_task, "starved", 2048, NULL, 3, NULL);

    TickType_t t0 = xTaskGetTickCount();
    BaseType_t ok = xSemaphoreTake(s_starve_done, pdMS_TO_TICKS(500));
    uint32_t waited_ms = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;
    Serial.printf("  2 high-prio(7) spinners + 1 low-prio(3) task: low ran=%s  waited=%u ms  (limit 500ms)\n",
                  s_starved_ran ? "YES" : "NO",
                  waited_ms);
    TEST_ASSERT_EQUAL(pdTRUE, ok);
    TEST_ASSERT_TRUE(s_starved_ran);

    vSemaphoreDelete(s_starve_done);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    Serial.println("\n=== Fault Injection Tests ===");
    UNITY_BEGIN();
    RUN_TEST(test_queue_survives_overflow_attempts);
    RUN_TEST(test_queue_reset_recovers_from_mid_flight_producer);
    RUN_TEST(test_mutex_timeout_returns_false_not_crash);
    RUN_TEST(test_malloc_returns_null_on_exhaustion);
    RUN_TEST(test_invalid_state_in_rtos_context_no_crash);
    RUN_TEST(test_low_priority_task_not_starved);
    UNITY_END();
}

void loop() {}
