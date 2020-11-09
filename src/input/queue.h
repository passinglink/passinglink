#pragma once

#if defined(CONFIG_PASSINGLINK_INPUT_QUEUE)

#include "input/input.h"
#include "types.h"

struct InputQueue {
  // State to set inputs to when resolving queue.
  RawInputState state;

  // Delay until the next element of the queue.
  k_timeout_t delay;

  // Next entry in the queue, or nullptr.
  // The head of the queue owns the tail: freeing a node will free everything following it.
  InputQueue* next;
};

InputQueue* input_queue_alloc();
InputQueue* input_queue_alloc_autofree();

void input_queue_free(InputQueue* p);

// Allocate a new InputQueue and append it to head.
// The new node inherits autofree state from the head.
InputQueue* input_queue_append(InputQueue* head);

optional<RawInputState> input_queue_get_state();

// Set the currently active InputQueue.
// If consume is true, it will be freed after completion.
void input_queue_set_active(InputQueue* queue, bool consume);

#endif
