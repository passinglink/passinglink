#include "metrics/metrics.h"

#include <zephyr.h>

#include "arch.h"
#include "display/display.h"
#include "types.h"

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(metrics);

#if !defined(CONFIG_PASSINGLINK_DISPLAY)
void metrics_record_input_read() {}
void metrics_record_usb_write() {}
#else

constexpr uint64_t REPORT_INTERVAL = 1024;
optional<uint32_t> input_cycle;

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

  size_t reports() { return reports_; }

  size_t reports_ = 0;
  optional<T> average_;
};

#if defined(__VFP_FP__)
moving_average<float, 2, 2048> averager;
#else
moving_average<uint64_t, 2, 2048> averager;
#endif

void metrics_record_input_read() {
  input_cycle = get_cycle_count();
}

void metrics_record_usb_write() {
  if (input_cycle) {
    uint32_t diff = get_cycle_count() - *input_cycle;
    input_cycle = {};
    averager.add(diff);
    if (averager.reports() % REPORT_INTERVAL == 0) {
      display_update_latency(static_cast<uint64_t>(averager.get()) * 1'000'000 / get_cpu_freq());
    }
  }
}

#endif
