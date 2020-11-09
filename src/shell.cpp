#include <zephyr.h>

#include <shell/shell.h>

#include "input/input.h"
#include "input/queue.h"

#if defined(CONFIG_PASSINGLINK_INPUT_SHELL)

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

#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)
static int cmd_input_clear(const struct shell* shell, size_t argc, char** argv) {
  shell_print(shell, "input: cleared");
  return 0;
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
#endif

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
    InputQueue* head = input_queue_alloc();
    if (!head) {
      shell_print(shell, "failed to allocate!");
      return 0;
    }

    InputQueue* cur = head;
    for (size_t i = 0; i < count; ++i) {
      cur->state = {};
      if (up) {
        cur->state.stick_up = 1;
      } else {
        cur->state.stick_down = 1;
      }
      cur->delay = K_USEC(16'666);
      cur = input_queue_append(cur);
      if (!cur) {
        shell_print(shell, "failed to allocate!");
        input_queue_free(head);
        return 0;
      }

      cur->state = {};
      cur->delay = K_USEC(16'666);
      cur = input_queue_append(cur);
      if (!cur) {
        shell_print(shell, "failed to allocate!");
        input_queue_free(head);
        return 0;
      }
    }

    cur->state = {};
    cur->delay = K_USEC(0);
    input_queue_set_active(head, true);
  }

  return 0;
}

static int cmd_input_home(const struct shell* shell, size_t argc, char** argv) {
  InputQueue* head = input_queue_alloc();
  if (!head) {
    shell_print(shell, "failed to allocate!");
    return 0;
  }

  head->state = {};
  head->state.button_home = 1;
  head->delay = K_USEC(33'333);

  InputQueue* next = input_queue_append(head);
  if (!next) {
    shell_print(shell, "failed to allocate!");
    input_queue_free(head);
    return 0;
  }

  next->state = {};
  next->delay = K_USEC(33'333);

  input_queue_set_active(head, true);
  return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(sub_input,
#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)
  SHELL_CMD(clear, NULL, "Clear inputs.", cmd_input_clear),
  SHELL_CMD(modify, NULL, "Modify inputs.", cmd_input_modify),
#endif
  SHELL_CMD(home, NULL, "Press home.", cmd_input_home),
  SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cam, NULL, "Gundam spectator camera control", cmd_cam);
// clang-format on
#pragma GCC diagnostic pop

SHELL_CMD_REGISTER(input, &sub_input, "Input commands", 0);
#endif
