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
#include <bluetooth/services/nus.h>

typedef struct {
    int64_t timestamp;
    int32_t adc_value;
} adc_sample_t;

#define MAX_SAMPLES 8192

static adc_sample_t samples[MAX_SAMPLES];
static uint8_t sample_index = 0;

// Constants and configurations
#define ADC_DEVICE_NAME DT_NODE_FULL_NAME(DT_NODELABEL(adc)) ///< ADC device node label
#define ADC_THREAD_STACK_SIZE 1024                          ///< Stack size for the ADC thread
#define ADC_THREAD_PRIORITY   5                             ///< Priority for the ADC thread
#define ADC_SAMPLE_INTERVAL_MS 500                          ///< Sampling interval in milliseconds
#define R1              240000                              ///< Voltage divider resistor R1 (in ohms)
#define R2              10000                               ///< Voltage divider resistor R2 (in ohms)
#define ADC_REF_MV      3300                                ///< Reference voltage in mV
#define ADC_RESOLUTION  1024                                ///< ADC resolution (10-bit)

K_THREAD_STACK_DEFINE(adc_thread_stack, ADC_THREAD_STACK_SIZE);
struct k_thread adc_thread_data;

#define ADC_SAMPLE_INTERVAL	20 ///< Sampling interval in milliseconds
#define BATTERY_VOLTAGE(sample) (sample * 6 * 600 / 1024) ///< Macro for calculating battery voltage

// ADC configuration
const struct device *adc_dev;
static int16_t adc_buffer[3];
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

/**
 * @brief ADC sampling thread.
 *
 * This thread periodically reads ADC samples, processes the results, and handles errors if they occur.
 *
 * @param arg1 Unused.
 * @param arg2 Unused.
 * @param arg3 Unused.
 */
static void adc_thread(void *arg1, void *arg2, void *arg3)
{
    uint16_t err = 0;

    while (1) {
        k_sleep(K_MSEC(ADC_SAMPLE_INTERVAL_MS)); // Wait for the next sampling interval
        err = adc_sample();
        if (err) {
            // Handle errors (optional)
        }
    }
}

// ADC channel configuration
static const struct adc_channel_cfg ch0_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_0));

/**
 * @brief Initialize the ADC and start the sampling thread.
 *
 * This function sets up the ADC device, configures the ADC channel, and creates a thread
 * to periodically read ADC samples.
 *
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

    // Start the ADC sampling thread
    // k_thread_create(&adc_thread_data, adc_thread_stack,
	// 	K_THREAD_STACK_SIZEOF(adc_thread_stack),
	// 	adc_thread,
	// 	NULL, NULL, NULL,
	// 	ADC_THREAD_PRIORITY, 0, K_NO_WAIT);
	return 0;
}

void format_csv(char *buffer, size_t buf_size) {
    if (buffer == NULL || samples == NULL || buf_size == 0) {
        return; // Prevent null pointer issues
    }

    // Write CSV header
    size_t offset = snprintf(buffer, buf_size, "Timestamp,ADC_Value\n");

    // Ensure snprintf didn't truncate
    if (offset >= buf_size) {
        buffer[buf_size - 1] = '\0';
        printf("Warning: Buffer too small, header truncated!\n");
        return;
    }

    // Write each sample
    for (uint8_t i = 0; i < sample_index; i++) {
        int written = snprintf(buffer + offset, buf_size - offset, "%lld,%d\n", 
                               samples[i].timestamp, samples[i].adc_value);

        if (written < 0 || offset + written >= buf_size) {
            buffer[buf_size - 1] = '\0';
            printf("Warning: Buffer too small, data truncated!\n");
            break;
        }

        offset += written;
    }

    // Ensure null-termination
    buffer[buf_size - 1] = '\0';
}

void store_sample(void)
{
	int err = adc_read(adc_dev, &sequence);
	int64_t timestamp = k_uptime_get()/1000;
	int32_t adc_value = convert_adc_to_mv(adc_buffer[0]) / 100;

	if (sample_index < MAX_SAMPLES && err == 0) {
		samples[sample_index].timestamp = timestamp;
		samples[sample_index].adc_value = adc_value;
		sample_index++;
	}
}

void attempt_send()
{
	int err = 0;
	char csv_buffer[1024];
	format_csv(csv_buffer, sizeof(csv_buffer));
	err = bt_nus_send(NULL, csv_buffer, strlen(csv_buffer));
	if (!err)
	{
		sample_index = 0;
	}
    
}