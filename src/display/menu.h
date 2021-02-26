#pragma once

void menu_init();

// Called when the menu button is pressed.
void menu_open();

// Called when the menu button is released.
void menu_close();

enum class MenuInput {
  Up,
  Down,
  Left,
  Right,
};

void menu_input(MenuInput input);
