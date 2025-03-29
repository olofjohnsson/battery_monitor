/**
 * @file main_voltage.c
 * @brief ADC handling and voltage measurement module.
 *
 * This module initializes the ADC, reads voltage samples periodically, converts the raw ADC
 * values into meaningful voltage readings in millivolts (mV), and transmits the results via
 * Bluetooth.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <hal/nrf_power.h>

#include "../sensor/main_voltage.h"
#include "../bluetooth/bluetooth.h"
#include "../bluetooth/service.h"
#include "../hardware/mux.h"
#include <bluetooth/services/nus.h>

#define NUMBER_OF_BATTERIES_IN_SERIES 20
#define NUMBER_OF_MUXES 2
#define NUMBER_OF_MUX_CHANNELS 16
#define TOTAL_CHANNELS (NUMBER_OF_MUXES * NUMBER_OF_MUX_CHANNELS)

typedef struct {
    int64_t timestamp;
    uint16_t adc_values[TOTAL_CHANNELS];  /* Adjusted size to store all channels */
} adc_sample_t;

#define MAX_SAMPLES 16

static adc_sample_t samples[MAX_SAMPLES];
static uint8_t sample_index = 0;

// Constants and configurations
#define ADC_DEVICE_NAME DT_NODE_FULL_NAME(DT_NODELABEL(adc)) ///< ADC device node label
#define R1              240000                              ///< Voltage divider resistor R1 (in ohms)
#define R2              10000                               ///< Voltage divider resistor R2 (in ohms)
#define ADC_REF_MV      3300                                ///< Reference voltage in mV
#define ADC_RESOLUTION  1024                                ///< ADC resolution (10-bit)

#define ADC_SAMPLE_INTERVAL	20 ///< Sampling interval in milliseconds
#define BATTERY_VOLTAGE(sample) (sample * 6 * 600 / 1024) ///< Macro for calculating battery voltage

// ADC configuration
const struct device *adc_dev;
static int16_t adc_buffer[1];
static uint8_t error_debug = 100;

static struct adc_sequence sequence = {
	.options        = NULL,
	.channels       = BIT(0),
	.buffer         = adc_buffer,
	.buffer_size    = sizeof(adc_buffer),
	.resolution     = 10,
	.oversampling   = 0,
	.calibrate      = false,
};

/**
 * @brief Convert raw ADC value to millivolts (mV).
 *
 * This function calculates the input voltage based on the raw ADC reading, considering the
 * reference voltage and the voltage divider circuit.
 *
 * @param adc_value Raw ADC reading.
 * @return Calculated voltage in millivolts (mV).
 */
int32_t convert_adc_to_mv(uint16_t adc_value)
{
    int32_t v_adc_mv = (adc_value * ADC_REF_MV) / ADC_RESOLUTION; // ADC value to mV
    int32_t v_in_mv = v_adc_mv * (R1 + R2) / R2;                 // Adjust using voltage divider
    return v_in_mv;
}

/**
 * @brief Perform a single ADC sample and transmit the result via Bluetooth.
 *
 * This function reads a single ADC sample, converts it to millivolts, and transmits the voltage
 * via Bluetooth. If an error occurs, an error code is transmitted instead.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int adc_sample(void)
{
	int err = adc_read(adc_dev, &sequence);
	if (err) {
		bt_send_voltage(3);
		k_sleep(K_MSEC(2000));
        bt_send_voltage(err);
		return err;
	}
	bt_send_voltage(convert_adc_to_mv(adc_buffer[0]) / 100); // Send voltage in 0.1V units
	return 0;
}

// ADC channel configuration
static const struct adc_channel_cfg ch0_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_0));

/**
 * @brief Initialize the ADC
 *
 * This function sets up the ADC device, and configures the ADC channel
 * @return 0 on success, or a negative error code on failure.
 */
int init_adc(void)
{
	int err;

	adc_dev = device_get_binding(ADC_DEVICE_NAME);
	error_debug = 101;
	if (!adc_dev) {
        bt_send_voltage(11); // Send error code via Bluetooth
		return -EIO;
	}

	err = adc_channel_setup(adc_dev, &ch0_cfg_dt);
	error_debug = 102;
	if (err) {
        bt_send_voltage(13); // Send error code via Bluetooth
		return err;
	}

	error_debug = 103;

	return 0;
}

int format_csv(char *buffer, size_t buf_size) {
    if (buffer == NULL || samples == NULL || buf_size == 0) {
        return -1; // Error: Invalid input
    }

    size_t offset = 0;

    // Write CSV header dynamically
    offset += snprintf(buffer + offset, buf_size - offset, "Timestamp");
    for (uint8_t i = 1; i <= NUMBER_OF_BATTERIES_IN_SERIES; i++) {
        offset += snprintf(buffer + offset, buf_size - offset, ",B%d", i);
    }
    offset += snprintf(buffer + offset, buf_size - offset, "\n");

    if (offset >= buf_size) {
        printf("Warning: Buffer too small, header truncated!\n");
        return -2; // Error: Buffer overflow
    }

    // Write each sample
    for (uint8_t j = 0; j < sample_index; j++) { 
        int written = snprintf(buffer + offset, buf_size - offset, "%lld", samples[j].timestamp);

        if (written < 0 || (size_t)written >= buf_size - offset) {
            printf("Warning: Buffer too small, data truncated!\n");
            return -2;
        }
        offset += written;

        // Add ADC values
        for (uint8_t i = 0; i < NUMBER_OF_BATTERIES_IN_SERIES; i++) {
            written = snprintf(buffer + offset, buf_size - offset, ",%d", samples[j].adc_values[i]);

            if (written < 0 || (size_t)written >= buf_size - offset) {
                printf("Warning: Buffer too small, data truncated!\n");
                return -2;
            }

            offset += written;
        }

        // Add newline
        if (offset < buf_size - 1) {
            buffer[offset++] = '\n';
            buffer[offset] = '\0';
        } else {
            printf("Warning: Buffer too small, data truncated!\n");
            return -2;
        }
    }

    return 0; // Success
}

void store_sample(void) {
    int err = adc_read(adc_dev, &sequence);
    int64_t timestamp = k_uptime_get() / 1000;

    if (sample_index < MAX_SAMPLES && err == 0) {
        samples[sample_index].timestamp = timestamp;

        for (uint8_t mux = 0; mux < NUMBER_OF_MUXES; mux++) {
            for (uint8_t channel = 0; channel < NUMBER_OF_MUX_CHANNELS; channel++) {
                adc_read(adc_dev, &sequence);
                set_mux_channel(mux, channel);
                k_sleep(K_USEC(50));  /* Allow settling */
                
                // Ensure adc_buffer is properly used
                samples[sample_index].adc_values[mux * NUMBER_OF_MUX_CHANNELS + channel] = adc_buffer[0];
            }
        }
        sample_index++;
    }
}

void attempt_send() {
    int err = 0;
    char csv_buffer[1024];

    format_csv(csv_buffer, sizeof(csv_buffer));

    err = bt_nus_send(NULL, csv_buffer, strlen(csv_buffer));

    if (!err) {
        sample_index = 0;
    } else {
        printf("Error: bt_nus_send failed with code %d\n", err);
    }
}
