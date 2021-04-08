#include "input/socd.h"

// TODO: Store these in flash.
static SOCDType input_socd_type_x = SOCDType::Neutral;
static SOCDType input_socd_type_y = SOCDType::Negative;

SOCDType input_socd_get_x_type() {
  return input_socd_type_x;
}

void input_socd_set_x_type(SOCDType type) {
  input_socd_type_x = type;
}

SOCDType input_socd_get_y_type() {
  return input_socd_type_y;
}

void input_socd_set_y_type(SOCDType type) {
  input_socd_type_y = type;
}

StickOutput::Axis input_socd_parse(SOCDType type, span<SOCDInputs> inputs) {
  optional<uint64_t> newest_positive;
  optional<uint64_t> newest_neutral;
  optional<uint64_t> newest_negative;

  for (auto& input : inputs) {
    if (input.input_value) {
      if (input.overrides) {
        return StickOutput::Axis {
          .value = static_cast<int>(input.button_type),
          .tick = input.input_tick,
        };
      }
      optional<uint64_t>* target = nullptr;
      switch (input.button_type) {
        case SOCDButtonType::Positive:
          target = &newest_positive;
          break;

        case SOCDButtonType::Neutral:
          target = &newest_neutral;
          break;

        case SOCDButtonType::Negative:
          target = &newest_negative;
          break;
      }

      if (!*target || **target < input.input_tick) {
        target->reset(input.input_tick);
      }
    }
  }

  switch (type) {
    case SOCDType::Neutral:
      return StickOutput::Axis {
        .value = newest_positive.valid() - newest_negative.valid(),
        .tick = max(newest_positive.get_or(0), newest_negative.get_or(0)),
      };

    case SOCDType::Positive:
      if (newest_positive.valid()) {
        return StickOutput::Axis {
          .value = 1,
          .tick = *newest_positive,
        };
      } else if (newest_negative.valid()) {
        return StickOutput::Axis {
          .value = -1,
          .tick = *newest_negative,
        };
      } else {
        return StickOutput::Axis {
          .value = 0,
          .tick = 0,
        };
      }

    case SOCDType::Negative:
      if (newest_negative.valid()) {
        return StickOutput::Axis {
          .value = -1,
          .tick = *newest_negative,
        };
      } else if (newest_positive.valid()) {
        return StickOutput::Axis {
          .value = 1,
          .tick = *newest_positive,
        };
      } else {
        return StickOutput::Axis {
          .value = 0,
          .tick = 0,
        };
      }

    case SOCDType::Last:
      break;
  }

  struct {
    optional<uint64_t> tick;
    int value;
  } results[] = {
    { newest_positive, 1 },
    { newest_neutral, 0 },
    { newest_negative, -1 },
  };

  optional<uint64_t> newest_tick;
  optional<int> newest_value;
#define X(tick, value)                          \
  if (tick) {                                   \
    if (!newest_tick || *newest_tick < *tick) { \
      newest_tick = *tick;                      \
      newest_value.reset(value);                \
    }                                           \
  }

  X(newest_positive, 1);
  X(newest_neutral, 0);
  X(newest_negative, -1);

  return StickOutput::Axis {
    .value = newest_value.get_or(0),
    .tick = newest_tick.get_or(0),
  };
}
