#include "input/queue.h"

#include <zephyr.h>

#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(queue);

#if defined(CONFIG_PASSINGLINK_INPUT_QUEUE)

static constexpr size_t queue_storage_size = 1024;
static size_t queue_storage_last_avail = 0;
static Bitmap<queue_storage_size> queue_storage_bitmap;
static InputQueue queue_storage[queue_storage_size];

static RawInputState queue_input;

static InputQueue* queue_next;
static InputQueue* queue_next_free_head;

static int64_t queue_next_tick;

InputQueue* input_queue_alloc() {
  ScopedIRQLock lock;
  InputQueue* result = nullptr;
  for (size_t i = queue_storage_last_avail; i < queue_storage_size; ++i) {
    auto bit = queue_storage_bitmap[i];
    if (!bit) {
      bit = true;
      queue_storage_last_avail = i + 1;
      result = &queue_storage[i];
      result->next = nullptr;
      break;
    }
  }

  // TODO: Handle allocation failure.
  return result;
}

InputQueue* input_queue_append(InputQueue* head) {
  InputQueue* tail = input_queue_alloc();
  if (!tail) {
    return nullptr;
  }

  head->next = tail;
  tail->next = nullptr;
  return tail;
}

void input_queue_free(InputQueue* p) {
  if (!p) return;

  ScopedIRQLock lock;

  do {
    ptrdiff_t offset = p - queue_storage;
    assert(offset >= 0);
    assert(static_cast<size_t>(offset) < queue_storage_size);
    p = p->next;

    queue_storage_bitmap[offset] = false;
    if (static_cast<size_t>(offset) < queue_storage_last_avail) {
      queue_storage_last_avail = offset;
    }
  } while (p);
}

optional<RawInputState> input_queue_get_state() {
  ScopedIRQLock lock;

  if (queue_next) {
    if (queue_next_tick < k_uptime_ticks()) {
      queue_input = queue_next->state;

      // TODO: k_timeout_t is supposed to be an opaque struct.
      queue_next_tick += queue_next->delay.ticks;

      InputQueue* prev = queue_next;
      queue_next = queue_next->next;

      if (!queue_next) {
        input_queue_free(queue_next_free_head);
        queue_next_free_head = nullptr;
      }
    }
    return queue_input;
  }
  return {};
}

void input_queue_set_active(InputQueue* queue, bool consume) {
  ScopedIRQLock lock;

  input_queue_free(queue_next_free_head);
  queue_next_free_head = nullptr;

  queue_next = queue;
  queue_next_tick = k_uptime_ticks();
  if (consume) {
    queue_next_free_head = queue;
  }
}

#endif
