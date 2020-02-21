#pragma once

#include <types.h>

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

#define PL_BUTTON_GPIOS()        \
  PL_BUTTON_GPIO(north, NORTH)   \
  PL_BUTTON_GPIO(east, EAST)     \
  PL_BUTTON_GPIO(south, SOUTH)   \
  PL_BUTTON_GPIO(west, WEST)     \
  PL_BUTTON_GPIO(l1, L1)         \
  PL_BUTTON_GPIO(l2, L2)         \
  PL_BUTTON_GPIO(l3, L3)         \
  PL_BUTTON_GPIO(r1, R1)         \
  PL_BUTTON_GPIO(r2, R2)         \
  PL_BUTTON_GPIO(r3, R3)         \
  PL_BUTTON_GPIO(select, SELECT) \
  PL_BUTTON_GPIO(start, START)   \
  PL_BUTTON_GPIO(home, HOME)     \
  PL_BUTTON_GPIO(touchpad, TOUCHPAD)

#define PL_STICK_GPIOS()      \
  PL_STICK_GPIO(up, UP)       \
  PL_STICK_GPIO(down, DOWN)   \
  PL_STICK_GPIO(right, RIGHT) \
  PL_STICK_GPIO(left, LEFT)

struct InputState {
  u32_t stick_state : 4;
#define PL_BUTTON_GPIO(name, NAME) u32_t button_##name : 1;
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO
};

static_assert(sizeof(InputState) == 4);

void input_init();

bool input_get_state(InputState* out);
