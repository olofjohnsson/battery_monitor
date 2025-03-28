#include "bluetooth.h"
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <bluetooth/services/nus.h>


#include "service.h"

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BATTERY_VAL),
};

/**
 * @brief Callback invoked when a Bluetooth connection is established.
 *
 * @param conn Pointer to the connection object.
 * @param err Error code (0 if successful, non-zero if an error occurred).
 */
void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Failed to connect (err %u)\n", err);
    } else {
        printk("Connected\n");
    }
}

/**
 * @brief Callback invoked when a Bluetooth connection is disconnected.
 *
 * @param conn Pointer to the connection object.
 * @param reason Reason for the disconnection.
 */
void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason %u)\n", reason);
}

#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
/**
 * @brief Callback invoked when the Bluetooth security level changes.
 *
 * @param conn Pointer to the connection object.
 * @param level New security level.
 * @param err Error code (0 if successful, non-zero if an error occurred).
 */
static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        printk("Security changed: %s level %u\n", addr, level);
    } else {
        printk("Security failed: %s level %u err %d\n", addr, level, err);
    }
}
#endif

struct bt_conn_cb conn_callbacks = {
    .connected        = connected,
    .disconnected     = disconnected,
#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_LBS_SECURITY_ENABLED)
/**
 * @brief Callback invoked to display the Bluetooth passkey for pairing.
 *
 * @param conn Pointer to the connection object.
 * @param passkey The passkey to display.
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Passkey for %s: %06u\n", addr, passkey);
}

/**
 * @brief Callback invoked when pairing is cancelled.
 *
 * @param conn Pointer to the connection object.
 */
static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

/**
 * @brief Callback invoked when pairing is completed.
 *
 * @param conn Pointer to the connection object.
 * @param bonded Indicates whether the devices are bonded.
 */
static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

/**
 * @brief Callback invoked when pairing fails.
 *
 * @param conn Pointer to the connection object.
 * @param reason Reason for the failure.
 */
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

/**
 * @brief Initialize the Bluetooth subsystem.
 *
 * This function sets up the Bluetooth stack, registers connection callbacks, and optionally
 * loads persistent settings if configured.
 */
void bluetooth_init(void)
{
    int err;

    if (IS_ENABLED(CONFIG_BT_LBS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    err = bt_nus_init(NULL);
    if (err) {
        printk("Failed to initialize NUS (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    // Register connection callbacks
    bt_conn_cb_register(&conn_callbacks);

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }
}

/**
 * @brief Start Bluetooth advertising.
 *
 * This function starts advertising to allow other devices to discover and connect.
 * It uses pre-defined advertising and scan response data.
 */
void bluetooth_start_advertising(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
                          sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}
