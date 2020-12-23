#include "gamecube.h"
#include <zephyr.h>
#include <timing/timing.h>
#include <include/drivers/gpio.h>
#define OUTPUT_SIZE 10
union __attribute__((packed)) OutputReport {
    struct {
        uint16_t button_a : 1;
        uint16_t button_b : 1;
        uint16_t button_x : 1;
        uint16_t button_y : 1;
        uint16_t button_start : 1;
        uint16_t orig: 1;
        uint16_t err_l: 1;
        uint16_t err_s: 1;
        uint16_t dp_l:1;
        uint16_t dp_r:1;
        uint16_t dp_d:1;
        uint16_t dp_u:1;
        uint16_t button_z : 1;
        uint16_t button_r : 1;
        uint16_t button_l : 1;
        uint16_t unknown_1: 1;


        uint8_t left_stick_x;
        uint8_t left_stick_y;
        uint8_t right_stick_x;
        uint8_t right_stick_y;
        uint8_t l_x;
        uint8_t r_x;


        uint8_t magic_1;
        uint8_t magic_2;
    } buttons;
    uint8_t [OUTPUT_SIZE]buf;
};

static_assert(sizeof(OutputReport.buttons) == OUTPUT_SIZE);

static uint8_t id[3]= {0x09,0x00,0x00}


GameCube::GameCube(device *data_port,gpio_pin_t data_pin) {
    pin = data_pin
}


void GameCube::init() {
    timing_init();
    timing_start();
    pinMode(GPIO_INPUT);
    timing_t start = timing_counter_get()

                     k_msleep(100);
    uint16_t buffer=0xffff;
    while (buffer!=0x201) {
        buffer = (buffer << 1) | readBit();
    }
}

ssize_t GameCube::run() {
    return handle_cmd(read_cmd());
}

void GameCube::loop() {
    while (run() >=0);
}

bool GameCube::read_bit() {
    while(getPinState()) {
        // wait for falling edge
    }
    timing_t start = timing_counter_get();
    while(!getPinState()) {
        // wait for rising edge
    }
    timing_t end = timing_counter_get();
    return get_timing_ns(start,end) > 2;
}

bool GameCube::write_bit(bool wb) {
    timing_t start = timing_counter_get();
    if (wb) {
        setPinState(LOW);
        while (get_timing_ns(start,timing_counter_get())<1) {
            // write low for 1ns
        }
        start = timing_counter_get();
        setPinState(HIGH);
        while (get_timing_ns(start,timing_counter_get())<3) {
            // write high for 3ns
        }
        return;
    } else {
        setPinState(LOW);
        while (get_timing_ns(start,timing_counter_get())<3) {
            // write low for 3ns
        }
        start = timing_counter_get();
        setPinState(HIGH);
        while (get_timing_ns(start,timing_counter_get())<1) {
            // write high for 1ns
        }
        return;
    }
}

bool GameCube::write_stop() {
    timing_t start = timing_counter_get();
    setPinState(LOW);
    while (get_timing_ns(start,timing_counter_get())<2) {
        // write low for 1ns
    }
    start = timing_counter_get();
    setPinState(HIGH);
    while (get_timing_ns(start,timing_counter_get())<2) {
        // write high for 3ns
    }
    return;
}

void write_byte(uint8_t wb) {
    for (uint8_t i = 0; i < 8; i++) {
        write_bit(wb & 0x80);
        wb <<= 1;
    }
}


uint8_t GameCube::read_cmd() {
    pinMode(GPIO_INPUT);
    uint8_t cmd;
    for (uint8_t i = 0; i < 8; i++) {
        cmd = (cmd << 1) | readBit();
    }
    if (cmd.command==CMD_ORIGIN||cmd==CMD_PROBE||cmd== CMD_RST) {
        readBit();			// stop bit
        return cmd;
    }
    else {
        // throw out un-needed data such as rumble info
        for (uint8_t i = 0; i < 16; i++) {
            readBit();
        }
        readBit();			// stop bit
        return cmd;
    }
}


ssize_t GameCube::handle_cmd(uint8_t cmd) {
    pinMode(GPIO_OUTPUT);
    if (cmd == CMD_PROBE||cmd == CMD_RST ) {

        for (uint8_t i = 0; i<3; i++) {
            write_byte(id[i]);
        }
        write_stop();

    } else if (cmd == CMD_ORIGIN|| cmd == CMD_POLL) {

        InputState input;
        if (!input_get_state(&input)) {
            LOG_ERR("failed to get InputState");
            return -1;
        }

        OutputReport output = {};
        output.buttons.left_stick_x = input.left_stick_x;
        output.buttons.left_stick_y = input.left_stick_y;
        output.buttons.right_stick_x = input.right_stick_x;
        output.buttons.right_stick_y = input.right_stick_y;
        switch (static_cast<StickState>(input.dpad)) {
        case StickState::North:
            output.buttons.dp_l=0;
            output.buttons.dp_r=0;
            output.buttons.dp_d=0;
            output.buttons.dp_u=1;
            break;
        case StickState::NorthEast:
            output.buttons.dp_l=0;
            output.buttons.dp_r=1;
            output.buttons.dp_d=0;
            output.buttons.dp_u=1;
            break;
        case StickState::East:
            output.buttons.dp_l=0;
            output.buttons.dp_r=1;
            output.buttons.dp_d=0;
            output.buttons.dp_u=0;
            break;
        case StickState::SouthEast:
            output.buttons.dp_l=0;
            output.buttons.dp_r=1;
            output.buttons.dp_d=1;
            output.buttons.dp_u=0;
            break;
        case StickState::South:
            output.buttons.dp_l=0;
            output.buttons.dp_r=0;
            output.buttons.dp_d=1;
            output.buttons.dp_u=0;
            break;
        case StickState::SouthWest:
            output.buttons.dp_l=1;
            output.buttons.dp_r=0;
            output.buttons.dp_d=1;
            output.buttons.dp_u=0;
            break;
        case StickState::West:
            output.buttons.dp_l=1;
            output.buttons.dp_r=0;
            output.buttons.dp_d=0;
            output.buttons.dp_u=0;
            break;
        case StickState::NorthWest:
            output.buttons.dp_l=1;
            output.buttons.dp_r=0;
            output.buttons.dp_d=0;
            output.buttons.dp_u=1;
            break;
        case StickState::Neutral:
            output.buttons.dp_l=0;
            output.buttons.dp_r=0;
            output.buttons.dp_d=0;
            output.buttons.dp_u=0;
            break;
        default:
            LOG_ERR("invalid stick state: %d", static_cast<int>(input.dpad));
            return -1;
        }

        output.buttons.button_x = input.button_north;
        output.buttons.button_y = input.button_east;
        output.buttons.button_a = input.button_south;
        output.buttons.button_b = input.button_west;
        output.buttons.button_l = input.button_l2;
        output.buttons.button_z = input.button_r1;
        output.buttons.button_r = input.button_r2;
        output.buttons.button_start = input.button_start;
        output.buttons.magic1 = 0x02;
        output.buttons.magic2 = 0x02;
        output.buttons.high = 0b1;
        output.buttons.errL = 0b0;
        output.buttons.errS = 0b0;
        output.buttons.orig = 0b0;

        for (uint8_t i = 0; i<OUTPUT_SIZE; i++) {
            write_byte(output.buf[i]);
        }
        write_stop();
    }

    else {
        LOG_ERR("handle_cmd called for unknown cmd 0x%02X", cmd);
        return -1;
    }
}
