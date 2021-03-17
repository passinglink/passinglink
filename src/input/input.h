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

static constexpr size_t PL_GPIO_COUNT = 27;

#define PL_GPIO_NODE(name) DT_PATH(gpio_keys, name)
#define PL_GPIO_LABEL(name) DT_GPIO_LABEL(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_PIN(name) DT_GPIO_PIN(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_FLAGS(name) DT_GPIO_FLAGS(PL_GPIO_NODE(name), gpios)
#define PL_GPIO_AVAILABLE(name) DT_NODE_HAS_STATUS(PL_GPIO_NODE(name), okay)

#if PL_GPIO_AVAILABLE(button_l3)
#define PL_GPIO_BUTTON_L3_AVAILABLE 1
#define PL_GPIO_BUTTON_L3(index) PL_GPIO(index, button_l3)
#else
#define PL_GPIO_BUTTON_L3(index)
#endif

#if PL_GPIO_AVAILABLE(button_r3)
#define PL_GPIO_BUTTON_R3_AVAILABLE 1
#define PL_GPIO_BUTTON_R3(index) PL_GPIO(index, button_r3)
#else
#define PL_GPIO_BUTTON_R3(index)
#endif

#if PL_GPIO_AVAILABLE(button_select)
#define PL_GPIO_BUTTON_SELECT_AVAILABLE 1
#define PL_GPIO_BUTTON_SELECT(index) PL_GPIO(index, button_select)
#else
#define PL_GPIO_BUTTON_SELECT(index)
#endif

#if PL_GPIO_AVAILABLE(button_touchpad)
#define PL_GPIO_BUTTON_TOUCHPAD_AVAILABLE 1
#define PL_GPIO_BUTTON_TOUCHPAD(index) PL_GPIO(index, button_touchpad)
#else
#define PL_GPIO_BUTTON_TOUCHPAD(index)
#endif

#if PL_GPIO_AVAILABLE(button_w)
#define PL_GPIO_BUTTON_W_AVAILABLE 1
#define PL_GPIO_BUTTON_W(index) PL_GPIO(index, button_w)
#else
#define PL_GPIO_BUTTON_W(index)
#endif

#if PL_GPIO_AVAILABLE(button_thumb_left)
#define PL_GPIO_BUTTON_THUMB_LEFT_AVAILABLE 1
#define PL_GPIO_BUTTON_THUMB_LEFT(index) PL_GPIO(index, button_thumb_left)
#else
#define PL_GPIO_BUTTON_THUMB_LEFT(index)
#endif

#if PL_GPIO_AVAILABLE(button_thumb_right)
#define PL_GPIO_BUTTON_THUMB_RIGHT_AVAILABLE 1
#define PL_GPIO_BUTTON_THUMB_RIGHT(index) PL_GPIO(index, button_thumb_right)
#else
#define PL_GPIO_BUTTON_THUMB_RIGHT(index)
#endif

#if PL_GPIO_AVAILABLE(button_menu)
#define PL_GPIO_BUTTON_MENU_AVAILABLE 1
#define PL_GPIO_BUTTON_MENU(index) PL_GPIO(index, button_menu)
#else
#define PL_GPIO_BUTTON_MENU(index)
#endif

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

#define PL_GPIOS()               \
  PL_GPIO(0, button_north)       \
  PL_GPIO(1, button_east)        \
  PL_GPIO(2, button_south)       \
  PL_GPIO(3, button_west)        \
  PL_GPIO(4, button_l1)          \
  PL_GPIO(5, button_l2)          \
  PL_GPIO(6, button_r1)          \
  PL_GPIO(7, button_r2)          \
  PL_GPIO(8, button_start)       \
  PL_GPIO(9, button_home)        \
  PL_GPIO(10, stick_up)          \
  PL_GPIO(11, stick_down)        \
  PL_GPIO(12, stick_right)       \
  PL_GPIO(13, stick_left)        \
  PL_GPIO_BUTTON_L3(14)          \
  PL_GPIO_BUTTON_R3(15)          \
  PL_GPIO_BUTTON_SELECT(16)      \
  PL_GPIO_BUTTON_TOUCHPAD(17)    \
  PL_GPIO_BUTTON_W(18)           \
  PL_GPIO_BUTTON_THUMB_LEFT(19)  \
  PL_GPIO_BUTTON_THUMB_RIGHT(20) \
  PL_GPIO_BUTTON_MENU(21)        \
  PL_GPIO_MODE_LOCK(22)          \
  PL_GPIO_MODE_PS3(23)           \
  PL_GPIO_OUTPUT_MODES(24)

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

SOCDType input_get_x_socd_type();
void input_set_x_socd_type(SOCDType type);

SOCDType input_get_y_socd_type();
void input_set_y_socd_type(SOCDType type);

// Get the raw state of the buttons, unaffected by SOCD cleaning, mode switches, etc.
bool input_get_raw_state(RawInputState* out);

#if defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)
void input_set_raw_state(RawInputState* out);
#endif

// Parse a RawInputState into host-facing output.
bool input_parse(InputState* out, const RawInputState* in);

// Get the parsed button state.
bool input_get_state(InputState* out);
