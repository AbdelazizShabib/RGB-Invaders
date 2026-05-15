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
    Serial.printf("  xQueueCreate(length=4, itemSize=4) = %s\n", q ? "non-NULL (OK)" : "NULL (FAIL)");
    TEST_ASSERT_NOT_NULL(q);
    vQueueDelete(q);
}

void test_queue_initial_count_is_zero(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    UBaseType_t count = uxQueueMessagesWaiting(q);
    Serial.printf("  new queue: messagesWaiting=%u  (expected 0)\n", count);
    TEST_ASSERT_EQUAL(0, count);
    vQueueDelete(q);
}

void test_queue_initial_spaces_equals_length(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    UBaseType_t spaces = uxQueueSpacesAvailable(q);
    Serial.printf("  new queue(capacity=4): spacesAvailable=%u\n", spaces);
    TEST_ASSERT_EQUAL(4, spaces);
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
    xQueueSend(q, &sent, 0);
    xQueueReceive(q, &recv, 0);
    Serial.printf("  sent=0x%08X  received=0x%08X  match=%s\n",
                  sent, recv, sent == recv ? "YES" : "NO");
    TEST_ASSERT_EQUAL_UINT32(sent, recv);
    vQueueDelete(q);
}

void test_queue_count_increments_after_send(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t v = 1;
    xQueueSend(q, &v, 0);
    UBaseType_t count = uxQueueMessagesWaiting(q);
    Serial.printf("  after 1 send: messagesWaiting=%u  (expected 1)\n", count);
    TEST_ASSERT_EQUAL(1, count);
    vQueueDelete(q);
}

void test_queue_count_decrements_after_receive(void)
{
    QueueHandle_t q = make_queue(4, sizeof(uint32_t));
    uint32_t v = 1;
    xQueueSend(q, &v, 0);
    xQueueReceive(q, &v, 0);
    UBaseType_t count = uxQueueMessagesWaiting(q);
    Serial.printf("  after send+receive: messagesWaiting=%u  (expected 0)\n", count);
    TEST_ASSERT_EQUAL(0, count);
    vQueueDelete(q);
}

// ---------------------------------------------------------------------------
// FIFO ordering
// ---------------------------------------------------------------------------

void test_queue_fifo_ordering(void)
{
    QueueHandle_t q = make_queue(8, sizeof(uint32_t));
    for (uint32_t i = 0; i < 5; i++) xQueueSend(q, &i, 0);
    Serial.printf("  FIFO order check (sent 0-4):");
    bool ok = true;
    for (uint32_t i = 0; i < 5; i++) {
        uint32_t recv;
        xQueueReceive(q, &recv, 0);
        Serial.printf(" %u", recv);
        if (recv != i) ok = false;
    }
    Serial.printf("  -> %s\n", ok ? "ORDERED" : "MISORDERED");
    vQueueDelete(q);

    // Re-run for assertion
    q = make_queue(8, sizeof(uint32_t));
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
    BaseType_t result = xQueueSend(q, &v, 0);
    UBaseType_t waiting = uxQueueMessagesWaiting(q);
    Serial.printf("  queue(cap=2) full: 3rd send result=%s  messagesWaiting=%u\n",
                  result == errQUEUE_FULL ? "errQUEUE_FULL" : "unexpected", waiting);
    TEST_ASSERT_EQUAL(errQUEUE_FULL, result);
    vQueueDelete(q);
}

void test_queue_receive_from_empty_fails_immediately(void)
{
    QueueHandle_t q = make_queue(2, sizeof(uint32_t));
    uint32_t v;
    BaseType_t result = xQueueReceive(q, &v, 0);
    Serial.printf("  receive from empty queue: result=%s\n",
                  result == pdFALSE ? "pdFALSE (OK)" : "unexpected");
    TEST_ASSERT_EQUAL(pdFALSE, result);
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
    UBaseType_t spaces_before = uxQueueSpacesAvailable(q);
    Serial.printf("  filled queue(cap=%d): spacesAvailable=%u  (expected 0)\n", DEPTH, spaces_before);
    for (int i = 0; i < DEPTH; i++) {
        int v;
        xQueueReceive(q, &v, 0);
        TEST_ASSERT_EQUAL(i, v);
    }
    UBaseType_t waiting_after = uxQueueMessagesWaiting(q);
    Serial.printf("  drained %d items: messagesWaiting=%u\n", DEPTH, waiting_after);
    TEST_ASSERT_EQUAL(0, spaces_before);
    TEST_ASSERT_EQUAL(0, waiting_after);
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
    UBaseType_t before = uxQueueMessagesWaiting(q);
    xQueueReset(q);
    UBaseType_t after = uxQueueMessagesWaiting(q);
    Serial.printf("  before reset: messagesWaiting=%u  after reset: messagesWaiting=%u\n",
                  before, after);
    TEST_ASSERT_EQUAL(0, after);
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
    Serial.printf("  sent a=%u then SendToFront b=%u: first dequeued=%u  (expected b=%u)\n",
                  a, b, recv, b);
    TEST_ASSERT_EQUAL_UINT32(b, recv);
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
    UBaseType_t still_waiting = uxQueueMessagesWaiting(q);
    xQueueReceive(q, &recvd, 0);
    Serial.printf("  sent=0x%04X  peeked=0x%04X  stillWaiting=%u  received=0x%04X\n",
                  sent, peeked, still_waiting, recvd);
    TEST_ASSERT_EQUAL_UINT32(sent, peeked);
    TEST_ASSERT_EQUAL(1, still_waiting);
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
    Serial.printf("  cross-task delivery: received=0x%08X  result=%s\n",
                  recv, ok == pdTRUE ? "pdTRUE" : "TIMEOUT");
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
    Serial.println("\n=== RTOS Queue Tests ===");
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
