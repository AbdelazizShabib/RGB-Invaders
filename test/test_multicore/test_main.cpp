#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_timer.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int current_core() { return (int)xPortGetCoreID(); }

// ---------------------------------------------------------------------------
// Baseline: xPortGetCoreID() returns 0 or 1 only
// ---------------------------------------------------------------------------

void test_core_id_is_valid(void)
{
    int id = current_core();
    Serial.printf("  main task running on core %d  (valid: 0 or 1)\n", id);
    TEST_ASSERT(id == 0 || id == 1);
}

// ---------------------------------------------------------------------------
// Cross-core queue: core-1 producer → core-0 consumer (main task)
// ---------------------------------------------------------------------------

static QueueHandle_t s_cc_q;

static void core1_producer(void *param)
{
    TEST_ASSERT_EQUAL(1, xPortGetCoreID());
    for (int i = 0; i < 10; i++) {
        xQueueSend(s_cc_q, &i, portMAX_DELAY);
    }
    vTaskDelete(NULL);
}

void test_cross_core_queue_core1_to_core0(void)
{
    s_cc_q = xQueueCreate(16, sizeof(int));
    xTaskCreatePinnedToCore(core1_producer, "cc_prod", 2048, NULL, 5, NULL, 1);

    int received = 0;
    bool ordered = true;
    for (int expected = 0; expected < 10; expected++) {
        int recv = -1;
        BaseType_t ok = xQueueReceive(s_cc_q, &recv, pdMS_TO_TICKS(500));
        if (ok != pdTRUE || recv != expected) ordered = false;
        else received++;
    }
    Serial.printf("  core1->core0: received=%d/10 items  ordered=%s\n",
                  received, ordered ? "YES" : "NO");
    vQueueDelete(s_cc_q);

    // Re-run for assertion
    s_cc_q = xQueueCreate(16, sizeof(int));
    xTaskCreatePinnedToCore(core1_producer, "cc_prod2", 2048, NULL, 5, NULL, 1);
    for (int expected = 0; expected < 10; expected++) {
        int recv = -1;
        BaseType_t ok = xQueueReceive(s_cc_q, &recv, pdMS_TO_TICKS(500));
        TEST_ASSERT_EQUAL(pdTRUE, ok);
        TEST_ASSERT_EQUAL(expected, recv);
    }
    vQueueDelete(s_cc_q);
}

// ---------------------------------------------------------------------------
// Cross-core queue: core-0 producer → core-1 consumer
// ---------------------------------------------------------------------------

static QueueHandle_t    s_cc_q2;
static SemaphoreHandle_t s_cc_done;
static volatile bool     s_cc_order_ok = false;

static void core1_consumer(void *param)
{
    TEST_ASSERT_EQUAL(1, xPortGetCoreID());
    bool ok = true;
    for (int expected = 0; expected < 10; expected++) {
        int recv = -1;
        if (xQueueReceive(s_cc_q2, &recv, pdMS_TO_TICKS(500)) != pdTRUE) { ok = false; break; }
        if (recv != expected) { ok = false; break; }
    }
    s_cc_order_ok = ok;
    xSemaphoreGive(s_cc_done);
    vTaskDelete(NULL);
}

void test_cross_core_queue_core0_to_core1(void)
{
    s_cc_q2      = xQueueCreate(16, sizeof(int));
    s_cc_done    = xSemaphoreCreateBinary();
    s_cc_order_ok = false;

    xTaskCreatePinnedToCore(core1_consumer, "cc_cons", 2048, NULL, 5, NULL, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    for (int i = 0; i < 10; i++) {
        xQueueSend(s_cc_q2, &i, portMAX_DELAY);
    }

    xSemaphoreTake(s_cc_done, pdMS_TO_TICKS(2000));
    Serial.printf("  core0->core1: 10 items delivered  ordered=%s\n",
                  s_cc_order_ok ? "YES" : "NO");
    TEST_ASSERT_TRUE(s_cc_order_ok);

    vQueueDelete(s_cc_q2);
    vSemaphoreDelete(s_cc_done);
}

// ---------------------------------------------------------------------------
// Cross-core semaphore: core-1 gives, core-0 (main) takes
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_cc_sem;
static volatile int64_t  s_give_us;

static void core1_giver(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(20));
    s_give_us = esp_timer_get_time();
    xSemaphoreGive(s_cc_sem);
    vTaskDelete(NULL);
}

void test_cross_core_semaphore_wake(void)
{
    s_cc_sem  = xSemaphoreCreateBinary();
    s_give_us = 0;

    xTaskCreatePinnedToCore(core1_giver, "cc_give", 2048, NULL, 6, NULL, 1);
    xSemaphoreTake(s_cc_sem, portMAX_DELAY);
    int64_t take_us  = esp_timer_get_time();
    int64_t latency  = take_us - s_give_us;

    Serial.printf("  cross-core semaphore wake latency = %lld µs  (limit <1000µs)\n", latency);
    TEST_ASSERT_GREATER_OR_EQUAL(0,    latency);
    TEST_ASSERT_LESS_OR_EQUAL   (1000, latency);

    vSemaphoreDelete(s_cc_sem);
}

// ---------------------------------------------------------------------------
// Cross-core mutex: both cores contend; final counter must be consistent
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_cc_mutex;
static volatile int      s_cc_counter = 0;
static SemaphoreHandle_t s_cc_worker_done;

static void core1_incrementer(void *param)
{
    for (int i = 0; i < 500; i++) {
        xSemaphoreTake(s_cc_mutex, portMAX_DELAY);
        s_cc_counter++;
        xSemaphoreGive(s_cc_mutex);
    }
    xSemaphoreGive(s_cc_worker_done);
    vTaskDelete(NULL);
}

void test_cross_core_mutex_counter_consistency(void)
{
    s_cc_mutex       = xSemaphoreCreateMutex();
    s_cc_worker_done = xSemaphoreCreateBinary();
    s_cc_counter     = 0;

    xTaskCreatePinnedToCore(core1_incrementer, "cc_incr", 2048, NULL, 5, NULL, 1);

    for (int i = 0; i < 500; i++) {
        xSemaphoreTake(s_cc_mutex, portMAX_DELAY);
        s_cc_counter++;
        xSemaphoreGive(s_cc_mutex);
    }

    xSemaphoreTake(s_cc_worker_done, pdMS_TO_TICKS(5000));
    Serial.printf("  core0+core1 each do 500 mutex-protected increments  ->  counter=%d  (expected 1000)\n",
                  s_cc_counter);
    TEST_ASSERT_EQUAL(1000, s_cc_counter);

    vSemaphoreDelete(s_cc_mutex);
    vSemaphoreDelete(s_cc_worker_done);
}

// ---------------------------------------------------------------------------
// Core affinity is not lost under load
// ---------------------------------------------------------------------------

static volatile int  s_affinity_violations = 0;
static SemaphoreHandle_t s_affinity_done;

static void affinity_verify_task(void *param)
{
    int expected_core = (int)(uintptr_t)param;
    for (int i = 0; i < 200; i++) {
        if ((int)xPortGetCoreID() != expected_core) {
            s_affinity_violations++;
        }
        taskYIELD();
    }
    xSemaphoreGive(s_affinity_done);
    vTaskDelete(NULL);
}

void test_pinned_task_stays_on_assigned_core(void)
{
    s_affinity_violations = 0;
    s_affinity_done       = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(affinity_verify_task, "aff1", 2048, (void *)1, 5, NULL, 1);
    xSemaphoreTake(s_affinity_done, pdMS_TO_TICKS(2000));
    Serial.printf("  core-affinity check: 200 yields on core1  violations=%d  (expected 0)\n",
                  s_affinity_violations);
    TEST_ASSERT_EQUAL(0, s_affinity_violations);

    vSemaphoreDelete(s_affinity_done);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    Serial.println("\n=== Multicore Tests ===");
    UNITY_BEGIN();
    RUN_TEST(test_core_id_is_valid);
    RUN_TEST(test_cross_core_queue_core1_to_core0);
    RUN_TEST(test_cross_core_queue_core0_to_core1);
    RUN_TEST(test_cross_core_semaphore_wake);
    RUN_TEST(test_cross_core_mutex_counter_consistency);
    RUN_TEST(test_pinned_task_stays_on_assigned_core);
    UNITY_END();
}

void loop() {}
