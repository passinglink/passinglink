#include <zephyr.h>

#include <shell/shell.h>

#include "input/input.h"

#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)

static RawInputState input_state;

struct InputQueue {
  RawInputState state;

  // Delay until the next element of the queue.
  k_timeout_t delay;

  InputQueue* next;
};

static InputQueue queue_storage[128];
static InputQueue* queue_next;
static int64_t queue_next_tick;

static void input_state_reset() {
  memset(&input_state, 0, sizeof(input_state));
}

static void input_queue_advance() {
  if (queue_next && queue_next_tick < k_uptime_ticks()) {
    input_state = queue_next->state;
    queue_next_tick = z_timeout_end_calc(queue_next->delay);
    queue_next = queue_next->next;
  }
}

bool input_get_raw_state(RawInputState* out) {
  input_queue_advance();

  *out = input_state;
  return true;
}

static int cmd_input_clear(const struct shell* shell, size_t argc, char** argv) {
  input_state_reset();
  shell_print(shell, "input: cleared");
  return 0;
}

static const char* shorten(const char* name) {
  if (strncmp(name, "button_", strlen("button_")) == 0) return name + strlen("button_");
  if (strncmp(name, "stick_", strlen("stick_")) == 0) return name + strlen("stick_");
  return nullptr;
}

static bool parse_button(RawInputState* state, const char* arg) {
  bool value = true;
  if (arg[0] == '+') {
    ++arg;
  } else if (arg[0] == '-') {
    value = false;
    ++arg;
  }

#define PL_GPIO(_, name, __)                                       \
  {                                                                \
    const char* short_name = shorten(#name);                       \
    if (strcmp(arg, #name) == 0 || strcmp(arg, short_name) == 0) { \
      state->name = value;                                         \
      return true;                                                 \
    }                                                              \
  }

  PL_GPIOS()

#undef PL_GPIO

  return false;
}

static int cmd_input_modify(const struct shell* shell, size_t argc, char** argv) {
  RawInputState copy = input_state;
  if (argc <= 1) {
    shell_print(shell, "usage: input modify [[+|-][button_|stick_]INPUT_NAME]...");
    return 0;
  }

  for (size_t i = 1; i < argc; ++i) {
    if (!parse_button(&copy, argv[i])) {
      shell_print(shell, "failed to parse argument '%s'", argv[i]);
      return 0;
    }
  }

  input_state = copy;
  shell_print(shell, "input: modified");
  return 0;
}

static int cmd_cam(const struct shell* shell, size_t argc, char** argv) {
  static int current_cam = 1;

  if (argc != 2) {
    shell_print(shell, "usage: cam [next | prev | reset | NUMBER]");
    return 0;
  }

  size_t count = 0;
  bool up = false;
  if (strcmp(argv[1], "reset") == 0) {
    current_cam = 1;
    return 0;
  } else if (strcmp(argv[1], "next") == 0) {
    count = 1;
    up = true;
  } else if (strcmp(argv[1], "prev") == 0) {
    count = 1;
    up = false;
  } else {
    int x = argv[1][0] - '0';
    if (x < 0 || x > 9) {
      shell_print(shell, "usage: cam [next | prev | reset | NUMBER]");
      return 0;
    }

    int diff = x - current_cam;
    if (diff < 0) {
      count = -diff;
      up = false;
    } else {
      count = diff;
      up = true;
    }
    current_cam = x;
  }

  if (count != 0) {
    for (size_t i = 0; i < count; ++i) {
      size_t j = i * 2;
      queue_storage[j].state = {};
      if (up) {
        queue_storage[j].state.stick_up = 1;
      } else {
        queue_storage[j].state.stick_down = 1;
      }
      queue_storage[j].delay = K_USEC(16'666);
      queue_storage[j].next = &queue_storage[j + 1];

      queue_storage[j + 1].state = {};
      queue_storage[j + 1].delay = K_USEC(16'666);
      queue_storage[j + 1].next = &queue_storage[j + 2];
    }
    queue_storage[count * 2 - 1].next = nullptr;

    queue_next = &queue_storage[0];
    queue_next_tick = k_uptime_ticks();
  }

  return 0;
}

static int cmd_input_home(const struct shell* shell, size_t argc, char** argv) {
  queue_storage[0].state = {};
  queue_storage[0].state.button_home = 1;
  queue_storage[0].delay = K_USEC(33'333);
  queue_storage[0].next = &queue_storage[1];
  queue_storage[1].state = {};
  queue_storage[1].delay = K_USEC(33'333);
  queue_storage[1].next = nullptr;

  queue_next = &queue_storage[0];
  queue_next_tick = k_uptime_ticks();
  return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(sub_input,
  SHELL_CMD(clear, NULL, "Clear inputs.", cmd_input_clear),
  SHELL_CMD(modify, NULL, "Modify inputs.", cmd_input_modify),
  SHELL_CMD(home, NULL, "Press home.", cmd_input_home),
  SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cam, NULL, "Gundam spectator camera control", cmd_cam);
// clang-format on
#pragma GCC diagnostic pop

SHELL_CMD_REGISTER(input, &sub_input, "Input commands", 0);
#endif
