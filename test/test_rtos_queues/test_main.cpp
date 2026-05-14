#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QueueHandle_t make_queue(UBaseType_t length, UBaseType_t item_size)
{
    return xQueueCreate(length, item_size);
}

// ---------------------------------------------------------------------------
// Creation
// ---------------------------------------------------------------------------

void test_queue_create_returns_non_null(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(q);
    vQueueDelete(q);
}

void test_queue_initial_count_is_zero(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(q));
    vQueueDelete(q);
}

void test_queue_initial_spaces_equals_length(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    TEST_ASSERT_EQUAL(4, uxQueueSpacesAvailable(q));
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// Single send / receive
// ---------------------------------------------------------------------------

void test_queue_send_and_receive_single_item(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t sent = 0xDEADBEEFu;
    uint32_t recv = 0;
    TEST_ASSERT_EQUAL(pdTRUE, xQueueSend(q, &sent, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(q, &recv, 0));
    TEST_ASSERT_EQUAL_UINT32(sent, recv);
    vQueueDelete(q);
}

void test_queue_count_increments_after_send(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t v = 1;
    xQueueSend(q, &v, 0);
    TEST_ASSERT_EQUAL(1, uxQueueMessagesWaiting(q));
    vQueueDelete(q);
}

void test_queue_count_decrements_after_receive(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t v = 1;
    xQueueSend(q, &v, 0);
    xQueueReceive(q, &v, 0);
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(q));
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// FIFO ordering
// ---------------------------------------------------------------------------

void test_queue_fifo_ordering(void)
{
    QueueHandle_t q = make_queue(8, sizeof(uint32_t));
    for (uint32_t i = 0; i < 5; i++) xQueueSend(q, &i, 0);
    for (uint32_t i = 0; i < 5; i++) {
        uint32_t recv;
        xQueueReceive(q, &recv, 0);
        TEST_ASSERT_EQUAL_UINT32(i, recv);
    }
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// Overflow — send to full queue without blocking
// ---------------------------------------------------------------------------

void test_queue_send_to_full_fails_immediately(void)
{
    QueueHandle_t q = make_queue(2, sizeof(uint32_t));
    uint32_t v = 0;
    xQueueSend(q, &v, 0);
    xQueueSend(q, &v, 0);
    // Queue is now full — next send must fail with zero timeout
    TEST_ASSERT_EQUAL(errQUEUE_FULL, xQueueSend(q, &v, 0));
    vQueueDelete(q);
}

void test_queue_receive_from_empty_fails_immediately(void)
{
    QueueHandle_t q = make_queue(2, sizeof(uint32_t));
    uint32_t v;
    TEST_ASSERT_EQUAL(pdFALSE, xQueueReceive(q, &v, 0));
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// Drain — fill then empty
// ---------------------------------------------------------------------------

void test_queue_fill_then_drain(void)
{
    const int DEPTH = 8;
    QueueHandle_t q = make_queue(DEPTH, sizeof(int));
    for (int i = 0; i < DEPTH; i++) xQueueSend(q, &i, 0);
    TEST_ASSERT_EQUAL(0, uxQueueSpacesAvailable(q));
    for (int i = 0; i < DEPTH; i++) {
        int v;
        xQueueReceive(q, &v, 0);
        TEST_ASSERT_EQUAL(i, v);
    }
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(q));
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void test_queue_reset_clears_all_items(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t v = 42;
    xQueueSend(q, &v, 0);
    xQueueSend(q, &v, 0);
    xQueueReset(q);
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(q));
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// xQueueSendToFront
// ---------------------------------------------------------------------------

void test_queue_send_to_front_inserts_at_head(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t a = 10, b = 20;
    xQueueSend(q, &a, 0);
    xQueueSendToFront(q, &b, 0);
    uint32_t recv;
    xQueueReceive(q, &recv, 0);
    TEST_ASSERT_EQUAL_UINT32(b, recv); // b should come out first
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// xQueuePeek — data not consumed
// ---------------------------------------------------------------------------

void test_queue_peek_does_not_remove_item(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t sent = 0xCAFEu, peeked = 0, recvd = 0;
    xQueueSend(q, &sent, 0);
    xQueuePeek(q, &peeked, 0);
    TEST_ASSERT_EQUAL_UINT32(sent, peeked);
    TEST_ASSERT_EQUAL(1, uxQueueMessagesWaiting(q)); // still there
    xQueueReceive(q, &recvd, 0);
    TEST_ASSERT_EQUAL_UINT32(sent, recvd);
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// Cross-task: producer task sends, main task receives
// ---------------------------------------------------------------------------

static QueueHandle_t s_cross_q;

static void producer_task(void *param)
{
    uint32_t val = 0xABCD1234u;
    xQueueSend(s_cross_q, &val, pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}

void test_cross_task_queue_delivery(void)
{
    s_cross_q = make_queue(2, sizeof(uint32_t));
    xTaskCreate(producer_task, "prod", 2048, NULL, 5, NULL);
    uint32_t recv = 0;
    BaseType_t ok = xQueueReceive(s_cross_q, &recv, pdMS_TO_TICKS(200));
    TEST_ASSERT_EQUAL(pdTRUE, ok);
    TEST_ASSERT_EQUAL_UINT32(0xABCD1234u, recv);
    vQueueDelete(s_cross_q);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_queue_create_returns_non_null);
    RUN_TEST(test_queue_initial_count_is_zero);
    RUN_TEST(test_queue_initial_spaces_equals_length);
    RUN_TEST(test_queue_send_and_receive_single_item);
    RUN_TEST(test_queue_count_increments_after_send);
    RUN_TEST(test_queue_count_decrements_after_receive);
    RUN_TEST(test_queue_fifo_ordering);
    RUN_TEST(test_queue_send_to_full_fails_immediately);
    RUN_TEST(test_queue_receive_from_empty_fails_immediately);
    RUN_TEST(test_queue_fill_then_drain);
    RUN_TEST(test_queue_reset_clears_all_items);
    RUN_TEST(test_queue_send_to_front_inserts_at_head);
    RUN_TEST(test_queue_peek_does_not_remove_item);
    RUN_TEST(test_cross_task_queue_delivery);
    UNITY_END();
}

void loop() {}
