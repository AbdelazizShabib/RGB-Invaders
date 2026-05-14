#ifdef TEST_BUILD

#include "test_instrumentation.h"

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <Arduino.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

void timing_start(TimingCapture *tc)
{
    tc->start_us = esp_timer_get_time();
    tc->end_us   = 0;
    tc->elapsed_us = 0;
}

void timing_stop(TimingCapture *tc)
{
    tc->end_us     = esp_timer_get_time();
    tc->elapsed_us = tc->end_us - tc->start_us;
    tc->sum_us    += tc->elapsed_us;
    tc->sample_count++;
    if (tc->sample_count == 1 || tc->elapsed_us < tc->min_us) tc->min_us = tc->elapsed_us;
    if (tc->elapsed_us > tc->max_us) tc->max_us = tc->elapsed_us;
}

int64_t timing_elapsed_us(const TimingCapture *tc)
{
    return tc->elapsed_us;
}

void timing_reset(TimingCapture *tc)
{
    memset(tc, 0, sizeof(TimingCapture));
}

// ---------------------------------------------------------------------------
// Stack watermark
// ---------------------------------------------------------------------------

void stack_snapshot_take(StackSnapshot *ss, const char *name, void *task_handle)
{
    ss->task_name      = name;
    ss->watermark_words = uxTaskGetStackHighWaterMark((TaskHandle_t)task_handle);
    if (ss->baseline_words == 0) {
        ss->baseline_words = ss->watermark_words;
    }
}

int32_t stack_words_consumed(const StackSnapshot *ss)
{
    // baseline_words was the HIGH watermark (free words) before; after running,
    // a lower watermark means more was consumed.
    return (int32_t)ss->baseline_words - (int32_t)ss->watermark_words;
}

// ---------------------------------------------------------------------------
// Runtime counters
// ---------------------------------------------------------------------------

void counter_reset(RuntimeCounter *c)
{
    c->value = 0;
}

void counter_increment(RuntimeCounter *c)
{
    // portENTER_CRITICAL is overkill here for a test counter; use atomic add.
    __atomic_add_fetch(&c->value, 1, __ATOMIC_RELAXED);
}

uint32_t counter_get(const RuntimeCounter *c)
{
    return __atomic_load_n(&c->value, __ATOMIC_RELAXED);
}

// ---------------------------------------------------------------------------
// Latency histogram
// ---------------------------------------------------------------------------

void hist_init(LatencyHistogram *h, const int64_t bounds_us[], size_t num_bounds)
{
    memset(h, 0, sizeof(LatencyHistogram));
    size_t n = num_bounds < (HIST_BUCKETS - 1) ? num_bounds : (HIST_BUCKETS - 1);
    for (size_t i = 0; i < n; i++) {
        h->bounds_us[i] = bounds_us[i];
    }
    h->bounds_us[HIST_BUCKETS - 1] = INT64_MAX;
}

void hist_record(LatencyHistogram *h, int64_t elapsed_us)
{
    for (int i = 0; i < HIST_BUCKETS; i++) {
        if (elapsed_us <= h->bounds_us[i]) {
            h->counts[i]++;
            return;
        }
    }
    h->overflow++;
}

void hist_print(const LatencyHistogram *h, const char *label)
{
    Serial.printf("[HIST] %s\n", label);
    for (int i = 0; i < HIST_BUCKETS; i++) {
        if (h->bounds_us[i] == INT64_MAX) {
            Serial.printf("  [>prev] : %u\n", (unsigned)h->counts[i]);
        } else {
            Serial.printf("  [<=%lld us]: %u\n", (long long)h->bounds_us[i], (unsigned)h->counts[i]);
        }
    }
    if (h->overflow) {
        Serial.printf("  [overflow]: %u\n", (unsigned)h->overflow);
    }
}

// ---------------------------------------------------------------------------
// Heap guard
// ---------------------------------------------------------------------------

void heap_guard_start(HeapGuard *g)
{
    g->before = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    g->after  = 0;
    g->delta  = 0;
}

void heap_guard_stop(HeapGuard *g)
{
    g->after = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    g->delta = (int32_t)g->before - (int32_t)g->after; // positive = consumed
}

#endif // TEST_BUILD
