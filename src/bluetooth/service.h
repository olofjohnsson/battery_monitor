#ifndef SERVICE_H
#define SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/**
 * @file service.h
 * @brief Bluetooth Service and Characteristic UUID definitions and functions.
 *
 * This header file defines the UUIDs for the custom Battery Service and its characteristics,
 * along with the prototypes for functions to send data (voltage and temperature) as notifications.
 */

/** @brief Battery Service UUID. 
 *
 * This is the UUID for the custom Battery Service, used to identify the service
 * in the Bluetooth GATT server.
 */
#define BT_UUID_BATTERY_VAL \
    BT_UUID_128_ENCODE(0x00001000, 0x1010, 0xefde, 0x1000, 0x785feabcd123)

/** @brief Voltage Characteristic UUID. 
 *
 * This is the UUID for the Voltage characteristic within the Battery Service.
 * It allows reading the voltage value and enabling notifications for updates.
 */
#define BT_UUID_VOLTAGE_VAL \
    BT_UUID_128_ENCODE(0x00001001, 0x1010, 0xefde, 0x1000, 0x785feabcd123)

/** @brief Temperature Characteristic UUID. 
 *
 * This is the UUID for the Temperature characteristic within the Battery Service.
 * It allows reading the temperature value and enabling notifications for updates.
 */
#define BT_UUID_TEMP_VAL \
    BT_UUID_128_ENCODE(0x00001002, 0x1010, 0xefde, 0x1000, 0x785feabcd123)

/** @brief Declaration of Battery Service UUID. */
#define BT_UUID_BATTERY       BT_UUID_DECLARE_128(BT_UUID_BATTERY_VAL)
/** @brief Declaration of Voltage Characteristic UUID. */
#define BT_UUID_VOLTAGE       BT_UUID_DECLARE_128(BT_UUID_VOLTAGE_VAL)
/** @brief Declaration of Temperature Characteristic UUID. */
#define BT_UUID_TEMP          BT_UUID_DECLARE_128(BT_UUID_TEMP_VAL)

/**
 * @brief Send a voltage reading via notification.
 *
 * This function sends a voltage value as a notification to the connected client,
 * if notifications are enabled for the Voltage characteristic.
 *
 * @param voltage The voltage value to send.
 * @return 0 on success, or a negative error code if notifications are not enabled or fail.
 */
int bt_send_voltage(uint32_t voltage);

/**
 * @brief Send a temperature reading via notification.
 *
 * This function sends a temperature value as a notification to the connected client,
 * if notifications are enabled for the Temperature characteristic.
 *
 * @param temp The temperature value to send.
 * @return 0 on success, or a negative error code if notifications are not enabled or fail.
 */
int bt_send_temp(uint32_t temp);

int bt_send_csv(const char *csv_data);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_H */
