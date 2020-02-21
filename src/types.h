#pragma once

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <sys/atomic.h>

// Placement new overload that normally come from <new>.
inline void* operator new(size_t size, void* ptr) {
  return ptr;
}

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
  static u32_t to_int(T x) {
    u32_t result;
    memcpy(&result, &x, sizeof(T));
    return result;
  }

  static T from_int(u32_t x) {
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

  void remove_prefix(size_t n) {
    assert(length_ > n);
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_t n) {
    assert(length_ > n);
    length_ -= n;
  }

 private:
  T* ptr_;
  size_t length_;
};

#define SAMPLING_LOG(freq, ...) \
  ({                            \
    static int counter = 0;     \
    if (++counter == (freq)) {  \
      counter = 0;              \
      LOG_INF(__VA_ARGS__);     \
    }                           \
  })
