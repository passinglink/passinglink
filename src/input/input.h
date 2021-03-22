#pragma once

#include <kernel.h>

#include "input/touchpad.h"

enum class StickState {
  Neutral = 0,
  North,
  NorthEast,
  East,
  SouthEast,
  South,
  SouthWest,
  West,
  NorthWest,
};

inline const char* to_string(StickState state) {
  switch (state) {
    case StickState::Neutral:
      return "Neutral";
    case StickState::North:
      return "North";
    case StickState::NorthEast:
      return "NorthEast";
    case StickState::East:
      return "East";
    case StickState::SouthEast:
      return "SouthEast";
    case StickState::South:
      return "South";
    case StickState::SouthWest:
      return "SouthWest";
    case StickState::West:
      return "West";
    case StickState::NorthWest:
      return "NorthWest";
  }
  return "<invalid>";
}

#define PL_GPIO_NODE(name) DT_PATH(gpio_keys, name)
#define PL_GPIO_LABEL(name) DT_GPIO_LABEL(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_PIN(name) DT_GPIO_PIN(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_FLAGS(name) DT_GPIO_FLAGS(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_AVAILABLE(name) DT_NODE_HAS_STATUS(PL_GPIO_NODE(name), okay)

static constexpr size_t PL_GPIO_COUNT = 27;

#define PL_GPIO_WRAPPER(idx, name) PL_GPIO(idx, name, PL_GPIO_AVAILABLE(name))
#define PL_GPIOS()                        \
  PL_GPIO_WRAPPER(0, stick_up)            \
  PL_GPIO_WRAPPER(1, stick_down)          \
  PL_GPIO_WRAPPER(2, stick_right)         \
  PL_GPIO_WRAPPER(3, stick_left)          \
  PL_GPIO_WRAPPER(4, button_north)        \
  PL_GPIO_WRAPPER(5, button_east)         \
  PL_GPIO_WRAPPER(6, button_south)        \
  PL_GPIO_WRAPPER(7, button_west)         \
  PL_GPIO_WRAPPER(8, button_l1)           \
  PL_GPIO_WRAPPER(9, button_l2)           \
  PL_GPIO_WRAPPER(10, button_l3)          \
  PL_GPIO_WRAPPER(11, button_r1)          \
  PL_GPIO_WRAPPER(12, button_r2)          \
  PL_GPIO_WRAPPER(13, button_r3)          \
  PL_GPIO_WRAPPER(14, button_start)       \
  PL_GPIO_WRAPPER(15, button_select)      \
  PL_GPIO_WRAPPER(16, button_home)        \
  PL_GPIO_WRAPPER(17, button_touchpad)    \
  PL_GPIO_WRAPPER(18, button_menu)        \
  PL_GPIO_WRAPPER(19, button_w)           \
  PL_GPIO_WRAPPER(20, button_thumb_left)  \
  PL_GPIO_WRAPPER(21, button_thumb_right) \
  PL_GPIO_WRAPPER(22, mode_lock)          \
  PL_GPIO_WRAPPER(23, mode_ps3)           \
  PL_GPIO_OUTPUT_MODES()

#define PL_GPIO_OUTPUT_MODES() \
  PL_GPIO_WRAPPER(24, mode_ls) \
  PL_GPIO_WRAPPER(25, mode_rs) \
  PL_GPIO_WRAPPER(26, mode_dpad)

union ButtonHistory {
  struct Button {
    bool state;

    // The cycle on which it entered the state.
    uint64_t tick;
  };

  struct {
#define PL_GPIO(index, name, available) Button name;
    PL_GPIOS()
#undef PL_GPIO
  };

  Button values[PL_GPIO_COUNT];
};

extern ButtonHistory button_history;

struct RawInputState {
#define PL_GPIO(index, name, available) \
  uint32_t name : 1;                    \
  static constexpr size_t name##_offset = index;
  PL_GPIOS()
#undef PL_GPIO
};

struct InputState {
  uint8_t left_stick_x;
  uint8_t left_stick_y;
  uint8_t right_stick_x;
  uint8_t right_stick_y;
  StickState dpad;
  uint16_t button_north : 1;
  uint16_t button_east : 1;
  uint16_t button_south : 1;
  uint16_t button_west : 1;
  uint16_t button_l1 : 1;
  uint16_t button_l2 : 1;
  uint16_t button_l3 : 1;
  uint16_t button_r1 : 1;
  uint16_t button_r2 : 1;
  uint16_t button_r3 : 1;
  uint16_t button_select : 1;
  uint16_t button_start : 1;
  uint16_t button_home : 1;
  uint16_t button_touchpad : 1;
  TouchpadData touchpad_data;
};

void input_init();

optional<uint64_t> input_get_lock_tick();
void input_set_locked(bool locked);

enum class OutputMode {
  mode_dpad,
  mode_ls,
  mode_rs,
};

OutputMode input_get_output_mode();
void input_set_output_mode(OutputMode mode);

// Get the raw state of the buttons, unaffected by SOCD cleaning, mode switches, etc.
bool input_get_raw_state(RawInputState* out);

#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)
void input_set_raw_state(RawInputState* out);
#endif

// Parse a RawInputState into host-facing output.
bool input_parse(InputState* out, const RawInputState* in);

// Get the parsed button state.
bool input_get_state(InputState* out);
