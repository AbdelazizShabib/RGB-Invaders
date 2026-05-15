// =============================================================================
// PHASE 9: DEADLOCK AND PRIORITY INVERSION DEMONSTRATION
//
// This suite demonstrates two classic RTOS synchronisation failures:
//
//  ── SECTION A: DEADLOCK ────────────────────────────────────────────────────
//
//  A deadlock occurs when two tasks each hold one lock and are waiting for
//  the other's lock — neither can ever proceed.
//
//  Classic ABBA pattern:
//
//    Task A                       Task B
//    ──────────────────────────   ──────────────────────────
//    take(M1)  ← succeeds         take(M2)  ← succeeds
//    take(M2)  ← BLOCKS           take(M1)  ← BLOCKS
//     waiting for B to release     waiting for A to release
//          ↑──────── DEADLOCK ─────────↑
//
//  This test demonstrates the scenario safely by using bounded timeouts
//  on the second take so neither task hangs permanently.
//
//  Fix: enforce a CONSISTENT LOCK ORDER.  If every task always acquires M1
//  before M2, the ABBA circle is broken and deadlock becomes impossible.
//
//  ── SECTION B: PRIORITY INVERSION ─────────────────────────────────────────
//
//  Priority inversion occurs when a high-priority task is forced to wait
//  for a LOW-priority task that has been preempted by a MEDIUM-priority task.
//
//    Priority 7  HIGH   ── blocked waiting for lock held by LOW ──────────────→
//    Priority 5  MED    ──── runs, consuming CPU, preventing LOW from releasing ─→
//    Priority 3  LOW    ── holds lock ─ preempted by MED ─ can't release ────────→
//                            time ──────────────────────────────────────────────→
//
//  Result: HIGH task waits much longer than the LOW task's intended hold time.
//
//  Fix: Priority Inheritance Protocol (PIP).  When HIGH blocks on a mutex,
//  FreeRTOS temporarily raises LOW's priority to HIGH's level (7), so LOW
//  preempts MED, releases the mutex quickly, and HIGH unblocks on time.
//
//  This test uses two approaches:
//    ① Binary semaphore — no PIP → HIGH waits a long time (inversion visible)
//    ② Mutex            — PIP active → HIGH waits only the remaining hold time
//
//  All four tasks in Section B are pinned to core 1 to force single-core
//  RTOS scheduling, making priority behaviour clearly observable.
// =============================================================================

#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_timer.h>

void setUp(void)    {}
void tearDown(void) {}

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  SECTION A — DEADLOCK                                                   ║
// ╚══════════════════════════════════════════════════════════════════════════╝

// ---------------------------------------------------------------------------
// Shared locks for the ABBA scenario
// ---------------------------------------------------------------------------
static SemaphoreHandle_t s_M1;
static SemaphoreHandle_t s_M2;

// Rendezvous semaphores so both tasks take their first lock BEFORE
// either attempts its second lock — guaranteeing the deadlock setup.
static SemaphoreHandle_t s_A_has_M1;   // Task A signals it holds M1
static SemaphoreHandle_t s_B_has_M2;   // Task B signals it holds M2

// Result flags — set inside each task to report outcome
static volatile bool s_A_got_M2   = false;  // true only if Task A acquired M2
static volatile bool s_B_got_M1   = false;  // true only if Task B acquired M1
static SemaphoreHandle_t s_A_done;
static SemaphoreHandle_t s_B_done;

// ---------------------------------------------------------------------------
// Task A: acquires M1, waits for B to acquire M2, then tries M2 (ABBA).
// ---------------------------------------------------------------------------
static void task_A_abba(void *param)
{
    // Step 1: Acquire first lock (M1).
    xSemaphoreTake(s_M1, portMAX_DELAY);

    // Step 2: Signal that we hold M1 and wait for B to hold M2.
    //         This rendezvous guarantees both are in the "hold one, want other" state.
    xSemaphoreGive(s_A_has_M1);
    xSemaphoreTake(s_B_has_M2, portMAX_DELAY);

    // Step 3: Attempt to acquire M2.  B holds M2 and wants M1 — DEADLOCK.
    //         The 200 ms timeout prevents the system from hanging permanently.
    BaseType_t got = xSemaphoreTake(s_M2, pdMS_TO_TICKS(200));
    s_A_got_M2 = (got == pdTRUE);
    if (got == pdTRUE) xSemaphoreGive(s_M2);

    xSemaphoreGive(s_M1);
    xSemaphoreGive(s_A_done);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// Task B: acquires M2, waits for A to acquire M1, then tries M1 (ABBA).
// ---------------------------------------------------------------------------
static void task_B_abba(void *param)
{
    // Step 1: Acquire first lock (M2) — opposite order to Task A.
    xSemaphoreTake(s_M2, portMAX_DELAY);

    // Step 2: Signal that we hold M2 and wait for A to hold M1.
    xSemaphoreGive(s_B_has_M2);
    xSemaphoreTake(s_A_has_M1, portMAX_DELAY);

    // Step 3: Attempt to acquire M1 — A holds M1 and wants M2 — DEADLOCK.
    BaseType_t got = xSemaphoreTake(s_M1, pdMS_TO_TICKS(200));
    s_B_got_M1 = (got == pdTRUE);
    if (got == pdTRUE) xSemaphoreGive(s_M1);

    xSemaphoreGive(s_M2);
    xSemaphoreGive(s_B_done);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// TEST A-1: ABBA lock order causes deadlock (detected via timeout)
//
// Both tasks time out trying to acquire the second lock.
// Neither s_A_got_M2 nor s_B_got_M1 will be true — proving deadlock.
// ---------------------------------------------------------------------------
void test_abba_lock_order_causes_deadlock(void)
{
    s_M1       = xSemaphoreCreateMutex();
    s_M2       = xSemaphoreCreateMutex();
    s_A_has_M1 = xSemaphoreCreateBinary();
    s_B_has_M2 = xSemaphoreCreateBinary();
    s_A_done   = xSemaphoreCreateBinary();
    s_B_done   = xSemaphoreCreateBinary();
    s_A_got_M2 = false;
    s_B_got_M1 = false;

    xTaskCreate(task_A_abba, "taskA", 4096, NULL, 5, NULL);
    xTaskCreate(task_B_abba, "taskB", 4096, NULL, 5, NULL);

    xSemaphoreTake(s_A_done, portMAX_DELAY);
    xSemaphoreTake(s_B_done, portMAX_DELAY);

    Serial.printf("\n[DEADLOCK] ABBA result: A_got_M2=%s  B_got_M1=%s\n",
                  s_A_got_M2 ? "YES" : "TIMEOUT",
                  s_B_got_M1 ? "YES" : "TIMEOUT");
    Serial.printf("[DEADLOCK] Both timed out — deadlock confirmed.\n\n");

    // Neither task should have successfully acquired the second lock.
    TEST_ASSERT_FALSE(s_A_got_M2);
    TEST_ASSERT_FALSE(s_B_got_M1);

    vSemaphoreDelete(s_M1);
    vSemaphoreDelete(s_M2);
    vSemaphoreDelete(s_A_has_M1);
    vSemaphoreDelete(s_B_has_M2);
    vSemaphoreDelete(s_A_done);
    vSemaphoreDelete(s_B_done);
}

// ---------------------------------------------------------------------------
// Fixed tasks: BOTH acquire in the same order — M1 then M2.
// ---------------------------------------------------------------------------
static volatile bool s_fixed_A_done_flag = false;
static volatile bool s_fixed_B_done_flag = false;
static SemaphoreHandle_t s_fixed_done1;
static SemaphoreHandle_t s_fixed_done2;
static volatile int s_fixed_work_count = 0;

static void task_fixed_order(void *done_sem)
{
    // Always acquire M1 first, then M2 — same order as every other task.
    xSemaphoreTake(s_M1, portMAX_DELAY);
    xSemaphoreTake(s_M2, portMAX_DELAY);

    // Critical section — both locks held simultaneously.
    s_fixed_work_count++;

    xSemaphoreGive(s_M2);
    xSemaphoreGive(s_M1);

    xSemaphoreGive((SemaphoreHandle_t)done_sem);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// TEST A-2: Consistent lock order prevents deadlock
//
// With both tasks always acquiring M1 → M2, at worst one task blocks at M1
// until the other finishes and releases both.  No circular dependency.
// The work counter should equal 2 — both tasks completed.
// ---------------------------------------------------------------------------
void test_consistent_lock_order_prevents_deadlock(void)
{
    s_M1             = xSemaphoreCreateMutex();
    s_M2             = xSemaphoreCreateMutex();
    s_fixed_done1    = xSemaphoreCreateBinary();
    s_fixed_done2    = xSemaphoreCreateBinary();
    s_fixed_work_count = 0;

    xTaskCreate(task_fixed_order, "fixA", 4096, (void*)s_fixed_done1, 5, NULL);
    xTaskCreate(task_fixed_order, "fixB", 4096, (void*)s_fixed_done2, 5, NULL);

    // Both should complete within a generous timeout — no deadlock.
    BaseType_t ok1 = xSemaphoreTake(s_fixed_done1, pdMS_TO_TICKS(2000));
    BaseType_t ok2 = xSemaphoreTake(s_fixed_done2, pdMS_TO_TICKS(2000));

    Serial.printf("[DEADLOCK] FIXED ORDER: taskA done=%s  taskB done=%s  work_count=%d\n\n",
                  ok1 == pdTRUE ? "YES" : "TIMEOUT",
                  ok2 == pdTRUE ? "YES" : "TIMEOUT",
                  s_fixed_work_count);

    TEST_ASSERT_EQUAL(pdTRUE, ok1);
    TEST_ASSERT_EQUAL(pdTRUE, ok2);
    TEST_ASSERT_EQUAL(2, s_fixed_work_count);

    vSemaphoreDelete(s_M1);
    vSemaphoreDelete(s_M2);
    vSemaphoreDelete(s_fixed_done1);
    vSemaphoreDelete(s_fixed_done2);
}

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  SECTION B — PRIORITY INVERSION                                         ║
// ╚══════════════════════════════════════════════════════════════════════════╝

// ---------------------------------------------------------------------------
// Timing constants (all in ms)
// ---------------------------------------------------------------------------
#define LOW_HOLD_MS    200   // How long the LOW task holds the lock
#define HIGH_START_MS   30   // HIGH task starts trying to acquire this far in
#define MED_START_MS    40   // MED spinner starts just after HIGH starts waiting
#define MED_SPIN_MS    250   // How long MED spinner occupies core 1

// ---------------------------------------------------------------------------
// Shared state for priority inversion tests
// ---------------------------------------------------------------------------
static SemaphoreHandle_t s_prio_lock;      // the contested resource
static volatile int64_t  s_high_block_start_us;
static volatile int64_t  s_high_unblock_us;
static SemaphoreHandle_t s_high_done;
static SemaphoreHandle_t s_low_done;

// ---------------------------------------------------------------------------
// LOW task (priority 3): holds the lock for LOW_HOLD_MS then releases.
// Pinned to core 1 — subject to preemption by any higher-priority task
// also on core 1.
// ---------------------------------------------------------------------------
static void low_prio_holder(void *param)
{
    xSemaphoreTake(s_prio_lock, portMAX_DELAY);
    // Simulate low-priority work done while holding the lock.
    // vTaskDelay yields — the task can be preempted while sleeping.
    vTaskDelay(pdMS_TO_TICKS(LOW_HOLD_MS));
    xSemaphoreGive(s_prio_lock);

    xSemaphoreGive(s_low_done);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// MEDIUM spinner (priority 5): tight CPU loop with no yield.
// Represents background processing that should not delay a high-priority task.
// On a single core it WILL preempt the low task (5 > 3), keeping it from
// releasing the lock unless the low task inherits a higher priority.
// ---------------------------------------------------------------------------
static void med_prio_spinner(void *param)
{
    // Spin for MED_SPIN_MS consuming 100% of core 1.
    int64_t end_us = esp_timer_get_time() + (int64_t)MED_SPIN_MS * 1000;
    while (esp_timer_get_time() < end_us) { /* tight spin */ }
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// HIGH task (priority 7): tries to acquire the lock after HIGH_START_MS.
// Records exact wait time using esp_timer.
// ---------------------------------------------------------------------------
static void high_prio_waiter(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(HIGH_START_MS));

    s_high_block_start_us = esp_timer_get_time();
    xSemaphoreTake(s_prio_lock, portMAX_DELAY);  // blocks here
    s_high_unblock_us = esp_timer_get_time();

    xSemaphoreGive(s_prio_lock);
    xSemaphoreGive(s_high_done);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// Helper: run one priority-inversion scenario and return the HIGH task's
// measured blocking time in milliseconds.
//
// use_mutex: true  → xSemaphoreCreateMutex()   — PIP active
//            false → xSemaphoreCreateBinary()  — no PIP
// ---------------------------------------------------------------------------
static int64_t run_prio_scenario(bool use_mutex)
{
    s_prio_lock = use_mutex ? xSemaphoreCreateMutex()
                            : xSemaphoreCreateBinary();
    s_high_done = xSemaphoreCreateBinary();
    s_low_done  = xSemaphoreCreateBinary();

    // Binary semaphore must be given once before it can be taken as a lock.
    if (!use_mutex) xSemaphoreGive(s_prio_lock);

    s_high_block_start_us = 0;
    s_high_unblock_us     = 0;

    // All three tasks pinned to core 1 — single-core scheduling guarantees
    // priority-driven preemption with no parallelism to muddy the result.
    xTaskCreatePinnedToCore(low_prio_holder,  "low",  4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(high_prio_waiter, "high", 4096, NULL, 7, NULL, 1);

    // Medium spinner starts slightly after HIGH begins blocking, so it can
    // demonstrate its effect on the low task.
    vTaskDelay(pdMS_TO_TICKS(MED_START_MS));
    xTaskCreatePinnedToCore(med_prio_spinner, "med",  4096, NULL, 5, NULL, 1);

    xSemaphoreTake(s_high_done, portMAX_DELAY);
    xSemaphoreTake(s_low_done,  portMAX_DELAY);

    int64_t wait_us = s_high_unblock_us - s_high_block_start_us;

    vSemaphoreDelete(s_prio_lock);
    vSemaphoreDelete(s_high_done);
    vSemaphoreDelete(s_low_done);

    return wait_us / 1000; // return ms
}

// ---------------------------------------------------------------------------
// TEST B-1: Priority inversion — HIGH task excessively delayed by MED spinner
//
// WITHOUT priority inheritance (binary semaphore):
//   • LOW task (prio 3) holds the lock.
//   • HIGH task (prio 7) blocks waiting for the lock.
//   • MED spinner (prio 5) preempts LOW (5 > 3) and runs for MED_SPIN_MS.
//   • LOW cannot release the lock while MED is running.
//   • HIGH waits at least MED_SPIN_MS ms beyond its expected wake-up.
//
// The test asserts that the HIGH task's wait time significantly exceeds
// (LOW_HOLD_MS - HIGH_START_MS) — the "ideal" wait with no medium task.
// ---------------------------------------------------------------------------
void test_priority_inversion_binary_semaphore(void)
{
    int64_t wait_ms = run_prio_scenario(/*use_mutex=*/false);

    // Ideal wait: time remaining in LOW's hold after HIGH starts trying.
    int64_t ideal_wait_ms = LOW_HOLD_MS - HIGH_START_MS; // 170 ms

    Serial.printf("\n[PRIO-INV] BINARY SEM (no PIP):\n");
    Serial.printf("           Ideal wait = %lld ms\n", ideal_wait_ms);
    Serial.printf("           Actual wait = %lld ms\n", wait_ms);
    Serial.printf("           Extra delay (inversion) = %lld ms\n\n",
                  wait_ms - ideal_wait_ms);

    // The HIGH task's actual wait should be substantially more than the ideal
    // because MED (250 ms spinner) blocked LOW from releasing the lock early.
    // Allow generous tolerance since timer precision varies.
    TEST_ASSERT_GREATER_THAN(ideal_wait_ms + 50, wait_ms);
}

// ---------------------------------------------------------------------------
// TEST B-2: Priority inheritance — mutex limits inversion to the hold time
//
// WITH priority inheritance (mutex):
//   • LOW task (prio 3) holds the mutex.
//   • HIGH task (prio 7) blocks on the mutex.
//   • FreeRTOS immediately raises LOW's effective priority to 7 (inherits HIGH's).
//   • LOW (now prio 7) preempts MED (prio 5) and finishes its hold time.
//   • Lock released; HIGH unblocks promptly.
//   • HIGH's wait ≈ (LOW_HOLD_MS - HIGH_START_MS) — only the remaining hold.
//
// The test asserts the HIGH task's wait is close to the ideal, NOT inflated
// by MED's spin time — proving priority inheritance is working.
// ---------------------------------------------------------------------------
void test_priority_inheritance_mutex_limits_inversion(void)
{
    int64_t wait_ms = run_prio_scenario(/*use_mutex=*/true);

    int64_t ideal_wait_ms = LOW_HOLD_MS - HIGH_START_MS; // 170 ms

    Serial.printf("[PRIO-INV] MUTEX (PIP active):\n");
    Serial.printf("           Ideal wait = %lld ms\n", ideal_wait_ms);
    Serial.printf("           Actual wait = %lld ms\n", wait_ms);
    Serial.printf("           Overhead above ideal = %lld ms\n\n",
                  wait_ms - ideal_wait_ms);

    // With PIP the wait should be close to ideal (within ~50 ms scheduling noise).
    // It must be much shorter than the binary-semaphore case.
    TEST_ASSERT_LESS_OR_EQUAL(ideal_wait_ms + 50, wait_ms);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
void setup()
{
    delay(2000);
    Serial.println("\n=== Phase 9: Deadlock & Priority Inversion Demonstration ===");
    UNITY_BEGIN();
    RUN_TEST(test_abba_lock_order_causes_deadlock);
    RUN_TEST(test_consistent_lock_order_prevents_deadlock);
    RUN_TEST(test_priority_inversion_binary_semaphore);
    RUN_TEST(test_priority_inheritance_mutex_limits_inversion);
    UNITY_END();
}

void loop() {}
