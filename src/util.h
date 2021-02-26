#pragma once

#include <string.h>

#include <zephyr.h>

#include "types.h"

static size_t copy_text(span<char> buf, const char* str) {
  if (!str) {
    return 0;
  }

  size_t n = min(strlen(str), buf.size());
  strncpy(buf.data(), str, n);
  return n;
}
