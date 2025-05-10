/**
 * @file main_voltage.c
 * @brief ADC handling and voltage measurement module.
 *
 * This module initializes the ADC, reads voltage samples periodically, converts the raw ADC
 * values into meaningful voltage readings in centivolts (cV), and transmits the results via
 * Bluetooth.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <hal/nrf_power.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>

#include "../sensor/main_voltage.h"
#include "../bluetooth/bluetooth.h"
#include "../bluetooth/service.h"
#include "../hardware/mux.h"
#include <bluetooth/services/nus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main_voltage);  // Use your module name

#define NUMBER_OF_BATTERIES_IN_SERIES 5
#define NUMBER_OF_MUXES 2
#define NUMBER_OF_MUX_CHANNELS 4
#define TOTAL_CHANNELS (NUMBER_OF_MUXES * NUMBER_OF_MUX_CHANNELS)

typedef struct {
    int64_t timestamp;
    uint16_t adc_values[TOTAL_CHANNELS];  /* Adjusted size to store all channels */
} adc_sample_t;

#define MAX_SAMPLES 128

static adc_sample_t samples[MAX_SAMPLES];
static uint8_t sample_index = 0;

// Constants and configurations
#define ADC_DEVICE_NAME DT_NODE_FULL_NAME(DT_NODELABEL(adc)) ///< ADC device node label
#define R1              240000                              ///< Voltage divider resistor R1 (in ohms)
#define R2              10000                               ///< Voltage divider resistor R2 (in kohms)
#define ADC_REF_CV      330                                ///< Reference voltage in cV (centi volts)
#define ADC_RESOLUTION  1024                                ///< ADC resolution (10-bit)

#define ADC_SAMPLE_INTERVAL	20 ///< Sampling interval in milliseconds
#define BATTERY_VOLTAGE(sample) (sample * 6 * 600 / 1024) ///< Macro for calculating battery voltage

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(nvs_storage)
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(nvs_storage)

#define ADDRESS_ID 1
#define KEY_ID 2

// ADC configuration
const struct device *adc_dev;
static uint32_t adc_buffer[1];
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
 * @brief Convert raw ADC value to scaled voltage (x100). Example 100 --> 1 V
 *
 * This function calculates the input voltage based on the raw ADC reading, considering the
 * reference voltage and the voltage divider circuit.
 *
 * @param adc_value Raw ADC reading.
 * @return Calculated scaled voltage
 */
uint16_t convert_adc_to_scaled_voltage(uint32_t adc_value)
{
    uint32_t v_adc = ((uint32_t)adc_value * ADC_REF_CV) / ADC_RESOLUTION; // ADC value to cV (centi volts)
    uint16_t v_in = v_adc * (R1 + R2) / R2;                 // Adjust using voltage divider
    return v_in;
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
	bt_send_voltage(convert_adc_to_scaled_voltage(adc_buffer[0])); // Send voltage in 0.1V units
	return 0;
}

static struct nvs_fs fs;

#define FLASH_OFFSET 0xFE000  // adjust based on available space in flash
#define FLASH_SECTOR_SIZE 4096  // common sector size, check your flash definition

void flash_init(void)
{
    printk("Flash init\n");
    int rc;
    char debug_buf[128];
    char buf[16];
    const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash));
    struct flash_pages_info info;

    /* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors7
	 *	starting at NVS_PARTITION_OFFSET
	 */
	fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
        printk("Flash device %s is not ready\n", fs.flash_device->name);
        snprintf(debug_buf, sizeof(debug_buf), "Flash device %s is not ready\n", fs.flash_device->name);
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
		return 0;
	}
	fs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
        snprintf(debug_buf, sizeof(debug_buf), "Unable to get page info\n");
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
		return 0;
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

    snprintf(debug_buf, sizeof(debug_buf), "Offset: 0x%x, Sector size: %u, Sector count: %u", fs.offset, fs.sector_size, fs.sector_count);
    bt_nus_send(NULL, debug_buf, strlen(debug_buf));

    if ((fs.offset % info.size) != 0)
    {
        bt_nus_send(NULL, "NVS offset is not aligned to page size\n", 39);
        return 0;
    }

    snprintf(debug_buf, sizeof(debug_buf), "info.size %d. name: %d", info.size, fs.flash_device->name);
    bt_nus_send(NULL, debug_buf, strlen(debug_buf));

	rc = nvs_mount(&fs);
	if (rc) {
        snprintf(debug_buf, sizeof(debug_buf), "NVS mount failed: %d\n", rc);
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
    } else {
        snprintf(debug_buf, sizeof(debug_buf), "NVS mounted successfully at offset 0x%x\n. NVS ready: %d", fs.offset, fs.ready);
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
    }

    /* ADDRESS_ID is used to store an address, lets see if we can
	 * read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	rc = nvs_read(&fs, ADDRESS_ID, &buf, sizeof(buf));
	if (rc > 0) { /* item was found, show it */
        printk("Id: %d, Address: %s\n. rc = %d", ADDRESS_ID, buf, rc);
        snprintf(debug_buf, sizeof(debug_buf), "Id: %d, Address: %s\n. rc = %d", ADDRESS_ID, buf, rc);
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
	}
    else
    { /* item was not found, add it */
		strcpy(buf, "192.168.1.1");
        printk("No address found, adding %s at id %d\n", buf,
		       ADDRESS_ID);
        snprintf(debug_buf, sizeof(debug_buf), "No address found, adding %s at id %d\n", buf,
		       ADDRESS_ID);
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
		rc = nvs_write(&fs, ADDRESS_ID, &buf, strlen(buf)+1);
        printk("Writing result, rc = %d", rc);
        snprintf(debug_buf, sizeof(debug_buf), "Writing result, rc = %d", rc);
        bt_nus_send(NULL, debug_buf, strlen(debug_buf));
	}
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

int format_csv_nvs(char *buffer, size_t buf_size) {
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
                samples[sample_index].adc_values[mux * NUMBER_OF_MUX_CHANNELS + channel] = convert_adc_to_scaled_voltage(adc_buffer[0]);
            }
        }
        sample_index++;
    }
}

void store_sample_nvs(void) {
    int err = adc_read(adc_dev, &sequence);
    int64_t timestamp = k_uptime_get() / 1000;
    char debug_buf[128];

    if (sample_index < MAX_SAMPLES && err == 0) {
        samples[sample_index].timestamp = timestamp;

        for (uint8_t mux = 0; mux < NUMBER_OF_MUXES; mux++) {
            for (uint8_t channel = 0; channel < NUMBER_OF_MUX_CHANNELS; channel++) {
                set_mux_channel(mux, channel);
                k_sleep(K_USEC(50));  // Allow settling

                adc_read(adc_dev, &sequence);
                samples[sample_index].adc_values[mux * NUMBER_OF_MUX_CHANNELS + channel] =
                    convert_adc_to_scaled_voltage(adc_buffer[0]);
            }
        }

        int rc = nvs_write(&fs, sample_index, &samples[sample_index], sizeof(samples[sample_index])+1);
        if (rc >= 0)
        {
            snprintf(debug_buf, sizeof(debug_buf), "Stored sample %d\n", sample_index);
            bt_nus_send(NULL, debug_buf, strlen(debug_buf));
            sample_index++;
        }
        else
        {
            snprintf(debug_buf, sizeof(debug_buf), "Failed to store sample: %d\n", rc);
            bt_nus_send(NULL, debug_buf, strlen(debug_buf));
        }
    }
    
    snprintf(debug_buf, sizeof(debug_buf), "Sample index: %d\n", sample_index);
    bt_nus_send(NULL, debug_buf, strlen(debug_buf));
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

void load_samples_from_nvs(void) {
    sample_index = 0;
    while (sample_index < MAX_SAMPLES) {
        int rc = nvs_read(&fs, sample_index, &samples[sample_index], sizeof(samples[sample_index]));
        if (rc <= 0) {
            break;  // No more samples
        }
        sample_index++;
    }
}

void nvs_debug()
{
    char debug_buf[128];
    flash_init();
    struct sample {
        int64_t timestamp;
        int16_t adc_values[20];  // adjust for your number of channels
    };
    
    struct sample s = {
        .timestamp = k_uptime_get(),
    };
    for (int i = 0; i < 20; i++) {
        s.adc_values[i] = i * 100;
    }
    int32_t value = 1234;
    const void *data = &value;
    int16_t id = 1;
    int rc = nvs_write(&fs, id, &data, sizeof(data)+1);
    //int rc = nvs_write(&fs, 5, &s, sizeof(s));
    snprintf(debug_buf, sizeof(debug_buf), "fsready = %d\n", fs.ready);
    bt_nus_send(NULL, debug_buf, strlen(debug_buf));
    printk("Write rc = %d\n", rc);
    snprintf(debug_buf, sizeof(debug_buf), "Write rc = %d\n", rc);
    bt_nus_send(NULL, debug_buf, strlen(debug_buf));


    struct sample s_read;
    int32_t dummy_read = 0;
    rc = nvs_read(&fs, 1, &dummy_read, sizeof(dummy_read));
    //rc = nvs_read(&fs, 5, &s_read, sizeof(s_read));
    //printk("Read rc = %d, ts = %lld, val[0] = %d\n", rc, s_read.timestamp, s_read.adc_values[0]);
    printk("Read rc: %d, val: %d\n", rc, dummy_read);
    snprintf(debug_buf, sizeof(debug_buf), "Read rc: %d, val: %d\n", rc, dummy_read);
    bt_nus_send(NULL, debug_buf, strlen(debug_buf));

    
}