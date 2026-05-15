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
    Serial.printf("  xSemaphoreCreateBinary() = %s\n", sem ? "non-NULL (OK)" : "NULL (FAIL)");
    TEST_ASSERT_NOT_NULL(sem);
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — newly created is NOT available (must Give first)
// ---------------------------------------------------------------------------

void test_binary_semaphore_initially_unavailable(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    BaseType_t result = xSemaphoreTake(sem, 0);
    Serial.printf("  take on newly created binary semaphore (no give): result=%s  (must be unavailable)\n",
                  result == pdFALSE ? "pdFALSE (OK)" : "pdTRUE (unexpected)");
    TEST_ASSERT_EQUAL(pdFALSE, result);
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — give then take succeeds
// ---------------------------------------------------------------------------

void test_binary_semaphore_give_then_take(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    xSemaphoreGive(sem);
    BaseType_t result = xSemaphoreTake(sem, 0);
    Serial.printf("  give then take: result=%s\n", result == pdTRUE ? "pdTRUE (OK)" : "pdFALSE (FAIL)");
    TEST_ASSERT_EQUAL(pdTRUE, result);
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Binary semaphore — double give does NOT increase count past 1
// ---------------------------------------------------------------------------

void test_binary_semaphore_double_give_only_one_take(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    BaseType_t first  = xSemaphoreTake(sem, 0);
    BaseType_t second = xSemaphoreTake(sem, 0);
    Serial.printf("  double-give: 1st take=%s  2nd take=%s  (binary max=1)\n",
                  first  == pdTRUE  ? "pdTRUE"  : "pdFALSE",
                  second == pdFALSE ? "pdFALSE (OK)" : "pdTRUE (unexpected)");
    TEST_ASSERT_EQUAL(pdTRUE,  first);
    TEST_ASSERT_EQUAL(pdFALSE, second);
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
    Serial.printf("  take with 50ms timeout on unavailable semaphore: elapsed=%u ms  (>= 40ms)\n",
                  elapsed_ms);
    TEST_ASSERT_GREATER_OR_EQUAL(40, elapsed_ms);
    vSemaphoreDelete(sem);
}

// ---------------------------------------------------------------------------
// Counting semaphore — creation and initial count
// ---------------------------------------------------------------------------

void test_counting_semaphore_create(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    Serial.printf("  xSemaphoreCreateCounting(max=5, initial=0) = %s\n",
                  sem ? "non-NULL (OK)" : "NULL (FAIL)");
    TEST_ASSERT_NOT_NULL(sem);
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_initial_count_zero_blocks(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    BaseType_t result = xSemaphoreTake(sem, 0);
    Serial.printf("  counting(max=5, initial=0) take: result=%s  (count=0 so must block)\n",
                  result == pdFALSE ? "pdFALSE (OK)" : "pdTRUE (unexpected)");
    TEST_ASSERT_EQUAL(pdFALSE, result);
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_initial_count_nonzero_allows_takes(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 3);
    BaseType_t t1 = xSemaphoreTake(sem, 0);
    BaseType_t t2 = xSemaphoreTake(sem, 0);
    BaseType_t t3 = xSemaphoreTake(sem, 0);
    BaseType_t t4 = xSemaphoreTake(sem, 0);
    Serial.printf("  counting(max=5, initial=3): takes [%s, %s, %s, %s]  (3 OK then blocked)\n",
                  t1 == pdTRUE ? "OK" : "FAIL",
                  t2 == pdTRUE ? "OK" : "FAIL",
                  t3 == pdTRUE ? "OK" : "FAIL",
                  t4 == pdFALSE ? "BLOCKED(OK)" : "unexpected");
    TEST_ASSERT_EQUAL(pdTRUE,  t1);
    TEST_ASSERT_EQUAL(pdTRUE,  t2);
    TEST_ASSERT_EQUAL(pdTRUE,  t3);
    TEST_ASSERT_EQUAL(pdFALSE, t4);
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_give_increments_count(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    BaseType_t t1 = xSemaphoreTake(sem, 0);
    BaseType_t t2 = xSemaphoreTake(sem, 0);
    BaseType_t t3 = xSemaphoreTake(sem, 0);
    Serial.printf("  gave 2x then takes [%s, %s, %s]  (2 available, 3rd blocked)\n",
                  t1 == pdTRUE ? "OK" : "FAIL",
                  t2 == pdTRUE ? "OK" : "FAIL",
                  t3 == pdFALSE ? "BLOCKED(OK)" : "unexpected");
    TEST_ASSERT_EQUAL(pdTRUE,  t1);
    TEST_ASSERT_EQUAL(pdTRUE,  t2);
    TEST_ASSERT_EQUAL(pdFALSE, t3);
    vSemaphoreDelete(sem);
}

void test_counting_semaphore_saturates_at_max(void)
{
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(2, 0);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    BaseType_t t1 = xSemaphoreTake(sem, 0);
    BaseType_t t2 = xSemaphoreTake(sem, 0);
    BaseType_t t3 = xSemaphoreTake(sem, 0);
    Serial.printf("  gave 3x on max=2 sem: takes [%s, %s, %s]  (saturates at max=2)\n",
                  t1 == pdTRUE ? "OK" : "FAIL",
                  t2 == pdTRUE ? "OK" : "FAIL",
                  t3 == pdFALSE ? "BLOCKED(OK)" : "unexpected");
    TEST_ASSERT_EQUAL(pdTRUE,  t1);
    TEST_ASSERT_EQUAL(pdTRUE,  t2);
    TEST_ASSERT_EQUAL(pdFALSE, t3);
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
    TickType_t t0 = xTaskGetTickCount();
    BaseType_t ok = xSemaphoreTake(s_isr_sem, pdMS_TO_TICKS(200));
    uint32_t elapsed_ms = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;
    Serial.printf("  giver task signals after ~30ms: take result=%s  waited=%u ms\n",
                  ok == pdTRUE ? "pdTRUE (unblocked)" : "TIMEOUT",
                  elapsed_ms);
    TEST_ASSERT_EQUAL(pdTRUE, ok);
    vSemaphoreDelete(s_isr_sem);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    Serial.println("\n=== RTOS Semaphore Tests ===");
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
