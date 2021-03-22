#include "input/profile.h"

#include "display/menu.h"
#include "input/input.h"
#include "input/socd.h"
#include "types.h"

// Struct representing button remapping in a profile.
//
// If ButtonMapping::button_foo == RawInputState::button_baz_offset, then when
// the physical button_baz is pressed, it is interpreted as button_foo.
//
// 0xFF is treated specially as a nonexistent button.
//
// This could be a function in each Profile, but to more easily support custom profiles,
// track these mappings as data instead.
struct ButtonMapping {
#define PL_GPIO(index, name, available) uint8_t name;
  PL_GPIOS()
#undef PL_GPIO
};

struct Profile {
  virtual const char* name() = 0;
  virtual const ButtonMapping* button_mapping() = 0;
  virtual size_t socd_x(span<SOCDInputs> out, const RawInputState* in) = 0;
  virtual size_t socd_y(span<SOCDInputs> out, const RawInputState* in) = 0;
};

static constexpr ButtonMapping base_mapping() {
  ButtonMapping result = {};
#define PL_GPIO(index, name, available) \
  COND_CODE_1(available, (result.name = index;), (result.name = 0xff;))
  PL_GPIOS()
#undef PL_GPIO

  return result;
}

static constexpr ButtonMapping default_mapping() {
  ButtonMapping result = base_mapping();
#if defined(CONFIG_BOARD_MICRODASH)
  result.button_l3 = RawInputState::button_thumb_left_offset;
  result.button_r3 = RawInputState::button_thumb_right_offset;
#endif
  return result;
}

static size_t default_socd_x(span<SOCDInputs> out, const RawInputState* in) {
  size_t i = 0;
  out[i++] = { in->stick_left, button_history.stick_left.tick, SOCDButtonType::Negative };
  out[i++] = { in->stick_right, button_history.stick_right.tick, SOCDButtonType::Positive };
  return i;
}

static size_t default_socd_y(span<SOCDInputs> out, const RawInputState* in) {
  size_t i = 0;
  out[i++] = { in->stick_up, button_history.stick_up.tick, SOCDButtonType::Negative };
  out[i++] = { in->stick_down, button_history.stick_down.tick, SOCDButtonType::Positive };
#if PL_GPIO_AVAILABLE(button_w)
  out[i++] = { in->button_w, button_history.button_w.tick, SOCDButtonType::Negative };
#endif
  return i;
}

static struct DefaultProfile : Profile {
  const char* name() final { return "Default"; }

  const ButtonMapping* button_mapping() final { return &mapping; }

  size_t socd_x(span<SOCDInputs> out, const RawInputState* in) final {
    return default_socd_x(out, in);
  }

  size_t socd_y(span<SOCDInputs> out, const RawInputState* in) final {
    return default_socd_y(out, in);
  }

  static constexpr ButtonMapping mapping = default_mapping();
} default_profile;

#if defined(CONFIG_PASSINGLINK_DISPLAY)

static size_t dashblock_socd_x(span<SOCDInputs> out, const RawInputState* in) {
  size_t n = default_socd_x(out, in);
#if PL_GPIO_AVAILABLE(button_thumb_left)
  out[n++] = { in->button_thumb_left, button_history.button_thumb_left.tick,
               SOCDButtonType::Negative, true };
#endif
#if PL_GPIO_AVAILABLE(button_thumb_right)
  out[n++] = { in->button_thumb_right, button_history.button_thumb_right.tick,
               SOCDButtonType::Positive, true };
#endif
  return n;
}

static size_t dashblock_socd_y(span<SOCDInputs> out, const RawInputState* in) {
  size_t n = default_socd_y(out, in);
#if PL_GPIO_AVAILABLE(button_thumb_left)
  out[n++] = { in->button_thumb_left, button_history.button_thumb_left.tick,
               SOCDButtonType::Neutral, true };
#endif
#if PL_GPIO_AVAILABLE(button_thumb_right)
  out[n++] = { in->button_thumb_right, button_history.button_thumb_right.tick,
               SOCDButtonType::Neutral, true };
#endif
  return n;
}

static struct DashblockProfile : Profile {
  const char* name() final { return "Dashblock"; }

  const ButtonMapping* button_mapping() final { return &mapping; }

  size_t socd_x(span<SOCDInputs> out, const RawInputState* in) final {
    return dashblock_socd_x(out, in);
  }

  size_t socd_y(span<SOCDInputs> out, const RawInputState* in) final {
    return dashblock_socd_y(out, in);
  }

  static constexpr ButtonMapping mapping = base_mapping();
} dashblock_profile;

static size_t tigerknee_socd_x(span<SOCDInputs> out, const RawInputState* in) {
  size_t n = default_socd_x(out, in);
#if PL_GPIO_AVAILABLE(button_thumb_left)
  out[n++] = { in->button_thumb_left, button_history.button_thumb_left.tick,
               SOCDButtonType::Negative, true };
#endif
#if PL_GPIO_AVAILABLE(button_thumb_right)
  out[n++] = { in->button_thumb_right, button_history.button_thumb_right.tick,
               SOCDButtonType::Positive, true };
#endif
  return n;
}

static size_t tigerknee_socd_y(span<SOCDInputs> out, const RawInputState* in) {
  size_t n = default_socd_y(out, in);
#if PL_GPIO_AVAILABLE(button_thumb_left)
  out[n++] = { in->button_thumb_left, button_history.button_thumb_left.tick,
               SOCDButtonType::Negative, true };
#endif
#if PL_GPIO_AVAILABLE(button_thumb_right)
  out[n++] = { in->button_thumb_right, button_history.button_thumb_right.tick,
               SOCDButtonType::Negative, true };
#endif
  return n;
}

static struct TigerKneeProfile : Profile {
  const char* name() final { return "Tigerknee"; }

  const ButtonMapping* button_mapping() final { return &mapping; }

  size_t socd_x(span<SOCDInputs> out, const RawInputState* in) final {
    return tigerknee_socd_x(out, in);
  }

  size_t socd_y(span<SOCDInputs> out, const RawInputState* in) final {
    return tigerknee_socd_y(out, in);
  }

  static constexpr ButtonMapping mapping = base_mapping();
} tigerknee_profile;
#endif  // defined(CONFIG_PASSINGLINK_DISPLAY)

// TODO: Support custom profiles.
#if defined(CONFIG_PASSINGLINK_DISPLAY)
static const array<Profile*, 3> profiles = { &default_profile, &dashblock_profile,
                                             &tigerknee_profile };
#else
static const array<Profile*, 1> profiles = { &default_profile };
#endif

// TODO: Save active profile.
static size_t active_profile_idx = 0;

size_t input_profile_count() {
  return profiles.size();
}

const char* input_profile_get_name(size_t idx) {
  return profiles[idx]->name();
}

size_t input_profile_get_active() {
  return active_profile_idx;
}

void input_profile_activate(size_t idx) {
  active_profile_idx = idx;
}

static Profile* active_profile() {
  return profiles[active_profile_idx];
}

#if defined(CONFIG_PASSINGLINK_DISPLAY)
static SOCDInputs socd_buf[32];
#else
static SOCDInputs socd_buf[2];
#endif

static StickOutput::Axis input_profile_socd_x(Profile* profile, const RawInputState* in) {
  size_t n = profile->socd_x(socd_buf, in);
  span<SOCDInputs> inputs(socd_buf, n);
  return input_socd_parse(input_socd_get_x_type(), inputs);
}

static StickOutput::Axis input_profile_socd_y(Profile* profile, const RawInputState* in) {
  size_t n = profile->socd_y(socd_buf, in);
  span<SOCDInputs> inputs(socd_buf, n);
  return input_socd_parse(input_socd_get_y_type(), inputs);
}

static bool get_bit(const void* ptr, size_t idx) {
  auto p = static_cast<const char*>(ptr);
  char byte = p[idx / 8];
  return byte & 1 << (idx % 8);
}

#if defined(CONFIG_PASSINGLINK_DISPLAY)
bool input_profile_parse_menu(const RawInputState* in, ButtonHistory::Button* menu_button,
                              StickOutput stick, uint64_t current_tick) {
  static bool menu_opened = false;

  if (!menu_button->state) {
    if (menu_opened) {
      menu_close();
      menu_opened = false;
    }
    return false;
  }

  if (optional<uint64_t> lock_tick = input_get_lock_tick()) {
    // TODO: Make timeout configurable.
    // TODO: Display progress bar.
    if (current_tick - menu_button->tick < k_ms_to_ticks_ceil64(2000)) {
      return false;
    } else if (*lock_tick < menu_button->tick) {
      if (!menu_opened) {
        menu_opened = true;
        menu_open();
      }
    }
  } else if (!menu_opened) {
    menu_opened = true;
    menu_open();
  }

  if (stick.x.value != 0 && stick.x.tick == current_tick) {
    if (stick.x.value == -1) {
      menu_input(MenuInput::Left);
    } else {
      menu_input(MenuInput::Right);
    }
    return true;
  }

  if (stick.y.value != 0 && stick.y.tick == current_tick) {
    if (stick.y.value == -1) {
      menu_input(MenuInput::Up);
    } else {
      menu_input(MenuInput::Down);
    }
    return true;
  }

  return true;
}
#endif

// Convert ({-1, 0, 1}, {-1, 0, 1}) to a StickState.
static StickState stick_state_from_x_y(int horizontal, int vertical) {
  if (vertical == -1 && horizontal == 0) {
    return StickState::North;
  } else if (vertical == -1 && horizontal == 1) {
    return StickState::NorthEast;
  } else if (vertical == 0 && horizontal == 1) {
    return StickState::East;
  } else if (vertical == 1 && horizontal == 1) {
    return StickState::SouthEast;
  } else if (vertical == 1 && horizontal == 0) {
    return StickState::South;
  } else if (vertical == 1 && horizontal == -1) {
    return StickState::SouthWest;
  } else if (vertical == 0 && horizontal == -1) {
    return StickState::West;
  } else if (vertical == -1 && horizontal == -1) {
    return StickState::NorthWest;
  } else {
    return StickState::Neutral;
  }

  __builtin_unreachable();
}

// Scale {-1, 0, 1} to {-128, 0, 127}.
static uint8_t stick_scale(int sign) {
  if (sign < 0) {
    return 0x00;
  } else if (sign == 0) {
    return 0x80;
  } else {
    return 0xFF;
  }
}

static StickOutput input_socd(Profile* profile, const RawInputState* in) {
  return StickOutput {
    .x = input_profile_socd_x(profile, in),
    .y = input_profile_socd_y(profile, in),
  };
}

void input_profile_parse(InputState* out, const RawInputState* in, uint64_t current_tick) {
  Profile* profile = active_profile();
  const ButtonMapping* mapping = profile->button_mapping();

  StickOutput stick_output = input_socd(profile, in);

#if defined(CONFIG_PASSINGLINK_DISPLAY)
  if (mapping->button_menu != 0xff) {
    if (input_profile_parse_menu(in, &button_history.values[mapping->button_menu], stick_output,
                                 current_tick)) {
      return;
    }
  }
#endif

#define BUTTONS()       \
  BUTTON(button_north)  \
  BUTTON(button_east)   \
  BUTTON(button_south)  \
  BUTTON(button_west)   \
  BUTTON(button_l1)     \
  BUTTON(button_l2)     \
  BUTTON(button_l3)     \
  BUTTON(button_r1)     \
  BUTTON(button_r2)     \
  BUTTON(button_r3)     \
  BUTTON(button_select) \
  BUTTON(button_start)  \
  BUTTON(button_home)   \
  BUTTON(button_touchpad)
#define BUTTON(name) out->name = (mapping->name == 0xff) ? 0 : get_bit(in, mapping->name);
  BUTTONS();
#undef BUTTON
#undef BUTTONS

  OutputMode output_mode = input_get_output_mode();
  switch (output_mode) {
    case OutputMode::mode_dpad:
      out->dpad = stick_state_from_x_y(stick_output.x.value, stick_output.y.value);
      break;

    case OutputMode::mode_ls:
      out->left_stick_x = stick_scale(stick_output.x.value);
      out->left_stick_y = stick_scale(stick_output.y.value);
      break;

    case OutputMode::mode_rs:
      out->right_stick_x = stick_scale(stick_output.x.value);
      out->right_stick_y = stick_scale(stick_output.y.value);
      break;
  }

  bool input_locked = input_get_lock_tick();
  if (input_locked) {
    out->button_select = 0;
    out->button_start = 0;
    out->button_home = 0;
  }
}

void input_profile_init() {}
