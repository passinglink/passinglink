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

static constexpr size_t PL_GPIO_COUNT = 23;

#define PL_GPIO_NODE(name) DT_PATH(gpio_keys, name)
#define PL_GPIO_LABEL(name) DT_GPIO_LABEL(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_PIN(name) DT_GPIO_PIN(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_FLAGS(name) DT_GPIO_FLAGS(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_AVAILABLE(name) DT_NODE_HAS_STATUS(PL_GPIO_NODE(name), okay)

#if PL_GPIO_AVAILABLE(mode_lock)
#define PL_GPIO_MODE_LOCK_AVAILABLE 1
#define PL_GPIO_MODE_LOCK(index) PL_GPIO(index, mode_lock)
#else
#define PL_GPIO_MODE_LOCK(index)
#endif

#if PL_GPIO_AVAILABLE(mode_ps3)
#define PL_GPIO_MODE_PS3_AVAILABLE 1
#define PL_GPIO_MODE_PS3(index) PL_GPIO(index, mode_ps3)
#else
#define PL_GPIO_MODE_PS3(index)
#endif

#if PL_GPIO_AVAILABLE(mode_dpad)
#define PL_GPIO_MODE_DPAD_AVAILABLE 1
#define PL_GPIO_MODE_DPAD(index) PL_GPIO(index, mode_dpad)
#else
#define PL_GPIO_MODE_DPAD(index)
#endif

#if PL_GPIO_AVAILABLE(mode_ls)
#define PL_GPIO_MODE_LS_AVAILABLE 1
#define PL_GPIO_MODE_LS(index) PL_GPIO(index, mode_ls)
#else
#define PL_GPIO_MODE_LS(index)
#endif

#if PL_GPIO_AVAILABLE(mode_rs)
#define PL_GPIO_MODE_RS_AVAILABLE 1
#define PL_GPIO_MODE_RS(index) PL_GPIO(index, mode_rs)
#else
#define PL_GPIO_MODE_RS(index)
#endif

#define PL_GPIO_OUTPUT_MODES(base_index) \
  PL_GPIO_MODE_DPAD(base_index)          \
  PL_GPIO_MODE_LS(base_index + 1)        \
  PL_GPIO_MODE_RS(base_index + 2)

#define PL_GPIOS()             \
  PL_GPIO(0, button_north)     \
  PL_GPIO(1, button_east)      \
  PL_GPIO(2, button_south)     \
  PL_GPIO(3, button_west)      \
  PL_GPIO(4, button_l1)        \
  PL_GPIO(5, button_l2)        \
  PL_GPIO(6, button_l3)        \
  PL_GPIO(7, button_r1)        \
  PL_GPIO(8, button_r2)        \
  PL_GPIO(9, button_r3)        \
  PL_GPIO(10, button_select)   \
  PL_GPIO(11, button_start)    \
  PL_GPIO(12, button_home)     \
  PL_GPIO(13, button_touchpad) \
  PL_GPIO(14, stick_up)        \
  PL_GPIO(15, stick_down)      \
  PL_GPIO(16, stick_right)     \
  PL_GPIO(17, stick_left)      \
  PL_GPIO_MODE_LOCK(18)        \
  PL_GPIO_MODE_PS3(19)         \
  PL_GPIO_OUTPUT_MODES(20)

struct RawInputState {
#define PL_GPIO(index, name) uint32_t name : 1;
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

bool input_get_locked();
void input_set_locked(bool locked);

// Get the raw state of the buttons, unaffected by SOCD cleaning, mode switches, etc.
bool input_get_raw_state(RawInputState* out);

#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)
void input_set_raw_state(RawInputState* out);
#endif

// Parse a RawInputState into host-facing output.
bool input_parse(InputState* out, const RawInputState* in);

// Get the parsed button state.
bool input_get_state(InputState* out);
