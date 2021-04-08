#include "metrics/metrics.h"

#include <zephyr.h>

#include "arch.h"
#include "display/display.h"
#include "types.h"

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(metrics);

#if !defined(CONFIG_PASSINGLINK_DISPLAY)
void metrics_reset() {}
void metrics_record_input_read() {}
void metrics_record_usb_write() {}
#else

constexpr uint64_t REPORT_INTERVAL = 1024;
optional<uint32_t> input_tick;

template <typename T, size_t alpha_num, size_t alpha_denom>
struct moving_average {
  static_assert(alpha_denom > alpha_num);

  void add(T value) {
    if (!average_) {
      average_ = value;
    } else {
      constexpr T beta = alpha_denom - alpha_num;
      average_ = (beta * (*average_) + alpha_num * value) / alpha_denom;
    }
    ++reports_;
  }

  T get() { return *average_; }

  void reset() {
    reports_ = 0;
    average_.reset();
  }

  size_t reports() { return reports_; }

  size_t reports_ = 0;
  optional<T> average_;
};

#if defined(__VFP_FP__)
moving_average<float, 2, 2048> averager;
#else
moving_average<uint64_t, 2, 2048> averager;
#endif

void metrics_reset() {
  ScopedIRQLock lock;
  averager.reset();
  input_tick.reset();
}

void metrics_record_input_read() {
  if (!input_tick) {
    input_tick = k_uptime_ticks();
  }
}

void metrics_record_usb_write() {
  if (input_tick) {
    uint32_t diff = k_uptime_ticks() - *input_tick;
    input_tick = {};
    averager.add(diff);
    if (averager.reports() % REPORT_INTERVAL == 0) {
      display_update_latency(k_ticks_to_us_ceil32(1) * averager.get());
    }
  }
}

#endif
