#ifndef GAMECUBE_H
#define GAMECUBE_H
#include <zephyr.h>
#include <include/drivers/gpio.h>
#define getPinState() gpio_pin_get_raw(port,pin)
#define setPinState(state) gpio_pin_set_raw(port,pin,state)
#define pinMode(flags) gpio_pin_configure(port,pin,flags)
#define HIGH_PIN_LOW_NS 3
#define STOP_PIN_LOW_NS 2
#define LOW_PIN_LOW_NS 1
#define HIGH 1
#define LOW 0
#define CMD_PROBE 0x00
#define CMD_ORIGIN 0x41
#define CMD_POLL 0x40
#define CMD_RST 0xFF





class GameCube {
public:
    GameCube(device* data_port,gpio_pin_t data_pin);
    void init();
    ssize_t run();
    void loop();



private:
    gpio_pin_t pin;
    device *port;

    static inline uint64_t get_timing_ns(timing_t start, timing_t end) {
        return timing_cycles_to_ns(timing_cycles_get(&start,&end));
    }

    ssize_t handle_cmd()
    bool read_bit();
    void write_bit(bool wb);
    void write_stop();

}
#endif
