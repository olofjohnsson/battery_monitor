#include "application.h"
#include "../bluetooth/bluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>
#include "../bluetooth/service.h"
#include "../sensor/main_voltage.h"
#include "../sensor/internal_temp.h"
#include "../hardware/led.h"

/**
 * @brief Main application loop to initialize peripherals and run the core functionality.
 *
 * This function performs the following steps:
 * - Initializes GPIO pins (e.g., LEDs).
 * - Initializes Bluetooth functionality and starts advertising.
 * - Sets up the ADC for voltage sensing.
 * - Initializes the internal temperature sensor.
 * - Continuously reads the internal temperature, sends the temperature data over Bluetooth, 
 *   and introduces a 1-second delay between iterations.
 */
void run_application()
{
    // Initialize the LED GPIO pins
    init_pins();

    // Initialize Bluetooth stack and start advertising
    bluetooth_init();
    bluetooth_start_advertising();

    // Initialize ADC for voltage measurement
    init_adc();

    // Initialize internal temperature sensor
    init_temp();

    // Main loop: Read and send temperature data over Bluetooth
    for (;;) 
    {
        int32_t temp = read_temperature_int();  // Read the temperature in integer format
        bt_send_temp(temp);                    // Send temperature data over Bluetooth
        store_sample();
        attempt_send();
        k_sleep(K_MSEC(1000));                 // Wait for 1 second
    }
}
