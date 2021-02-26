#pragma once

#include <zephyr.h>

#define PANIC(...)       \
  ({                          \
    printk(__VA_ARGS__); \
    k_panic();                \
  })
