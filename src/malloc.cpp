#if defined(CONFIG_PASSINGLINK_ALLOCATOR)
#include <zephyr.h>

#include <logging/log.h>
#include <types.h>

#include <string.h>

#include "types.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(malloc);

#define ALLOC_HWM 0

extern "C" void dump_allocator_hwm();

template <size_t Bits>
struct Bitset {
  static constexpr size_t Bytes = (Bits + 7) / 8;

  bool get(size_t idx) {
    size_t byte_idx = idx / 8;
    size_t bit_idx = idx % 8;
    u8_t byte = data_[byte_idx];
    return byte & (1 << bit_idx);
  }

  void set(size_t idx, bool value) {
    size_t byte_idx = idx / 8;
    size_t bit_idx = idx % 8;
    u8_t byte = data_[byte_idx];
    if (value) {
      byte |= 1 << bit_idx;
    } else {
      byte &= ~(1 << bit_idx);
    }
    data_[byte_idx] = byte;
  }

  optional<size_t> find_first_unset() {
    for (size_t i = 0; i < Bits; ++i) {
      if (!get(i)) {
        return i;
      }
    }
    return {};
  }

  u8_t data_[Bytes] = {};
};

template <size_t Size, size_t Count>
struct Bucket {
  struct Block {
    alignas(int) char bytes[Size];
  };

  void* alloc(size_t size) {
    optional<size_t> available = used_.find_first_unset();
    if (!available) {
      LOG_ERR("Bucket<%zu>::alloc(%zu) unavailable", Size, size);
      errno = ENOMEM;
      return nullptr;
    }

    size_t idx = *available;
    used_.set(idx, true);
    LOG_DBG("Bucket<%zu>::alloc(%zu) = %p", Size, size, blocks_[idx].bytes);
    update_hwm(1);
    return blocks_[idx].bytes;
  }

  void free(void* ptr) {
    size_t idx = static_cast<Block*>(ptr) - blocks_;
    used_.set(idx, false);
    update_hwm(-1);
  }

#if ALLOC_HWM
  void update_hwm(int sign) {
    current += sign;
    if (current > hwm) {
      hwm = current;
    }
  }

  void dump_hwm() { LOG_WRN("Bucket<%zu> hwm = %zu", Size, hwm); }
#else
  void update_hwm(int sign) {}
  void dump_hwm() {}
#endif

  Block blocks_[Count];
  Bitset<Count> used_;

#if ALLOC_HWM
  size_t current;
  size_t hwm;
#endif
};

#define BUCKETS() \
  BUCKET(264, 35) \
  BUCKET(528, 1)

struct Allocator {
#define BUCKET(block_size, count) Bucket<block_size, count> bucket_##block_size;
  BUCKETS()
#undef BUCKET

  void* malloc(size_t size) {
    // clang-format off
#define BUCKET(block_size, count)               \
    if (size <= block_size) {                   \
      return (bucket_##block_size).alloc(size); \
    }
    BUCKETS()
#undef BUCKET
    // clang-format on

    errno = ENOMEM;
    return nullptr;
  }

  void free(void* ptr) {
    // clang-format off
#define BUCKET(block_size, count)              \
    {                                          \
      auto* bucket = &bucket_##block_size;     \
      auto* bucket_end = bucket + 1;           \
      if (ptr >= bucket && ptr < bucket_end) { \
        return bucket->free(ptr);              \
      }                                        \
    }
    BUCKETS()
#undef BUCKET
    // clang-format on

    return;
  }

  void dump_hwm() {
#define BUCKET(block_size, count) bucket_##block_size.dump_hwm();
    BUCKETS()
#undef BUCKETS
  }
};

static Allocator allocator;

extern "C" void* malloc(size_t size) {
  return allocator.malloc(size);
}

extern "C" void free(void* ptr) {
  allocator.free(ptr);
}

extern "C" void dump_allocator_hwm() {
  allocator.dump_hwm();
}

#else

extern "C" void dump_allocator_hwm() {}

#endif  // defined(CONFIG_PASSINGLINK_ALLOCATOR)
