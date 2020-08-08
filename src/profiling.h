#pragma once

#ifndef LOG_LEVEL
#error LOG_LEVEL must be defined before including profiling.h.
#endif

#include "arch.h"

#if defined(CONFIG_PASSINGLINK_PROFILING)
template<int Frequency = 1>
struct Profiler {
  void begin() {
    begin_cycle_ = get_cycle_count();
  }

  void end(const char* name) {
    if (times_count_ != Frequency) {
      uint32_t end_cycle = get_cycle_count();
      uint32_t diff = end_cycle - begin_cycle_;
      times_[times_count_++] = diff;
    } else {
      times_count_ = 0;
      u64_t total = 0;
      uint32_t min = UINT32_MAX;
      uint32_t max = 0;
      for (auto time : times_) {
        total += time;
        if (time < min) {
          min = time;
        }
        if (time > max) {
          max = time;
        }
      }

      uint32_t average = total / (Frequency - 1);
      LOG_INF("%s: avg = %u, min = %u, max = %u", name, average, min, max);
    }
  }

  uint32_t times_[Frequency - 1];
  size_t times_count_;

  uint32_t begin_cycle_;
};

template<int Frequency>
struct ScopedProfile {
  explicit ScopedProfile(Profiler<Frequency>& profiler, const char* name)
      : profiler_(profiler), name_(name) {
    profiler_.begin();
  }

  ~ScopedProfile() {
    profiler_.end(name_);
  }

  Profiler<Frequency>& profiler_;
  const char* name_;
};
#else
template<int Frequency>
struct Profiler {};

template<int Frequency>
struct ScopedProfile {
  explicit ScopedProfile(Profiler<Frequency>& profiler, const char* name) {}
};
#endif

#define PROFILE(name, frequency)         \
  static Profiler<frequency> __profiler; \
  ScopedProfile<frequency> __scoped_profiler(__profiler, (name))
