#pragma once

#include <kernel.h>

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

#if defined(DT_GPIO_KEYS_MODE_LOCK_GPIOS)
#define PL_GPIO_MODE_LOCK_AVAILABLE 1
#define PL_GPIO_MODE_LOCK(index) PL_GPIO(index, mode_lock, MODE_LOCK)
#else
#define PL_GPIO_MODE_LOCK(index)
#endif

#if defined(DT_GPIO_KEYS_MODE_PS3_GPIOS)
#define PL_GPIO_MODE_PS3_AVAILABLE 1
#define PL_GPIO_MODE_PS3(index) PL_GPIO(index, mode_ps3, MODE_PS3)
#else
#define PL_GPIO_MODE_PS3(index)
#endif

#if defined(DT_GPIO_KEYS_MODE_DPAD_GPIOS)
#define PL_GPIO_MODE_DPAD_AVAILABLE 1
#define PL_GPIO_MODE_DPAD(index) PL_GPIO(index, mode_dpad, MODE_DPAD)
#else
#define PL_GPIO_MODE_DPAD(index)
#endif

#if defined(DT_GPIO_KEYS_MODE_LS_GPIOS)
#define PL_GPIO_MODE_LS_AVAILABLE 1
#define PL_GPIO_MODE_LS(index) PL_GPIO(index, mode_ls, MODE_LS)
#else
#define PL_GPIO_MODE_LS(index)
#endif

#if defined(DT_GPIO_KEYS_MODE_RS_GPIOS)
#define PL_GPIO_MODE_RS_AVAILABLE 1
#define PL_GPIO_MODE_RS(index) PL_GPIO(index, mode_rs, MODE_RS)
#else
#define PL_GPIO_MODE_RS(index)
#endif

#define PL_GPIO_OUTPUT_MODES(base_index) \
  PL_GPIO_MODE_DPAD(base_index)          \
  PL_GPIO_MODE_LS(base_index + 1)        \
  PL_GPIO_MODE_RS(base_index + 2)

#define PL_GPIOS()                              \
  PL_GPIO(0, button_north, BUTTON_NORTH)        \
  PL_GPIO(1, button_east, BUTTON_EAST)          \
  PL_GPIO(2, button_south, BUTTON_SOUTH)        \
  PL_GPIO(3, button_west, BUTTON_WEST)          \
  PL_GPIO(4, button_l1, BUTTON_L1)              \
  PL_GPIO(5, button_l2, BUTTON_L2)              \
  PL_GPIO(6, button_l3, BUTTON_L3)              \
  PL_GPIO(7, button_r1, BUTTON_R1)              \
  PL_GPIO(8, button_r2, BUTTON_R2)              \
  PL_GPIO(9, button_r3, BUTTON_R3)              \
  PL_GPIO(10, button_select, BUTTON_SELECT)     \
  PL_GPIO(11, button_start, BUTTON_START)       \
  PL_GPIO(12, button_home, BUTTON_HOME)         \
  PL_GPIO(13, button_touchpad, BUTTON_TOUCHPAD) \
  PL_GPIO(14, stick_up, STICK_UP)               \
  PL_GPIO(15, stick_down, STICK_DOWN)           \
  PL_GPIO(16, stick_right, STICK_RIGHT)         \
  PL_GPIO(17, stick_left, STICK_LEFT)           \
  PL_GPIO_MODE_LOCK(18)                         \
  PL_GPIO_MODE_PS3(19)                          \
  PL_GPIO_OUTPUT_MODES(20)

struct RawInputState {
#define PL_GPIO(index, name, NAME) u32_t name : 1;
  PL_GPIOS()
#undef PL_GPIO
};

struct InputState {
  u8_t left_stick_x;
  u8_t left_stick_y;
  u8_t right_stick_x;
  u8_t right_stick_y;
  StickState dpad;
  u16_t button_north : 1;
  u16_t button_east : 1;
  u16_t button_south : 1;
  u16_t button_west : 1;
  u16_t button_l1 : 1;
  u16_t button_l2 : 1;
  u16_t button_l3 : 1;
  u16_t button_r1 : 1;
  u16_t button_r2 : 1;
  u16_t button_r3 : 1;
  u16_t button_select : 1;
  u16_t button_start : 1;
  u16_t button_home : 1;
  u16_t button_touchpad : 1;
};

void input_init();

// Get the raw state of the buttons, unaffected by SOCD cleaning, mode switches, etc.
bool input_get_raw_state(RawInputState* out);

// Parse a RawInputState into host-facing output.
bool input_parse(InputState* out, const RawInputState* in);

// Get the parsed button state.
bool input_get_state(InputState* out);
