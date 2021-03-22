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

#define PL_GPIO(_, name, available)                                \
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
  RawInputState state = {};
  input_get_raw_state(&state);

  if (argc <= 1) {
    shell_print(shell, "usage: input modify [[+|-][button_|stick_]INPUT_NAME]...");
    return 0;
  }

  for (size_t i = 1; i < argc; ++i) {
    if (!parse_button(&state, argv[i])) {
      shell_print(shell, "failed to parse argument '%s'", argv[i]);
      return 0;
    }
  }

  input_set_raw_state(&state);
  shell_print(shell, "input: modified");
  return 0;
}
#endif

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

// clang-format on
#pragma GCC diagnostic pop

SHELL_CMD_REGISTER(input, &sub_input, "Input commands", 0);
#endif
