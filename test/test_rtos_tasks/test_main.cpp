#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Task creation and deletion
// ---------------------------------------------------------------------------

static volatile bool s_task_ran = false;
static SemaphoreHandle_t s_done_sem;

static void minimal_task(void *param)
{
    s_task_ran = true;
    xSemaphoreGive(s_done_sem);
    vTaskDelete(NULL);
}

void test_task_create_and_run(void)
{
    s_task_ran = false;
    s_done_sem = xSemaphoreCreateBinary();
    xTaskCreate(minimal_task, "min", 2048, NULL, 5, NULL);
    xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(500));
    Serial.printf("  task created and ran: task_ran=%s\n", s_task_ran ? "true (OK)" : "false (FAIL)");
    TEST_ASSERT_TRUE(s_task_ran);
    vSemaphoreDelete(s_done_sem);
}

// ---------------------------------------------------------------------------
// Core affinity — verify pinned tasks run on the intended core
// ---------------------------------------------------------------------------

static volatile int s_core_id_seen = -1;

static void core_check_task(void *param)
{
    s_core_id_seen = xPortGetCoreID();
    xSemaphoreGive(s_done_sem);
    vTaskDelete(NULL);
}

void test_task_pinned_to_core_0_runs_on_core_0(void)
{
    s_core_id_seen = -1;
    s_done_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(core_check_task, "c0", 2048, NULL, 5, NULL, 0);
    xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(500));
    Serial.printf("  task pinned to core 0: ran on core %d  (expected 0)\n", s_core_id_seen);
    TEST_ASSERT_EQUAL(0, s_core_id_seen);
    vSemaphoreDelete(s_done_sem);
}

void test_task_pinned_to_core_1_runs_on_core_1(void)
{
    s_core_id_seen = -1;
    s_done_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(core_check_task, "c1", 2048, NULL, 5, NULL, 1);
    xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(500));
    Serial.printf("  task pinned to core 1: ran on core %d  (expected 1)\n", s_core_id_seen);
    TEST_ASSERT_EQUAL(1, s_core_id_seen);
    vSemaphoreDelete(s_done_sem);
}

// ---------------------------------------------------------------------------
// Stack watermark — created task must have usable stack remaining
// ---------------------------------------------------------------------------

static volatile UBaseType_t s_watermark = 0;

static void watermark_task(void *param)
{
    volatile uint8_t buf[64];
    for (int i = 0; i < 64; i++) buf[i] = (uint8_t)i;
    (void)buf;
    s_watermark = uxTaskGetStackHighWaterMark(NULL);
    xSemaphoreGive(s_done_sem);
    vTaskDelete(NULL);
}

void test_task_stack_watermark_above_zero(void)
{
    s_watermark = 0;
    s_done_sem  = xSemaphoreCreateBinary();
    xTaskCreate(watermark_task, "wm", 4096, NULL, 5, NULL);
    xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(500));
    Serial.printf("  stack high-water mark = %u words  (%u bytes remaining, min 64 words required)\n",
                  s_watermark, s_watermark * sizeof(StackType_t));
    TEST_ASSERT_GREATER_THAN(64, s_watermark);
    vSemaphoreDelete(s_done_sem);
}

// ---------------------------------------------------------------------------
// Task priority: higher-priority task preempts a spinning lower one
// ---------------------------------------------------------------------------

static volatile bool s_high_prio_ran   = false;
static volatile bool s_low_spin_active = false;

static void high_prio_task(void *param)
{
    s_high_prio_ran = true;
    xSemaphoreGive(s_done_sem);
    vTaskDelete(NULL);
}

static void low_prio_spin_task(void *param)
{
    s_low_spin_active = true;
    while (!s_high_prio_ran) {
        taskYIELD();
    }
    vTaskDelete(NULL);
}

void test_high_priority_task_runs_before_low(void)
{
    s_high_prio_ran   = false;
    s_low_spin_active = false;
    s_done_sem        = xSemaphoreCreateBinary();

    xTaskCreate(low_prio_spin_task, "low",  2048, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
    xTaskCreate(high_prio_task, "high", 2048, NULL, 7, NULL);
    BaseType_t ok = xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(500));
    Serial.printf("  high-prio(7) created after low-prio(3): high ran=%s  result=%s\n",
                  s_high_prio_ran ? "true" : "false",
                  ok == pdTRUE ? "pdTRUE (preempted low)" : "TIMEOUT");
    TEST_ASSERT_EQUAL(pdTRUE, ok);
    TEST_ASSERT_TRUE(s_high_prio_ran);
    vSemaphoreDelete(s_done_sem);
}

// ---------------------------------------------------------------------------
// vTaskDelayUntil: task wakes at consistent interval
// ---------------------------------------------------------------------------

void test_vtask_delay_until_period(void)
{
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);
    TickType_t before = xTaskGetTickCount();

    for (int i = 0; i < 5; i++) {
        vTaskDelayUntil(&last_wake, period);
    }

    TickType_t after   = xTaskGetTickCount();
    uint32_t elapsed_ms = (after - before) * portTICK_PERIOD_MS;
    Serial.printf("  vTaskDelayUntil: 5 x 20ms = %u ms elapsed  (expected ~100ms, tolerance ±15ms)\n",
                  elapsed_ms);
    TEST_ASSERT_GREATER_OR_EQUAL(85, elapsed_ms);
    TEST_ASSERT_LESS_OR_EQUAL(130, elapsed_ms);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    Serial.println("\n=== RTOS Task Tests ===");
    UNITY_BEGIN();
    RUN_TEST(test_task_create_and_run);
    RUN_TEST(test_task_pinned_to_core_0_runs_on_core_0);
    RUN_TEST(test_task_pinned_to_core_1_runs_on_core_1);
    RUN_TEST(test_task_stack_watermark_above_zero);
    RUN_TEST(test_high_priority_task_runs_before_low);
    RUN_TEST(test_vtask_delay_until_period);
    UNITY_END();
}

void loop() {}
