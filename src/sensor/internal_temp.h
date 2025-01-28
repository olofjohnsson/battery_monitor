/**
 * @file internal_temp.h
 * @brief Interface for reading and managing the internal temperature sensor.
 *
 * This header file provides declarations for functions to initialize and read 
 * the internal temperature sensor. The temperature readings are returned as 
 * signed 32-bit integers in tenths of a degree Celsius (1/10 °C).
 */

#ifndef INTERNAL_TEMP_H
#define INTERNAL_TEMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Read the temperature from the internal sensor.
 *
 * This function retrieves the temperature measurement from the internal temperature sensor,
 * converts the raw reading into tenths of a degree Celsius, and returns it as a 32-bit signed integer.
 *
 * @return The temperature in tenths of a degree Celsius (1/10 °C).
 */
int32_t read_temperature_int(void);

/**
 * @brief Initialize the internal temperature sensor.
 *
 * This function initializes the internal temperature sensor with the required configuration.
 * It must be called before attempting to read the temperature.
 *
 * @return 0 on success, or a negative error code if initialization fails.
 */
int init_temp(void);

#ifdef __cplusplus
}
#endif

#endif /* INTERNAL_TEMP_H */