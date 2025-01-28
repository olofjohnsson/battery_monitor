#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "service.h"

static bool notify_enabled;

/**
 * @brief Callback function to handle changes in Client Characteristic Configuration (CCC).
 *
 * This function is called whenever a connected client modifies the CCC for a characteristic.
 * It sets the `notify_enabled` flag based on whether notifications are enabled.
 *
 * @param attr The GATT attribute whose CCC was modified.
 * @param value The new CCC value, which determines if notifications are enabled.
 */
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

/* LED Button Service Declaration */
/**
 * @brief Battery Service GATT Declaration.
 *
 * This service includes two characteristics:
 * - Voltage: Allows reading voltage values and enabling notifications.
 * - Temperature: Allows reading temperature values and enabling notifications.
 */
BT_GATT_SERVICE_DEFINE(battery_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_BATTERY),
    BT_GATT_CHARACTERISTIC(BT_UUID_VOLTAGE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL,
                           NULL),
    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD,              // Characteristic User Descriptor (CUD)
                       BT_GATT_PERM_READ,
                       NULL, NULL, "Voltage reading"),
    BT_GATT_CCC(ccc_cfg_changed,                     // CCCD for notifications
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(BT_UUID_TEMP,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL,
                           NULL),
    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD,              // Characteristic User Descriptor (CUD)
                       BT_GATT_PERM_READ,
                       NULL, NULL, "Temp reading"),
    BT_GATT_CCC(ccc_cfg_changed,                     // CCCD for notifications
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/**
 * @brief Send a voltage reading to a connected client via notification.
 *
 * This function sends the voltage value as a notification to the connected client
 * if notifications are enabled for the voltage characteristic.
 *
 * @param voltage The voltage value to send.
 * @return 0 on success, or a negative error code if notifications are not enabled or fail.
 */
int bt_send_voltage(uint32_t voltage)
{
    if (!notify_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &battery_svc.attrs[2],
                          &voltage,
                          sizeof(voltage));
}

/**
 * @brief Send a temperature reading to a connected client via notification.
 *
 * This function sends the temperature value as a notification to the connected client
 * if notifications are enabled for the temperature characteristic.
 *
 * @param temp The temperature value to send.
 * @return 0 on success, or a negative error code if notifications are not enabled or fail.
 */
int bt_send_temp(uint32_t temp)
{
    if (!notify_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &battery_svc.attrs[6],
                          &temp,
                          sizeof(temp));
}