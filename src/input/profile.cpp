#include "input/profile.h"

#include "display/menu.h"
#include "input/input.h"
#include "input/socd.h"

void input_profile_init() {}

static ButtonHistory::Button* input_profile_menu_button() {
#if defined(PL_GPIO_BUTTON_MENU_AVAILABLE)
  return &button_history.button_menu;
#endif
  return nullptr;
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
  out->button_north = in->button_north;
  out->button_east = in->button_east;
  out->button_south = in->button_south;
  out->button_west = in->button_west;
  out->button_l1 = in->button_l1;
  out->button_l2 = in->button_l2;

  out->button_r1 = in->button_r1;
  out->button_r2 = in->button_r2;

#if defined(PL_GPIO_BUTTON_L3_AVAILABLE)
  out->button_l3 = in->button_l3;
#endif

#if defined(PL_GPIO_BUTTON_R3_AVAILABLE)
  out->button_r3 = in->button_r3;
#endif

#if defined(PL_GPIO_BUTTON_TOUCHPAD_AVAILABLE)
  out->button_touchpad = in->button_touchpad;
#endif

  bool input_locked = input_get_lock_tick();

#if defined(PL_GPIO_BUTTON_SELECT_AVAILABLE)
  out->button_select = input_locked ? 0 : in->button_select;
#endif

  out->button_start = input_locked ? 0 : in->button_start;
  out->button_home = input_locked ? 0 : in->button_home;
}

StickOutput::Axis input_profile_socd_x(const RawInputState* in) {
  SOCDInputs inputs[] = {
    { in->stick_left, button_history.stick_left.tick, SOCDButtonType::Negative },
    { in->stick_right, button_history.stick_right.tick, SOCDButtonType::Positive },
#if defined(PL_GPIO_BUTTON_THUMB_LEFT_AVAILABLE)
    { in->button_thumb_left, button_history.button_thumb_left.tick, SOCDButtonType::Negative,
      true },
#endif
#if defined(PL_GPIO_BUTTON_THUMB_RIGHT_AVAILABLE)
    { in->button_thumb_right, button_history.button_thumb_right.tick, SOCDButtonType::Positive,
      true },
#endif
  };

  return input_socd_parse(input_socd_get_x_type(), inputs);
}

StickOutput::Axis input_profile_socd_y(const RawInputState* in) {
  SOCDInputs inputs[] = {
    { in->stick_up, button_history.stick_up.tick, SOCDButtonType::Negative },
    { in->stick_down, button_history.stick_down.tick, SOCDButtonType::Positive },
#if defined(PL_GPIO_BUTTON_W_AVAILABLE)
    { in->button_w, button_history.button_w.tick, SOCDButtonType::Negative },
#endif
#if defined(PL_GPIO_BUTTON_THUMB_LEFT_AVAILABLE)
    { in->button_thumb_left, button_history.button_thumb_left.tick, SOCDButtonType::Negative,
      true },
#endif
#if defined(PL_GPIO_BUTTON_THUMB_RIGHT_AVAILABLE)
    { in->button_thumb_right, button_history.button_thumb_right.tick, SOCDButtonType::Negative,
      true },
#endif
  };

  return input_socd_parse(input_socd_get_y_type(), inputs);
}
