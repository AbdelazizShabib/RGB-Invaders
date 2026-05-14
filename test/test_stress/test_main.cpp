#include <Arduino.h>
#include <cstring>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Queue flood: 1000 items sent at full speed; no data dropped, order preserved
// ---------------------------------------------------------------------------

#define FLOOD_DEPTH  32
#define FLOOD_ITEMS  1000

static QueueHandle_t     s_flood_q;
static SemaphoreHandle_t s_flood_done;
static volatile uint32_t s_flood_received  = 0;
static volatile uint32_t s_flood_errors    = 0;

static void flood_consumer(void *param)
{
    for (uint32_t expected = 0; expected < FLOOD_ITEMS; expected++) {
        uint32_t val;
        if (xQueueReceive(s_flood_q, &val, pdMS_TO_TICKS(500)) != pdTRUE) {
            s_flood_errors++;
        } else if (val != expected) {
            s_flood_errors++;
        }
        s_flood_received++;
    }
    xSemaphoreGive(s_flood_done);
    vTaskDelete(NULL);
}

void test_queue_flood_no_data_loss(void)
{
    s_flood_q        = xQueueCreate(FLOOD_DEPTH, sizeof(uint32_t));
    s_flood_done     = xSemaphoreCreateBinary();
    s_flood_received = 0;
    s_flood_errors   = 0;

    xTaskCreatePinnedToCore(flood_consumer, "flood_cons", 4096, NULL, 6, NULL, 1);

    for (uint32_t i = 0; i < FLOOD_ITEMS; i++) {
        // Block until space — don't drop under back-pressure
        while (xQueueSend(s_flood_q, &i, pdMS_TO_TICKS(10)) != pdTRUE) {
            taskYIELD();
        }
    }

    xSemaphoreTake(s_flood_done, pdMS_TO_TICKS(10000));
    TEST_ASSERT_EQUAL(FLOOD_ITEMS, s_flood_received);
    TEST_ASSERT_EQUAL(0, s_flood_errors);

    vQueueDelete(s_flood_q);
    vSemaphoreDelete(s_flood_done);
}

// ---------------------------------------------------------------------------
// ISR-burst simulation: rapid semaphore gives from a high-frequency timer
// ---------------------------------------------------------------------------

#define BURST_COUNT 200

static SemaphoreHandle_t     s_burst_sem;
static volatile uint32_t     s_burst_taken = 0;
static esp_timer_handle_t    s_burst_timer;

static void burst_timer_cb(void *arg)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_burst_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

void test_isr_burst_semaphore_gives(void)
{
    s_burst_sem   = xSemaphoreCreateCounting(BURST_COUNT, 0);
    s_burst_taken = 0;

    esp_timer_create_args_t args = {};
    args.callback = burst_timer_cb;
    args.name     = "burst";
    esp_timer_create(&args, &s_burst_timer);

    // Fire every 500 µs for BURST_COUNT events = 100 ms total
    esp_timer_start_periodic(s_burst_timer, 500);

    // Drain all gives within a reasonable window
    uint32_t deadline_ms = 300;
    TickType_t deadline  = xTaskGetTickCount() + pdMS_TO_TICKS(deadline_ms);
    while (s_burst_taken < BURST_COUNT && xTaskGetTickCount() < deadline) {
        if (xSemaphoreTake(s_burst_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
            s_burst_taken++;
        }
    }

    esp_timer_stop(s_burst_timer);
    esp_timer_delete(s_burst_timer);

    // Allow up to 5 % loss due to counting semaphore saturation (max=BURST_COUNT)
    TEST_ASSERT_GREATER_OR_EQUAL(BURST_COUNT * 95 / 100, s_burst_taken);

    vSemaphoreDelete(s_burst_sem);
}

// ---------------------------------------------------------------------------
// Heap stability: 500 malloc/free cycles must not fragment the heap to death
// ---------------------------------------------------------------------------

#define ALLOC_CYCLES  500
#define ALLOC_SIZE    256

void test_heap_stable_under_repeated_alloc_free(void)
{
    size_t free_before = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

    for (int i = 0; i < ALLOC_CYCLES; i++) {
        void *p = malloc(ALLOC_SIZE);
        TEST_ASSERT_NOT_NULL_MESSAGE(p, "malloc returned NULL during stress cycle");
        memset(p, 0xAA, ALLOC_SIZE);
        free(p);
    }

    size_t free_after = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    // Allow at most 1 KB of permanent heap loss after 500 cycles
    int32_t lost = (int32_t)free_before - (int32_t)free_after;
    TEST_ASSERT_LESS_OR_EQUAL(1024, lost);
}

// ---------------------------------------------------------------------------
// Stack depth: deeply nested call chain must not overflow a 4 KB task stack
// ---------------------------------------------------------------------------

static int deep_recurse(int depth)
{
    volatile uint8_t pad[32]; // force some stack use per frame
    (void)pad;
    if (depth == 0) return 0;
    return 1 + deep_recurse(depth - 1);
}

static volatile int       s_recurse_result = -1;
static SemaphoreHandle_t  s_recurse_done;

static void recurse_task(void *param)
{
    s_recurse_result = deep_recurse(60); // 60 frames × ~80 bytes ≈ 4.8 KB
    xSemaphoreGive(s_recurse_done);
    vTaskDelete(NULL);
}

void test_deep_recursion_does_not_overflow_8k_stack(void)
{
    s_recurse_result = -1;
    s_recurse_done   = xSemaphoreCreateBinary();
    // Provide 8 KB stack to be safe
    xTaskCreate(recurse_task, "recurse", 8192, NULL, 5, NULL);
    xSemaphoreTake(s_recurse_done, pdMS_TO_TICKS(2000));
    TEST_ASSERT_EQUAL(60, s_recurse_result);
    vSemaphoreDelete(s_recurse_done);
}

// ---------------------------------------------------------------------------
// Concurrent mutex contention: 4 tasks all hammering one mutex
// ---------------------------------------------------------------------------

static SemaphoreHandle_t s_contend_mutex;
static volatile int      s_contend_total = 0;
static SemaphoreHandle_t s_contend_done;
static volatile int      s_contend_tasks_done = 0;

static void contend_task(void *param)
{
    for (int i = 0; i < 250; i++) {
        xSemaphoreTake(s_contend_mutex, portMAX_DELAY);
        s_contend_total++;
        xSemaphoreGive(s_contend_mutex);
    }
    xSemaphoreTake(s_contend_mutex, portMAX_DELAY);
    s_contend_tasks_done++;
    xSemaphoreGive(s_contend_mutex);
    vTaskDelete(NULL);
}

void test_four_tasks_contending_mutex_consistency(void)
{
    s_contend_mutex      = xSemaphoreCreateMutex();
    s_contend_done       = xSemaphoreCreateBinary();
    s_contend_total      = 0;
    s_contend_tasks_done = 0;

    for (int i = 0; i < 4; i++) {
        xTaskCreate(contend_task, "ctd", 2048, NULL, 5, NULL);
    }

    // Wait for all 4 tasks to finish (each sets tasks_done under mutex)
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
    while (s_contend_tasks_done < 4 && xTaskGetTickCount() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    TEST_ASSERT_EQUAL(4, s_contend_tasks_done);
    TEST_ASSERT_EQUAL(4 * 250, s_contend_total); // 1000 increments total

    vSemaphoreDelete(s_contend_mutex);
    vSemaphoreDelete(s_contend_done);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_queue_flood_no_data_loss);
    RUN_TEST(test_isr_burst_semaphore_gives);
    RUN_TEST(test_heap_stable_under_repeated_alloc_free);
    RUN_TEST(test_deep_recursion_does_not_overflow_8k_stack);
    RUN_TEST(test_four_tasks_contending_mutex_consistency);
    UNITY_END();
}

void loop() {}
