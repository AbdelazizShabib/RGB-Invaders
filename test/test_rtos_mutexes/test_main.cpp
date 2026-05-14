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
    TEST_ASSERT_NOT_NULL(m);
    vSemaphoreDelete(m);
}

void test_recursive_mutex_create_non_null(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    TEST_ASSERT_NOT_NULL(m);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Basic lock / unlock from the same task
// ---------------------------------------------------------------------------

void test_mutex_take_and_give_succeeds(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(m, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGive(m));
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Recursive mutex can be re-locked by the same task
// ---------------------------------------------------------------------------

void test_recursive_mutex_reentrant_take(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTakeRecursive(m, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTakeRecursive(m, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTakeRecursive(m, 0));
    // Must give exactly as many times as taken
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGiveRecursive(m));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGiveRecursive(m));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGiveRecursive(m));
    // Fully released — another task could take it now
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTakeRecursive(m, 0));
    xSemaphoreGiveRecursive(m);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Timeout: a held mutex blocks a second caller until timeout
// ---------------------------------------------------------------------------

void test_mutex_take_timeout_when_held(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    xSemaphoreTake(m, 0); // hold it

    TickType_t before = xTaskGetTickCount();
    // Second take from same task is undefined behaviour for a regular mutex
    // but here we just measure that the timeout fires (it blocks immediately).
    // NOTE: On FreeRTOS, taking a mutex you already hold from the same task
    // will deadlock with a non-recursive mutex.  We release first, then test
    // timeout with zero additional takers — instead measure via a separate task.
    xSemaphoreGive(m); // release so we don't deadlock

    // Now hold it again and launch a task that tries with 50 ms timeout
    xSemaphoreTake(m, 0);

    volatile bool contender_timed_out = false;

    struct Args { SemaphoreHandle_t m; volatile bool *flag; };
    static struct Args args;
    args.m    = m;
    args.flag = &contender_timed_out;

    xTaskCreate([](void *p) {
        auto *a = (struct Args *)p;
        BaseType_t ok = xSemaphoreTake(a->m, pdMS_TO_TICKS(50));
        if (ok == pdFALSE) *a->flag = true;
        vTaskDelete(NULL);
    }, "contend", 2048, &args, 5, NULL);

    // Wait long enough for the contender to timeout
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

    // Wait for worker task to finish
    uint32_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
    while (!s_worker_done && xTaskGetTickCount() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

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
    // After give, a third party should be able to take it
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(m, 0));
    xSemaphoreGive(m);
    vSemaphoreDelete(m);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
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
