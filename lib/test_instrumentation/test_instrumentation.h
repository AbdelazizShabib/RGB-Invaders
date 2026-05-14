#pragma once

#ifdef TEST_BUILD

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Timing capture — microsecond resolution via esp_timer_get_time()
// ---------------------------------------------------------------------------

typedef struct {
    int64_t  start_us;
    int64_t  end_us;
    int64_t  elapsed_us;
    uint32_t sample_count;
    int64_t  min_us;
    int64_t  max_us;
    int64_t  sum_us;
} TimingCapture;

void     timing_start(TimingCapture *tc);
void     timing_stop(TimingCapture *tc);
int64_t  timing_elapsed_us(const TimingCapture *tc);
void     timing_reset(TimingCapture *tc);

// ---------------------------------------------------------------------------
// Stack watermark snapshot — wraps uxTaskGetStackHighWaterMark()
// ---------------------------------------------------------------------------

typedef struct {
    const char *task_name;
    uint32_t    watermark_words; // free stack words remaining (high watermark)
    uint32_t    baseline_words;  // value recorded at snapshot start
} StackSnapshot;

// Pass NULL for task_handle to query the calling task.
void stack_snapshot_take(StackSnapshot *ss, const char *name, void *task_handle);

// Returns number of words consumed since baseline (negative = watermark went up).
int32_t stack_words_consumed(const StackSnapshot *ss);

// ---------------------------------------------------------------------------
// Runtime counters — simple atomic-safe event counters
// ---------------------------------------------------------------------------

typedef struct {
    volatile uint32_t value;
    const char *name;
} RuntimeCounter;

void     counter_reset(RuntimeCounter *c);
void     counter_increment(RuntimeCounter *c);
uint32_t counter_get(const RuntimeCounter *c);

// ---------------------------------------------------------------------------
// Latency histogram — bucket measurements into coarse bins
// ---------------------------------------------------------------------------

#define HIST_BUCKETS 8

typedef struct {
    uint32_t counts[HIST_BUCKETS];
    int64_t  bounds_us[HIST_BUCKETS]; // upper bound for each bucket (last = catch-all)
    uint32_t overflow;                // samples exceeding all bounds
} LatencyHistogram;

// bounds_us must be HIST_BUCKETS-1 ascending values; last bucket catches remainder.
void hist_init(LatencyHistogram *h, const int64_t bounds_us[], size_t num_bounds);
void hist_record(LatencyHistogram *h, int64_t elapsed_us);
void hist_print(const LatencyHistogram *h, const char *label);

// ---------------------------------------------------------------------------
// Heap guard — snapshot free heap before/after to detect leaks
// ---------------------------------------------------------------------------

typedef struct {
    size_t before;
    size_t after;
    int32_t delta; // positive = grew (leak), negative = freed
} HeapGuard;

void heap_guard_start(HeapGuard *g);
void heap_guard_stop(HeapGuard *g);

#ifdef __cplusplus
}
#endif

#endif // TEST_BUILD
