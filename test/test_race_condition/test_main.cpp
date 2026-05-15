// =============================================================================
// PHASE 8: RACE CONDITION DEMONSTRATION
//
// This test suite formally proves that unsynchronized concurrent access to a
// shared variable on a dual-core ESP32-S3 produces incorrect results, then
// proves that a mutex eliminates the problem entirely.
//
// THE RACE PATTERN (load → delay → store):
//
//   Core 0 — Task A                Core 1 — Task B
//   ─────────────────────────────  ─────────────────────────────
//   int old = counter;  // LOAD    int old = counter;  // LOAD
//    (both read the same value)
//   [1 ms delay]                   [1 ms delay]
//   counter = old + 1; // STORE    counter = old + 1; // STORE
//    ↑ one write overwrites the other — net result: only one increment
//
// Expected after N iterations each: counter == 2N
// Actual without sync:              counter ≈ N  (half the updates lost)
//
// The 1 ms window between load and store is long enough that on a 240 MHz
// dual-core processor, virtually EVERY iteration pair collides, making the
// race highly deterministic and easy to observe in the serial log.
// =============================================================================

#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_timer.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

// Number of increments each task performs.  With a 1 ms window per iteration
// and two tasks running in parallel, the test completes in ~RACE_ITERS ms.
#define RACE_ITERS 50

// ---------------------------------------------------------------------------
// Shared state
// ---------------------------------------------------------------------------

static volatile int      s_unsafe_counter;
static volatile int      s_safe_counter;
static SemaphoreHandle_t s_mutex;
static SemaphoreHandle_t s_t1_done;
static SemaphoreHandle_t s_t2_done;

// ---------------------------------------------------------------------------
// Task bodies
// ---------------------------------------------------------------------------

// UNSAFE: deliberately widens the race window between LOAD and STORE.
// The 1 ms delay gives the other core time to also load the same old value,
// so both will write back the same incremented value — losing one update.
static void racy_task(void *done_sem)
{
    for (int i = 0; i < RACE_ITERS; i++) {
        int snapshot = s_unsafe_counter;   // LOAD  — race window opens here
        vTaskDelay(pdMS_TO_TICKS(1));      // DELAY — other core reads same value
        s_unsafe_counter = snapshot + 1;  // STORE — overwrites other core's store
    }
    xSemaphoreGive((SemaphoreHandle_t)done_sem);
    vTaskDelete(NULL);
}

// SAFE: mutex wraps the entire read-modify-write section.
// While one task holds the mutex the other task blocks, so the load and store
// always operate on the most recent value.
static void safe_task(void *done_sem)
{
    for (int i = 0; i < RACE_ITERS; i++) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        int snapshot = s_safe_counter;    // LOAD  — exclusive access guaranteed
        vTaskDelay(pdMS_TO_TICKS(1));     // DELAY — other task cannot enter here
        s_safe_counter = snapshot + 1;   // STORE — always consistent
        xSemaphoreGive(s_mutex);
    }
    xSemaphoreGive((SemaphoreHandle_t)done_sem);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// TEST 1 — Without synchronisation: updates are lost
//
// Both tasks run in true parallel on separate cores.  Because the load and
// store are separated by a 1 ms window, nearly every iteration from core 0
// and core 1 overlaps: both tasks read the same stale value, both write
// back (old + 1), and one increment is silently discarded.
//
// The test ASSERTS that the result is LESS THAN the expected value,
// proving that a race condition occurred.
// ---------------------------------------------------------------------------
void test_unsynchronized_increments_lose_updates(void)
{
    s_unsafe_counter = 0;
    s_t1_done        = xSemaphoreCreateBinary();
    s_t2_done        = xSemaphoreCreateBinary();

    // Pin each task to a different core — guaranteed true parallel execution.
    xTaskCreatePinnedToCore(racy_task, "racy0", 4096, (void*)s_t1_done, 5, NULL, 0);
    xTaskCreatePinnedToCore(racy_task, "racy1", 4096, (void*)s_t2_done, 5, NULL, 1);

    xSemaphoreTake(s_t1_done, portMAX_DELAY);
    xSemaphoreTake(s_t2_done, portMAX_DELAY);

    int expected = RACE_ITERS * 2;   // 100 — correct result with no race
    int actual   = s_unsafe_counter;
    int lost     = expected - actual;

    // *** FORMAL RESULT ***
    // The counter MUST be less than 100.  Any value equal to 100 would mean
    // that by coincidence the two cores never overlapped — essentially
    // impossible with the 1 ms window and 50 paired iterations.
    Serial.printf("\n[RACE] WITHOUT MUTEX — expected=%d  actual=%d  lost_updates=%d\n",
                  expected, actual, lost);

    TEST_ASSERT_LESS_THAN(expected, actual);

    vSemaphoreDelete(s_t1_done);
    vSemaphoreDelete(s_t2_done);
}

// ---------------------------------------------------------------------------
// TEST 2 — With mutex: every increment is preserved
//
// The mutex serialises the read-modify-write triplet.  Even though the two
// tasks still run on separate cores, only one can hold the mutex at a time.
// The other task blocks at xSemaphoreTake until the holder calls Give.
// Result: all 100 increments are applied exactly once — no lost updates.
// ---------------------------------------------------------------------------
void test_mutex_protected_increments_are_exact(void)
{
    s_safe_counter = 0;
    s_mutex        = xSemaphoreCreateMutex();
    s_t1_done      = xSemaphoreCreateBinary();
    s_t2_done      = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(safe_task, "safe0", 4096, (void*)s_t1_done, 5, NULL, 0);
    xTaskCreatePinnedToCore(safe_task, "safe1", 4096, (void*)s_t2_done, 5, NULL, 1);

    xSemaphoreTake(s_t1_done, portMAX_DELAY);
    xSemaphoreTake(s_t2_done, portMAX_DELAY);

    int expected = RACE_ITERS * 2;   // 100
    int actual   = s_safe_counter;

    Serial.printf("[RACE] WITH MUTEX    — expected=%d  actual=%d  (exact!)\n\n",
                  expected, actual);

    TEST_ASSERT_EQUAL(expected, actual);

    vSemaphoreDelete(s_mutex);
    vSemaphoreDelete(s_t1_done);
    vSemaphoreDelete(s_t2_done);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
void setup()
{
    delay(2000);
    Serial.println("\n=== Phase 8: Race Condition Demonstration ===");
    UNITY_BEGIN();
    RUN_TEST(test_unsynchronized_increments_lose_updates);
    RUN_TEST(test_mutex_protected_increments_are_exact);
    UNITY_END();
}

void loop() {}
