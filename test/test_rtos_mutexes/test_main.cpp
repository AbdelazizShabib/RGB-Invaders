#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Creation
// ---------------------------------------------------------------------------

void test_mutex_create_non_null(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    Serial.printf("  xSemaphoreCreateMutex() = %s\n", m ? "non-NULL (OK)" : "NULL (FAIL)");
    TEST_ASSERT_NOT_NULL(m);
    vSemaphoreDelete(m);
}

void test_recursive_mutex_create_non_null(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    Serial.printf("  xSemaphoreCreateRecursiveMutex() = %s\n", m ? "non-NULL (OK)" : "NULL (FAIL)");
    TEST_ASSERT_NOT_NULL(m);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Basic lock / unlock from the same task
// ---------------------------------------------------------------------------

void test_mutex_take_and_give_succeeds(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    BaseType_t take_result = xSemaphoreTake(m, 0);
    BaseType_t give_result = xSemaphoreGive(m);
    Serial.printf("  take=%s  give=%s\n",
                  take_result == pdTRUE ? "pdTRUE" : "pdFALSE",
                  give_result == pdTRUE ? "pdTRUE" : "pdFALSE");
    TEST_ASSERT_EQUAL(pdTRUE, take_result);
    TEST_ASSERT_EQUAL(pdTRUE, give_result);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Recursive mutex can be re-locked by the same task
// ---------------------------------------------------------------------------

void test_recursive_mutex_reentrant_take(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    BaseType_t t1 = xSemaphoreTakeRecursive(m, 0);
    BaseType_t t2 = xSemaphoreTakeRecursive(m, 0);
    BaseType_t t3 = xSemaphoreTakeRecursive(m, 0);
    Serial.printf("  recursive take x3: [%s, %s, %s]  (all pdTRUE = re-entrant)\n",
                  t1 == pdTRUE ? "OK" : "FAIL",
                  t2 == pdTRUE ? "OK" : "FAIL",
                  t3 == pdTRUE ? "OK" : "FAIL");
    TEST_ASSERT_EQUAL(pdTRUE, t1);
    TEST_ASSERT_EQUAL(pdTRUE, t2);
    TEST_ASSERT_EQUAL(pdTRUE, t3);
    BaseType_t g1 = xSemaphoreGiveRecursive(m);
    BaseType_t g2 = xSemaphoreGiveRecursive(m);
    BaseType_t g3 = xSemaphoreGiveRecursive(m);
    Serial.printf("  recursive give x3: [%s, %s, %s]  (all pdTRUE = fully released)\n",
                  g1 == pdTRUE ? "OK" : "FAIL",
                  g2 == pdTRUE ? "OK" : "FAIL",
                  g3 == pdTRUE ? "OK" : "FAIL");
    TEST_ASSERT_EQUAL(pdTRUE, g1);
    TEST_ASSERT_EQUAL(pdTRUE, g2);
    TEST_ASSERT_EQUAL(pdTRUE, g3);
    BaseType_t retake = xSemaphoreTakeRecursive(m, 0);
    Serial.printf("  after full release: take again = %s\n", retake == pdTRUE ? "pdTRUE (OK)" : "pdFALSE");
    TEST_ASSERT_EQUAL(pdTRUE, retake);
    xSemaphoreGiveRecursive(m);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Timeout: a held mutex blocks a second caller until timeout
// ---------------------------------------------------------------------------

void test_mutex_take_timeout_when_held(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    xSemaphoreTake(m, 0);
    xSemaphoreGive(m);
    xSemaphoreTake(m, 0);

    volatile bool contender_timed_out = false;

    struct Args { SemaphoreHandle_t m; volatile bool *flag; };
    static struct Args args;
    args.m    = m;
    args.flag = &contender_timed_out;

    xTaskCreate([](void *p) {
        auto *a = (struct Args *)p;
        TickType_t t0 = xTaskGetTickCount();
        BaseType_t ok = xSemaphoreTake(a->m, pdMS_TO_TICKS(50));
        TickType_t elapsed_ms = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;
        Serial.printf("  contender: take result=%s  waited=%u ms  (50ms timeout)\n",
                      ok == pdFALSE ? "TIMEOUT (expected)" : "acquired (unexpected)",
                      elapsed_ms);
        if (ok == pdFALSE) *a->flag = true;
        vTaskDelete(NULL);
    }, "contend", 2048, &args, 5, NULL);

    vTaskDelay(pdMS_TO_TICKS(150));
    xSemaphoreGive(m);

    TEST_ASSERT_TRUE(contender_timed_out);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Cross-task: mutex guards a shared counter; both tasks must produce
//             consistent (non-interleaved) increments.
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_counter_mutex;
static volatile int      s_shared_counter = 0;
static volatile bool     s_worker_done    = false;

static void counter_worker(void *param)
{
    for (int i = 0; i < 1000; i++) {
        xSemaphoreTake(s_counter_mutex, portMAX_DELAY);
        s_shared_counter++;
        xSemaphoreGive(s_counter_mutex);
    }
    s_worker_done = true;
    vTaskDelete(NULL);
}

void test_mutex_guards_shared_counter(void)
{
    s_counter_mutex  = xSemaphoreCreateMutex();
    s_shared_counter = 0;
    s_worker_done    = false;

    xTaskCreate(counter_worker, "cnt", 2048, NULL, 5, NULL);

    for (int i = 0; i < 1000; i++) {
        xSemaphoreTake(s_counter_mutex, portMAX_DELAY);
        s_shared_counter++;
        xSemaphoreGive(s_counter_mutex);
    }

    uint32_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
    while (!s_worker_done && xTaskGetTickCount() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    Serial.printf("  2 tasks × 1000 increments each  ->  counter=%d  (expected 2000)  worker_done=%s\n",
                  s_shared_counter, s_worker_done ? "YES" : "NO (timeout)");
    TEST_ASSERT_TRUE(s_worker_done);
    TEST_ASSERT_EQUAL(2000, s_shared_counter);
    vSemaphoreDelete(s_counter_mutex);
}

// ---------------------------------------------------------------------------
// Mutex is released on task deletion? — just verify give after take works.
// ---------------------------------------------------------------------------

void test_mutex_give_after_take_restores_availability(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    xSemaphoreTake(m, 0);
    xSemaphoreGive(m);
    BaseType_t result = xSemaphoreTake(m, 0);
    Serial.printf("  take after give: result=%s  (mutex available again)\n",
                  result == pdTRUE ? "pdTRUE (OK)" : "pdFALSE (FAIL)");
    TEST_ASSERT_EQUAL(pdTRUE, result);
    xSemaphoreGive(m);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    Serial.println("\n=== RTOS Mutex Tests ===");
    UNITY_BEGIN();
    RUN_TEST(test_mutex_create_non_null);
    RUN_TEST(test_recursive_mutex_create_non_null);
    RUN_TEST(test_mutex_take_and_give_succeeds);
    RUN_TEST(test_recursive_mutex_reentrant_take);
    RUN_TEST(test_mutex_take_timeout_when_held);
    RUN_TEST(test_mutex_guards_shared_counter);
    RUN_TEST(test_mutex_give_after_take_restores_availability);
    UNITY_END();
}

void loop() {}
