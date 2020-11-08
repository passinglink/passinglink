#include <zephyr.h>

#include <shell/shell.h>

#include "input/input.h"

#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)

static RawInputState input_state;

bool input_get_raw_state(RawInputState* out) {
  *out = input_state;
  return true;
}

static int cmd_input_clear(const struct shell* shell, size_t argc, char** argv) {
  shell_print(shell, "input: cleared");
  memset(&input_state, 0, sizeof(input_state));
  return 0;
}

static const char* shorten(const char* name) {
  if (strncmp(name, "button_", strlen("button_")) == 0) return name + strlen("button_");
  if (strncmp(name, "stick_", strlen("stick_")) == 0) return name + strlen("stick_");
  return nullptr;
}

static bool parse_modify_arg(const struct shell* shell, RawInputState* state, const char* arg) {
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
      shell_print(shell, "%c%s", value ? '+' : '-', #name);        \
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
    if (!parse_modify_arg(shell, &copy, argv[i])) {
      shell_print(shell, "failed to parse argument '%s'", argv[i]);
      return 0;
    }
  }

  input_state = copy;
  shell_print(shell, "input: modified");
  return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(sub_input,
  SHELL_CMD(clear, NULL, "Clear inputs.", cmd_input_clear),
  SHELL_CMD(modify, NULL, "Modify inputs.", cmd_input_modify),
  SHELL_SUBCMD_SET_END
);
// clang-format on
#pragma GCC diagnostic pop

SHELL_CMD_REGISTER(input, &sub_input, "Input commands", 0);
#endif
