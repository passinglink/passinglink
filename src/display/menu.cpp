#include "display/menu.h"

#include <string.h>

#include <shell/shell.h>

#include "bootloader.h"
#include "display/display.h"
#include "input/input.h"
#include "types.h"
#include "util.h"
#include "version.h"

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(menu);

// Menu items:
struct MenuBase {
  uint8_t selected_item;

  // Returns true if it's a menu, false if it's a leaf node.
  virtual bool on_enter() = 0;

  virtual void on_exit() {}

  virtual bool show_caret() { return true; }

  // Return the text that should show up in its parent menu.
  virtual size_t render_text(span<char> buffer) = 0;
  virtual size_t menu_items(span<MenuBase*> buffer) { return 0; }
};

struct Menu : public MenuBase {
  Menu(const char* name) : name_(name) {}

  bool on_enter() override { return true; }
  size_t render_text(span<char> buffer) override { return copy_text(buffer, name_); }

  const char* name_;
};

struct TextItem : public MenuBase {
  TextItem(const char* heading, const char* value) : heading_(heading), value_(value) {}

  bool on_enter() override { return false; }
  size_t render_text(span<char> buffer) override {
    size_t n = copy_text(buffer, heading_);
    size_t m = copy_text(buffer.remove_prefix(n), ": ");
    return n + m + copy_text(buffer.remove_prefix(m), value_);
  }

  const char* heading_;
  const char* value_;
};

struct DynamicTextItem : public MenuBase {
  DynamicTextItem(const char* heading, size_t (*fn)(span<char>)) : heading_(heading), fn_(fn) {}

  bool on_enter() override { return false; }
  size_t render_text(span<char> buffer) override {
    size_t n = copy_text(buffer, heading_);
    return n + fn_(buffer.remove_prefix(n));
  }

  const char* heading_;
  size_t (*fn_)(span<char>);
};

struct ExpandedTextItem : public MenuBase {
  ExpandedTextItem(const char* text) : text_(text) {}

  bool on_enter() override { return false; }
  size_t render_text(span<char> buffer) override { return copy_text(buffer, text_); }
  bool show_caret() override { return false; }

  const char* text_;
};

struct ExpandedTextMenu : public Menu {
  ExpandedTextMenu(const char* name, const char* row1, const char* row2 = nullptr,
                   const char* row3 = nullptr)
      : Menu(name), row1_(row1), row2_(row2), row3_(row3) {}

  size_t menu_items(span<MenuBase*> buffer) override {
    buffer[0] = &row1_;
    buffer[1] = &row2_;
    buffer[2] = &row3_;
    return 3;
  }

  ExpandedTextItem row1_;
  ExpandedTextItem row2_;
  ExpandedTextItem row3_;
};

struct ActionItem : public MenuBase {
  ActionItem(const char* name, void (*fn)()) : name_(name), fn_(fn) {}

  bool on_enter() override {
    fn_();
    return false;
  }

  size_t render_text(span<char> buffer) override { return copy_text(buffer, name_); }

  const char* name_;
  void (*fn_)();
};

struct SettingsMenu : public Menu {
  SettingsMenu() : Menu("Settings"), dfu_("Firmware update", mcuboot_enter) {}

  size_t menu_items(span<MenuBase*> buffer) {
    buffer[0] = &dfu_;
    return 1;
  }

  ActionItem dfu_;
};

struct AboutMenu : public Menu {
  AboutMenu() : Menu("About"), version_("Version", version_string()) {}

  size_t menu_items(span<MenuBase*> buffer) {
    buffer[0] = &version_;
    return 1;
  }

  ExpandedTextMenu version_;
};

struct MainMenu : public Menu {
  MainMenu()
      : Menu("Main menu"),
        lock_("Lock", []() { input_set_locked(true); }),
        unlock_("Unlock", []() { input_set_locked(false); }) {}

  size_t menu_items(span<MenuBase*> buffer) override {
    buffer[0] = input_get_locked() ? &unlock_ : &lock_;
    buffer[1] = &settings_;
    buffer[2] = &about_;
    return 3;
  }

  ActionItem lock_;
  ActionItem unlock_;
  SettingsMenu settings_;
  AboutMenu about_;
};

struct MenuLocation {
  MenuBase* menu;
  size_t scroll_index;
  size_t selected_index;
};

static stack<MenuLocation, 8> menu_stack;

static array<MenuBase*, 8> menu_items_storage;
static span<MenuBase*> menu_items;

static size_t menu_scroll_index;
static size_t menu_selected_index;

static MainMenu root;

void menu_init() {
  new (&menu_stack) stack<MenuLocation, 8>();
  new (&menu_items) span<MenuBase*>();
  new (&root) MainMenu();
}

static void menu_fetch_items() {
  if (menu_stack.empty()) {
    menu_items = span<MenuBase*>();
  } else {
    MenuBase* menu = menu_stack.front().menu;
    size_t n = menu->menu_items(menu_items_storage);
    menu_items = span(menu_items_storage, n);
  }
}

static void menu_push(MenuBase* menu) {
  LOG_DBG("menu_push: size = %zu", menu_stack.size());
  if (menu->on_enter()) {
    LOG_DBG("menu_push: menu");
    MenuLocation loc = {
        .menu = menu,
        .scroll_index = menu_scroll_index,
        .selected_index = menu_selected_index,
    };

    menu_stack.push(loc);

    menu_fetch_items();
    menu_scroll_index = 0;
    menu_selected_index = 0;
  } else {
    LOG_DBG("menu_push: non-menu");

    // Refresh menu items: they might have changed as a result of an action.
    menu_fetch_items();
  }
}

static void menu_pop() {
  LOG_DBG("menu_pop: size = %zu", menu_stack.size());
  if (menu_stack.empty()) {
    return;
  }

  MenuLocation& top = menu_stack.front();
  menu_scroll_index = top.scroll_index;
  menu_selected_index = top.selected_index;
  top.menu->on_exit();
  menu_stack.pop();

  menu_fetch_items();
}

static void menu_draw() {
  if (menu_stack.empty()) {
    display_draw_logo();
  } else {
    for (size_t i = 0; i < DISPLAY_ROWS; ++i) {
      size_t menu_index = menu_scroll_index + i;
      if (i < menu_items.size()) {
        MenuBase* item = menu_items[menu_index];
        char buf[DISPLAY_WIDTH + 1];

        span output(buf, DISPLAY_WIDTH);
        if (item->show_caret()) {
          memcpy(output.data(), (menu_index == menu_selected_index) ? "> " : "  ", 2);
          output.remove_prefix(2);
        }

        size_t chars = item->render_text(output);
        output[chars] = '\0';

        display_set_line(i, buf);
      } else {
        display_set_line(i, nullptr);
      }
    }
  }

  display_blit();
}

void menu_open() {
  LOG_DBG("menu_open");
  while (!menu_stack.empty()) {
    menu_pop();
  }

  menu_push(&root);
  menu_draw();
}

void menu_close() {
  LOG_DBG("menu_close");
  while (!menu_stack.empty()) {
    menu_pop();
  }

  menu_draw();
}

void menu_input(MenuInput input) {
  switch (input) {
    case MenuInput::Up:
      if (menu_selected_index != 0) {
        --menu_selected_index;

        if (menu_selected_index < menu_scroll_index) {
          menu_scroll_index = menu_selected_index;
        }

        menu_draw();
      }
      break;

    case MenuInput::Down:
      if (menu_selected_index + 1 < menu_items.size()) {
        ++menu_selected_index;

        if (menu_selected_index >= menu_scroll_index + DISPLAY_ROWS) {
          ++menu_scroll_index;
        }

        menu_draw();
      }
      break;

    case MenuInput::Left:
      if (menu_stack.size() != 1) {
        menu_pop();
        menu_draw();
      }
      break;

    case MenuInput::Right:
      menu_push(menu_items[menu_selected_index]);
      menu_draw();
      break;
  }
}

#if defined(CONFIG_SHELL)
static int cmd_menu(const struct shell* shell, size_t argc, char** argv) {
  if (argc != 2) {
    goto usage;
  }

  if (strcmp(argv[1], "open") == 0) {
    menu_open();
  } else if (strcmp(argv[1], "close") == 0) {
    menu_close();
  } else if (strcmp(argv[1], "up") == 0) {
    menu_input(MenuInput::Up);
  } else if (strcmp(argv[1], "down") == 0) {
    menu_input(MenuInput::Down);
  } else if (strcmp(argv[1], "left") == 0) {
    menu_input(MenuInput::Left);
  } else if (strcmp(argv[1], "right") == 0) {
    menu_input(MenuInput::Right);
  } else {
    goto usage;
  }

  return 0;

usage:
  shell_print(shell, "usage: menu [open | close | up | down | left | right]");
  return 0;
}

SHELL_CMD_REGISTER(menu, NULL, "Menu commands", cmd_menu);
#endif
