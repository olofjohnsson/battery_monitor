#include <nrfx_temp.h>
#include <stdint.h>
#include <zephyr/kernel.h>

/**
 * @file temperature.c
 * @brief Functions for initializing and reading the temperature sensor.
 *
 * This file provides functionality for configuring and reading data from the temperature sensor
 * using the Nordic Semiconductor nrfx_temp driver.
 */

// Define the configuration for the temperature sensor
nrfx_temp_config_t temp_config = NRFX_TEMP_DEFAULT_CONFIG;

/**
 * @brief Read the temperature and convert it to a 32-bit signed integer (in tenths of a degree Celsius).
 *
 * This function starts a temperature measurement, waits for the result to be ready,
 * retrieves the raw temperature value, and converts it to tenths of a degree Celsius.
 *
 * @return The temperature as a 32-bit signed integer in tenths of a degree Celsius (1/10 °C).
 */
int32_t read_temperature_int(void)
{
    nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START); // Start temperature measurement
    while (!nrf_temp_event_check(NRF_TEMP, NRF_TEMP_EVENT_DATARDY)) {
        k_sleep(K_MSEC(1));  // Wait for the measurement to be ready
    }

    nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY); // Clear the event
    int32_t raw_temp = nrf_temp_result_get(NRF_TEMP);      // Get the raw temperature value
    nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_STOP);  // Stop the temperature measurement

    // Convert the raw temperature to tenths of a degree Celsius (1/10 °C)
    return raw_temp * 4;  // Return as a 32-bit signed integer in tenths of a degree Celsius
}

/**
 * @brief Initialize the temperature sensor.
 *
 * This function initializes the temperature sensor with the default configuration.
 * It must be called before attempting to read the temperature.
 *
 * @return 0 on success, or a negative error code if initialization fails.
 */
int init_temp(void)
{
    int err;
    err = nrfx_temp_init(&temp_config, NULL); // Initialize the temperature sensor
    return err;
}