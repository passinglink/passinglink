#include "input/profile.h"

#include "display/menu.h"
#include "input/input.h"
#include "input/socd.h"
#include "types.h"

void input_profile_init() {}

#define MAPPED_BUTTONS()       \
  MAPPED_BUTTON(button_north)  \
  MAPPED_BUTTON(button_east)   \
  MAPPED_BUTTON(button_south)  \
  MAPPED_BUTTON(button_west)   \
  MAPPED_BUTTON(button_l1)     \
  MAPPED_BUTTON(button_l2)     \
  MAPPED_BUTTON(button_l3)     \
  MAPPED_BUTTON(button_r1)     \
  MAPPED_BUTTON(button_r2)     \
  MAPPED_BUTTON(button_r3)     \
  MAPPED_BUTTON(button_select) \
  MAPPED_BUTTON(button_start)  \
  MAPPED_BUTTON(button_home)   \
  MAPPED_BUTTON(button_touchpad)

// Mapping from InputState button.
//
// This could be a function in each Profile, but to more easily support custom profiles,
// track these mappings as data instead.
struct ButtonMapping {
// TODO: Remove ifdefs in RawInputState so mappings are consistent across boards?
#define MAPPED_BUTTON(name) uint8_t name;
  MAPPED_BUTTONS()
#undef MAPPED_BUTTON
};

struct RawInputStateOffsets {
#define PL_GPIO(index, name) static constexpr size_t name = __COUNTER__;
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
#define MAP(name) result.name = RawInputStateOffsets::name
  MAP(button_north);
  MAP(button_south);
  MAP(button_east);
  MAP(button_west);
  MAP(button_l1);
  MAP(button_r1);
  MAP(button_l2);
  MAP(button_r2);

  MAP(button_start);
  MAP(button_home);

#if PL_GPIO_AVAILABLE(button_l3)
  MAP(button_l3);
#else
  result.button_l3 = 0xff;
#endif

#if PL_GPIO_AVAILABLE(button_r3)
  MAP(button_r3);
#else
  result.button_r3 = 0xff;
#endif

#if PL_GPIO_AVAILABLE(button_select)
  MAP(button_select);
#else
  result.button_select = 0xff;
#endif

#if PL_GPIO_AVAILABLE(button_touchpad)
  MAP(button_touchpad);
#else
  result.button_touchpad = 0xff;
#endif

  return result;
}

static constexpr ButtonMapping default_mapping() {
  ButtonMapping result = base_mapping();
#if defined(CONFIG_BOARD_MICRODASH)
  result.button_l3 = RawInputStateOffsets::button_thumb_left;
  result.button_r3 = RawInputStateOffsets::button_thumb_right;
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

static ButtonHistory::Button* input_profile_menu_button() {
#if PL_GPIO_AVAILABLE(button_menu)
  return &button_history.button_menu;
#endif
  return nullptr;
}

static bool get_bit(const void* ptr, size_t idx) {
  auto p = static_cast<const char*>(ptr);
  char byte = p[idx / 8];
  return byte & 1 << (idx % 8);
}

bool input_profile_parse_menu(const RawInputState* in, StickOutput stick, uint64_t current_tick) {
  static bool menu_opened = false;

#if defined(CONFIG_PASSINGLINK_DISPLAY)
  if (ButtonHistory::Button* menu_button = input_profile_menu_button()) {
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

  return false;
}

void input_profile_assign_buttons(InputState* out, const RawInputState* in) {
  Profile* profile = active_profile();
  const ButtonMapping* mapping = profile->button_mapping();

#define MAPPED_BUTTON(name)                   \
  {                                           \
    if (mapping->name != 0xff) {              \
      out->name = get_bit(in, mapping->name); \
    } else {                                  \
      out->name = 0;                          \
    }                                         \
  }

  MAPPED_BUTTONS();

  bool input_locked = input_get_lock_tick();
  if (input_locked) {
    out->button_select = 0;
    out->button_start = 0;
    out->button_home = 0;
  }
}

#if defined(CONFIG_PASSINGLINK_DISPLAY)
static SOCDInputs socd_buf[32];
#else
static SOCDInputs socd_buf[2];
#endif

StickOutput::Axis input_profile_socd_x(const RawInputState* in) {
  Profile* profile = active_profile();
  size_t n = profile->socd_x(socd_buf, in);
  span<SOCDInputs> inputs(socd_buf, n);

  return input_socd_parse(input_socd_get_x_type(), inputs);
}

StickOutput::Axis input_profile_socd_y(const RawInputState* in) {
  Profile* profile = active_profile();
  size_t n = profile->socd_y(socd_buf, in);
  span<SOCDInputs> inputs(socd_buf, n);
  return input_socd_parse(input_socd_get_y_type(), inputs);
}
