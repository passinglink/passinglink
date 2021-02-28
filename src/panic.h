#pragma once

#define PANIC(...)       \
  ({                     \
    printk(__VA_ARGS__); \
    k_panic();           \
  })
