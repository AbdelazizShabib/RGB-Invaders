#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Binary semaphore — creation
// ---------------------------------------------------------------------------

void test_binary_semaphore_create_non_null(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_NULL(sem);
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — newly created is NOT available (must Give first)
// ---------------------------------------------------------------------------

void test_binary_semaphore_initially_unavailable(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    // Zero timeout — must fail immediately
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0));
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — give then take succeeds
// ---------------------------------------------------------------------------

void test_binary_semaphore_give_then_take(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    xSemaphoreGive(sem);
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sem, 0));
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — double give does NOT increase count past 1
// ---------------------------------------------------------------------------

void test_binary_semaphore_double_give_only_one_take(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    xSemaphoreGive(sem);
    xSemaphoreGive(sem); // second give is a no-op for binary
    TEST_ASSERT_EQUAL(pdTRUE,  xSemaphoreTake(sem, 0)); // first take succeeds
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0)); // second must fail
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — take with timeout actually blocks then times out
// ---------------------------------------------------------------------------

void test_binary_semaphore_take_timeout_fires(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    TickType_t before = xTaskGetTickCount();
    xSemaphoreTake(sem, pdMS_TO_TICKS(50));
    TickType_t after  = xTaskGetTickCount();
    uint32_t elapsed_ms = (after - before) * portTICK_PERIOD_MS;
    // Must have waited at least ~50 ms (allow small scheduling jitter)
    TEST_ASSERT_GREATER_OR_EQUAL(40, elapsed_ms);
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Counting semaphore — creation and initial count
// ---------------------------------------------------------------------------

void test_counting_semaphore_create(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    TEST_ASSERT_NOT_NULL(sem);
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_initial_count_zero_blocks(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0));
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_initial_count_nonzero_allows_takes(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 3);
    TEST_ASSERT_EQUAL(pdTRUE,  xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdTRUE,  xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdTRUE,  xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0)); // count exhausted
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_give_increments_count(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0));
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_saturates_at_max(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(2, 0);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem); // beyond max — should be rejected
    TEST_ASSERT_EQUAL(pdTRUE,  xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdTRUE,  xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0));
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Cross-task: ISR-simulated give from a separate task unblocks waiter
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_isr_sem;

static void giver_task(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(30));
    xSemaphoreGive(s_isr_sem);
    vTaskDelete(NULL);
}

void test_semaphore_given_from_task_unblocks_waiter(void)
{
    s_isr_sem = xSemaphoreCreateBinary();
    xTaskCreate(giver_task, "giver", 2048, NULL, 5, NULL);
    // Block waiting; giver should release within 30 ms
    BaseType_t ok = xSemaphoreTake(s_isr_sem, pdMS_TO_TICKS(200));
    TEST_ASSERT_EQUAL(pdTRUE, ok);
    vSemaphoreDelete(s_isr_sem);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_binary_semaphore_create_non_null);
    RUN_TEST(test_binary_semaphore_initially_unavailable);
    RUN_TEST(test_binary_semaphore_give_then_take);
    RUN_TEST(test_binary_semaphore_double_give_only_one_take);
    RUN_TEST(test_binary_semaphore_take_timeout_fires);
    RUN_TEST(test_counting_semaphore_create);
    RUN_TEST(test_counting_semaphore_initial_count_zero_blocks);
    RUN_TEST(test_counting_semaphore_initial_count_nonzero_allows_takes);
    RUN_TEST(test_counting_semaphore_give_increments_count);
    RUN_TEST(test_counting_semaphore_saturates_at_max);
    RUN_TEST(test_semaphore_given_from_task_unblocks_waiter);
    UNITY_END();
}

void loop() {}
