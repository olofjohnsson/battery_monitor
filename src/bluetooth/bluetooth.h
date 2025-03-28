#ifndef BLUETOOTH_H
#define BLUETOOTH_H

/**
 * @brief Initialize the Bluetooth subsystem.
 *
 * This function sets up the Bluetooth stack, registers connection callbacks, 
 * and optionally loads persistent settings if configured.
 */
void bluetooth_init(void);

/**
 * @brief Start Bluetooth advertising.
 *
 * This function starts advertising to allow other devices to discover and connect.
 * It uses pre-defined advertising and scan response data.
 */
void bluetooth_start_advertising(void);

#endif // BLUETOOTH_H
