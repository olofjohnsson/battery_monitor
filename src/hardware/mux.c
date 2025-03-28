/**
 * @file mux.c
 * @brief Implementation file for controlling the CD74HC4067 multiplexer.
 *
 * This file provides the function to set the multiplexer channel by configuring
 * the 4 control pins using values from the device tree.
 *
 * @author [Your Name]
 * @date [Date]
 */

 #include <zephyr/kernel.h>
 #include <zephyr/device.h>
 #include <zephyr/devicetree.h>
 #include <zephyr/drivers/gpio.h>
 #include "mux.h"

#define MUX_A 0
#define MUX_B 1

static const struct gpio_dt_spec mux_a_pins[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_a_0), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_a_1), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_a_2), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_a_3), gpios)
};

static const struct gpio_dt_spec mux_b_pins[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_b_0), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_b_1), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_b_2), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(mux_b_3), gpios)
};

void set_mux_channel(uint8_t mux, uint8_t channel)
{
    const struct gpio_dt_spec *mux_pins = (mux == MUX_A) ? mux_a_pins : mux_b_pins;

    for (int i = 0; i < 4; i++) {
        gpio_pin_set_dt(&mux_pins[i], (channel >> i) & 1);
    }
}