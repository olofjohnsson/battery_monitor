#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize GPIO pins and configure the LED.
 *
 * This function configures the LED pin as an output and sets its initial state to inactive (led on). 
 * It also turns off the LED after a short delay.
 */
void init_pins(void);

/**
 * @brief Blink the LED a specified number of times.
 *
 * @param number_of_blinks The number of times the LED should toggle between on and off states.
 *
 * This function toggles the LED's state with a delay between each toggle, creating a blinking effect.
 */
void blink_led(uint8_t number_of_blinks);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
