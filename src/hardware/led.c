#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include "led.h"

// Static GPIO descriptor for the LED, defined using a device tree alias (redled).
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led_red), gpios);


/**
 * @brief Initialize GPIO pins and set the LED to an inactive state.
 *
 * This function configures the LED pin as an output and sets it to an inactive state. 
 * It then introduces a 1-second delay before turning the LED off.
 */
void init_pins(void)
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE); // Configure LED as output and set it to inactive
    k_sleep(K_MSEC(1000));                            // Sleep for 1 second
    gpio_pin_set_dt(&led, 1);                         // Turn the LED off
}

/**
 * @brief Blink the LED a specified number of times.
 *
 * @param number_of_blinks The number of times to toggle the LED.
 *
 * This function toggles the state of the LED with a 500 ms delay between each toggle.
 */
void blink_led(uint8_t number_of_blinks)
{
    for(uint8_t i = 0; i < number_of_blinks; i++)
    {
        gpio_pin_toggle_dt(&led); // Toggle LED state
        k_sleep(K_MSEC(500));     // Sleep for 500 ms
    }
}
