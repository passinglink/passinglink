#pragma once

#include "input/input.h"

enum class SOCDType : size_t {
  // Neutral.
  Neutral = 0,

  // Last input wins.
  Last = 1,

  // Negative input (up, left) wins
  Negative = 2,

  // Positive input (down, right) wins.
  Positive = 3,
};

SOCDType input_socd_get_x_type();
void input_socd_set_x_type(SOCDType type);

SOCDType input_socd_get_y_type();
void input_socd_set_y_type(SOCDType type);

struct StickOutput {
  struct Axis {
    // -1 to 1
    int value;

    uint64_t tick;
  };

  Axis x;
  Axis y;
};

enum class SOCDButtonType : int {
  // Down, right
  Positive = 1,

  // Neutral.
  Neutral = 0,

  // Up, left
  Negative = -1,
};

struct SOCDInputs {
  // bool, but bitfields make it so a cast would be needed to silence warnings.
  uint32_t input_value;
  uint64_t input_tick;
  SOCDButtonType button_type;
  bool overrides = false;
};

StickOutput::Axis input_socd_parse(SOCDType type, span<SOCDInputs> inputs);
