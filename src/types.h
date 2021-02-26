#pragma once

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <sys/atomic.h>

#include <zephyr.h>

#include "panic.h"

// Placement new overload that normally comes from <new>.
inline void* operator new(size_t size, void* ptr) {
  return ptr;
}

struct ScopedIRQLock {
  ScopedIRQLock() { irq_lock_ = irq_lock(); }
  ~ScopedIRQLock() { irq_unlock(irq_lock_); }

  ScopedIRQLock(const ScopedIRQLock& copy) = delete;
  ScopedIRQLock(ScopedIRQLock&& move) = delete;

  uint64_t irq_lock_;
};

template <typename T>
struct atomic_u32 {
  static_assert(alignof(T) <= alignof(atomic_t));
  static_assert(sizeof(T) == 4);
  static_assert(__is_trivially_copyable(T));
  static_assert(__has_trivial_destructor(T));

  atomic_u32() : value_(0) {}
  explicit atomic_u32(T t) : value_(to_int(t)) {}

  T load() { return from_int(atomic_get(&value_)); }
  void store(T value) { atomic_set(&value_, to_int(value)); }

  bool cas(T expected, T new_value) {
    // TODO: Is this weak or strong?
    return atomic_cas(&value_, to_int(expected), to_int(new_value));
  }

 private:
  static uint32_t to_int(T x) {
    uint32_t result;
    memcpy(&result, &x, sizeof(T));
    return result;
  }

  static T from_int(uint32_t x) {
    T result;
    memcpy(&result, &x, sizeof(T));
    return result;
  }

  atomic_t value_ = 0;
};

template <typename T>
struct optional {
  optional() {}

  optional(const optional& copy) {
    if (copy.get()) {
      reset(*copy);
    }
  }

  optional(optional&& move) {
    if (move.get()) {
      reset(static_cast<T&&>(*move));
    }
    move.reset();
  }

  optional(T t) { reset(static_cast<T&&>(t)); }
  ~optional() { reset(); }

  optional& operator=(const optional& copy) {
    if (copy) {
      reset(*copy);
    } else {
      reset();
    }
    return *this;
  }

  operator bool() const { return valid_; }

  T* get() {
    if (!valid_) return nullptr;
    return ptr();
  }

  const T* get() const {
    if (!valid_) return nullptr;
    return ptr();
  }

  T* operator->() { return get(); }
  const T* operator->() const { return get(); }

  T& operator*() { return *get(); }
  const T& operator*() const { return *get(); }

  void reset() {
    if (valid_) {
      get()->~T();
      valid_ = false;
    }
  }

  void reset(T value) {
    reset();
    new (obj_) T{static_cast<T&&>(value)};
    valid_ = true;
  }

 private:
  T* ptr() { return reinterpret_cast<T*>(&obj_); }
  const T* ptr() const { return reinterpret_cast<const T*>(&obj_); }

  bool valid_ = false;
  alignas(T) char obj_[sizeof(T)];
};

template <typename T, size_t N>
struct array {
  T contents[N];

  constexpr size_t size() const { return N; }

  T* data() { return contents; }
  const T* data() const { return contents; }

  T& operator*() { return *contents; }
  const T& operator*() const { return *contents; }

  T& operator->() { return *contents; }
  const T& operator->() const { return *contents; }

  T& operator[](size_t idx) { return contents[idx]; }
  const T& operator[](size_t idx) const { return contents[idx]; }
};

template <typename T>
struct span {
  span() : span(nullptr, 0) {}
  span(T* ptr, size_t length) : ptr_(ptr), length_(length) {}
  span(const span& copy) = default;
  span(span&& move) = default;

  template <size_t Length>
  explicit span(const T (&array)[Length]) : span(&array, Length) {}

  T& operator[](size_t offset) { return data()[offset]; }
  T* data() { return ptr_; }
  const T* data() const { return ptr_; }
  size_t size() const { return length_; }

  bool empty() const { return size() == 0; }

  void remove_prefix(size_t n) {
    assert(length_ >= n);
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_t n) {
    assert(length_ >= n);
    length_ -= n;
  }

 private:
  T* ptr_;
  size_t length_;
};

template <size_t Length>
struct Bitmap {
  struct BitmapProxy {
    BitmapProxy(uint8_t* p, uint8_t offset) {
      p_ = p;
      offset_ = offset;
    }

    operator bool() { return *p_ & 1 << offset_; }

    BitmapProxy& operator=(bool rhs) {
      if (rhs) {
        *p_ |= 1 << offset_;
      } else {
        *p_ &= ~(1 << offset_);
      }
      return *this;
    }

   private:
    uint8_t* p_;
    uint8_t offset_;
  };

  size_t size() { return Length; }
  BitmapProxy operator[](size_t i) { return BitmapProxy(&data_[i / 8], i % 8); }

 private:
  uint8_t data_[Length / 8];
};

template <typename T>
void swap(T& a, T& b) {
  auto tmp = a;
  a = b;
  b = tmp;
}

template <typename Iterator, typename Comparator>
Iterator min(Iterator begin, Iterator end, Comparator cmp) {
  Iterator min = begin;
  for (auto it = begin; it != end; ++it) {
    if (cmp(*min, *it)) {
      min = it;
    }
  }

  return min;
}

template <typename Iterator, typename Comparator>
void insertion_sort(Iterator begin, Iterator end, Comparator cmp) {
  while (begin != end) {
    Iterator min_it = min(begin, end, cmp);
    if (min_it == begin) {
      ++begin;
      continue;
    }

    swap(*min_it, *begin++);
  }
}

#define SAMPLING_LOG(freq, ...) \
  ({                            \
    static int counter = 0;     \
    if (++counter == (freq)) {  \
      counter = 0;              \
      LOG_INF(__VA_ARGS__);     \
    }                           \
  })
