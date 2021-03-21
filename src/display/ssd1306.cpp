#include "display/display.h"

#include <zephyr.h>

#include <device.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "arch.h"
#include "types.h"

#include "display/font.h"
#include "display/logo.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(ssd1306);

// static I2CManager* i2c;
static const struct device* i2c_device;

static constexpr uint8_t display_addr = 0x3c;

static constexpr array<uint8_t, 2> ssd1306_set_contrast(uint8_t contrast) {
  return { 0x81, contrast };
}

static constexpr array<uint8_t, 1> ssd1306_set_entire_display_on(bool on) {
  return { static_cast<uint8_t>(on ? 0xa5 : 0xa4) };
}

static constexpr array<uint8_t, 1> ssd1306_set_inverse(bool inverse) {
  return { static_cast<uint8_t>(inverse ? 0xa7 : 0xa6) };
}

static constexpr array<uint8_t, 1> ssd1306_set_display_on(bool on) {
  return { static_cast<uint8_t>(on ? 0xaf : 0xae) };
}

// Scrolling commands
enum class ScrollDirection {
  Right,
  Left,
};

enum class ScrollInterval : uint8_t {
  Frame5,
  Frame64,
  Frame128,
  Frame256,
  Frame3,
  Frame4,
  Frame25,
  Frame2,
};

static constexpr array<uint8_t, 7> ssd1306_setup_horizontal_scroll(ScrollDirection direction,
                                                                   uint8_t start_page,
                                                                   uint8_t end_page,
                                                                   ScrollInterval interval) {
  return { static_cast<uint8_t>(direction == ScrollDirection::Right ? 0x26 : 0x27),
           0,
           start_page,
           static_cast<uint8_t>(interval),
           end_page,
           0,
           0xff };
}

static constexpr array<uint8_t, 7> ssd1306_setup_horizontal_vertical_scroll(
  ScrollDirection direction, uint8_t start_page, uint8_t end_page, ScrollInterval interval,
  uint8_t vertical_offset) {
  return { static_cast<uint8_t>(direction == ScrollDirection::Right ? 0x29 : 0x2a),
           0,
           start_page,
           static_cast<uint8_t>(interval),
           end_page,
           0,
           vertical_offset };
}

static constexpr array<uint8_t, 1> ssd1306_deactivate_scroll() {
  return { 0x2e };
}

static constexpr array<uint8_t, 1> ssd1306_activate_scroll() {
  return { 0x2f };
}

static constexpr array<uint8_t, 3> ssd1306_set_vertical_scroll_area(uint8_t top, uint8_t rows) {
  return { 0xa3, top, rows };
}

// Addressing
static constexpr array<uint8_t, 1> ssd1306_set_column_start_address_lower(uint8_t nybble) {
  return { static_cast<uint8_t>(nybble & 0b1111) };
}

static constexpr array<uint8_t, 1> ssd1306_set_column_start_address_upper(uint8_t nybble) {
  return { static_cast<uint8_t>(0x10 | (nybble & 0b1111)) };
}

enum class MemoryAddressingMode : uint8_t {
  Horizontal,
  Vertical,
  Page,
};

static constexpr array<uint8_t, 2> ssd1306_set_memory_addressing_mode(MemoryAddressingMode mode) {
  return { 0x20, static_cast<uint8_t>(mode) };
}

static constexpr array<uint8_t, 3> ssd1306_set_column_address(uint8_t start, uint8_t end) {
  return { 0x21, start, end };
}

static constexpr array<uint8_t, 3> ssd1306_set_page_address(uint8_t start, uint8_t end) {
  return { 0x22, start, end };
}

static constexpr array<uint8_t, 1> ssd1306_set_page_start_address(uint8_t address) {
  return { static_cast<uint8_t>(0xb0 | (address & 0b111)) };
}

// Hardware configuration
static constexpr array<uint8_t, 1> ssd1306_set_display_start_line(uint8_t line) {
  return { static_cast<uint8_t>(0x40 | (line & 0b111111)) };
}

static constexpr array<uint8_t, 1> ssd1306_set_segment_remap(bool remap) {
  return { static_cast<uint8_t>(0xa0 | remap) };
}

// ratio must be >= 15
static constexpr array<uint8_t, 2> ssd1306_set_multiplex_ratio(uint8_t ratio) {
  return { 0xa8, static_cast<uint8_t>((ratio - 1) & 0b111111) };
}

static constexpr array<uint8_t, 2> ssd1306_set_com_output_scan_direction(bool remapped) {
  return { static_cast<uint8_t>(0xc0 | remapped << 3) };
}

static constexpr array<uint8_t, 2> ssd1306_set_display_offset(uint8_t offset) {
  return { 0xd3, offset };
}

enum class PinConfiguration {
  Sequential,
  Alternative,
};

enum class LeftRightRemap {
  Enabled,
  Disabled,
};

static constexpr array<uint8_t, 2> ssd1306_set_com_pins(PinConfiguration pin, LeftRightRemap lr) {
  uint8_t arg = 2;
  if (pin == PinConfiguration::Alternative) {
    arg |= 1 << 4;
  }
  if (lr == LeftRightRemap::Enabled) {
    arg |= 1 << 5;
  }
  return { 0xda, arg };
}

// Timing and driving
static constexpr array<uint8_t, 2> ssd1306_set_display_clock(uint8_t divide_ratio, uint8_t freq) {
  return { 0xd5, static_cast<uint8_t>(((divide_ratio - 1) & 0b111) | (freq & 0b1111) << 4) };
}

static constexpr array<uint8_t, 2> ssd1306_set_precharge_period(uint8_t phase1, uint8_t phase2) {
  return { 0xd9, static_cast<uint8_t>((phase1 & 0b1111) | (phase2 & 0b1111) << 4) };
}

// 0 = 0.65 * VCC
// 2 = 0.77 * VCC
// 3 = 0.83 * VCC
static constexpr array<uint8_t, 2> ssd1306_set_vcom_dselect_level(uint8_t level) {
  return { 0xdb, static_cast<uint8_t>((level & 0b111) << 4) };
}

static constexpr array<uint8_t, 1> ssd1306_nop() {
  return { 0xe3 };
}

// Charge pump
static constexpr array<uint8_t, 2> ssd1306_set_enable_charge_pump(bool enable) {
  return { 0x8d, static_cast<uint8_t>(0b00010000 | enable << 2) };
}

// Data write
static constexpr array<uint8_t, 2> ssd1306_write_byte(uint8_t byte) {
  return { 0x40, byte };
}

template <size_t N>
constexpr size_t ssd1306_cmd_size(const array<uint8_t, N>& arg) {
  return N;
}

template <size_t N, typename... Args>
constexpr size_t ssd1306_cmd_size(const array<uint8_t, N>& arg, Args&&... args) {
  return N + ssd1306_cmd_size(static_cast<Args>(args)...);
}

template <size_t N>
static void ssd1306_cmd_pack(uint8_t* buf, array<uint8_t, N> arg) {
  memcpy(buf, arg.data(), arg.size());
}

template <size_t N, typename... Args>
static void ssd1306_cmd_pack(uint8_t* buf, array<uint8_t, N> arg, Args... args) {
  memcpy(buf, arg.data(), arg.size());
  ssd1306_cmd_pack(buf + arg.size(), args...);
}

template <typename... Args>
static bool ssd1306_command(Args... commands) {
  uint8_t buf[ssd1306_cmd_size(commands...) + 1];
  buf[0] = 0x00;
  ssd1306_cmd_pack(buf + 1, commands...);

  int rc = i2c_write(i2c_device, buf, sizeof(buf), display_addr);
  return rc == 0;
}

static bool ssd1306_data(span<uint8_t> bytes) {
  struct i2c_msg msgs[2];
  uint8_t write = 0x40;
  msgs[0].buf = &write;
  msgs[0].len = 1;
  msgs[0].flags = I2C_MSG_WRITE;

  msgs[1].buf = bytes.data();
  msgs[1].len = bytes.size();
  msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

  int rc = i2c_transfer(i2c_device, msgs, 2, display_addr);
  return rc == 0;
}

struct Framebuffer {
  uint8_t buffer[512];
};

struct Display {
  Display() { memset(first_.buffer, 0, sizeof(first_.buffer)); }

  void set_row(size_t row_idx, const char* line) {
    dirty_ = true;

    uint8_t* buf = current_buffer()->buffer;

    // We use 126 columns for the text, but the logo takes the full 128.
    // Leave two empty columns at the beginning to approximately center things.
    buf[row_idx] = 0;
    buf[row_idx + 4] = 0;
    for (size_t column_idx = 0; column_idx < 21; ++column_idx) {
      size_t pixel_idx = column_idx * 6 + 2;
      char character = 0x20;
      if (line) {
        if (*line) {
          character = *line++;
        } else {
          line = nullptr;
        }
      }

      if (character < 0x20 || character > 0x7f) {
        character = 0x20;
      }

      const uint8_t* data = &font[5 * (character - 0x20)];
      for (size_t i = 0; i < 5; ++i) {
        buf[(pixel_idx + i) * 4 + row_idx] = data[i];
      }

      // Space between columns.
      buf[(pixel_idx + 5) * 4 + row_idx] = 0;
    }
  }

  void draw_logo() {
    dirty_ = true;

    uint8_t* buf = current_buffer()->buffer;
    const uint8_t* logo = display_logo;

    for (int x = 0; x < 128; ++x) {
      for (int y = 0; y < 3; ++y) {
        buf[x * 4 + y] = *logo++;
      }
    }
  }

  void blit() {
    if (dirty_) {
      // TODO: Double buffer?
      dirty_ = false;
      ssd1306_data(span(current_buffer()->buffer, 512));
    }
  }

  Framebuffer* current_buffer() { return &first_; }

 private:
  Framebuffer first_;
  uint8_t buffer_idx_;
  bool dirty_;
};

static Display display;
static bool initialized;

K_THREAD_STACK_DEFINE(ssd1306_stack, 512);
static struct k_work_q ssd1306_work_q;
static struct k_work ssd1306_blit_work;

bool ssd1306_init() {
  // TODO: Do this asynchronously.
  i2c_device = device_get_binding(DT_LABEL(DT_ALIAS(display_i2c)));

  initialized = false;

  for (int i = 0; i < 50; ++i) {
    // clang-format off
    initialized = ssd1306_command(
      ssd1306_set_multiplex_ratio(32),
      ssd1306_set_display_offset(0),
      ssd1306_set_display_start_line(0),
      ssd1306_set_segment_remap(true),
      ssd1306_set_com_output_scan_direction(true),
      ssd1306_set_com_pins(PinConfiguration::Sequential, LeftRightRemap::Disabled),
      ssd1306_set_entire_display_on(false),
      ssd1306_set_inverse(false),
      ssd1306_set_display_clock(1, 128),
      ssd1306_set_precharge_period(1, 15),
      ssd1306_set_enable_charge_pump(true),
      ssd1306_set_memory_addressing_mode(MemoryAddressingMode::Vertical),
      ssd1306_set_contrast(0xff),
      ssd1306_set_column_address(0, 127),
      ssd1306_set_page_address(0, 3)
    );
    // clang-format on

    if (initialized) {
      new (&display) Display();

      k_work_q_start(&ssd1306_work_q, ssd1306_stack, K_THREAD_STACK_SIZEOF(ssd1306_stack), 1);
      k_work_init(&ssd1306_blit_work, [](struct k_work*) { display.blit(); });

      display.blit();
      ssd1306_command(ssd1306_set_display_on(true));
      break;
    }

    LOG_ERR("failed to initialize display, retrying");
    k_sleep(K_MSEC(100));
  }

  if (initialized) {
    LOG_INF("display successfully initialized");
  } else {
    LOG_ERR("giving up on display initialization");
  }

  return initialized;
}

void display_set_line(size_t row_idx, const char* line) {
  if (initialized) {
    display.set_row(row_idx, line);
  }
}

void display_draw_logo() {
  if (initialized) {
    display.draw_logo();
  }
}

void display_blit() {
  if (initialized) {
    k_work_submit(&ssd1306_blit_work);
  }
}
