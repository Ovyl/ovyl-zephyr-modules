/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt_core.h
 * @brief Ovyl BT module public API for BLE peripheral functionality
 */

#ifndef OVYL_BT_CORE_H
#define OVYL_BT_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/bluetooth/conn.h>

#ifdef CONFIG_OVYL_BT_ZBUS_PUBLISH
#include <zephyr/zbus/zbus.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Structs, Unions, Enums, & Typedefs
 *****************************************************************************/

/**
 * @brief Bluetooth connection state
 */
enum ovyl_bt_conn_state {
    OVYL_BT_CONN_STATE_DISCONNECTED = 0,
    OVYL_BT_CONN_STATE_CONNECTED = 1,
};

/**
 * @brief Bluetooth connection event structure
 *
 * Published when BT connection state changes
 */
struct ovyl_bt_conn_event {
    enum ovyl_bt_conn_state state; /* Current connection state */
    uint8_t reason;                /* Connect error or disconnect reason code */
    uint16_t conn_handle;          /* Connection handle (0 if disconnected) */
};

#ifdef CONFIG_OVYL_BT_ZBUS_PUBLISH
/* Zbus channel for BT connection events */
ZBUS_CHAN_DECLARE(ovyl_bt_conn_chan);
#endif

/*****************************************************************************
 * Public Types
 *****************************************************************************/

/**
 * @brief BT connection callbacks structure
 */
struct ble_core_callbacks {
    void (*on_connected)(struct bt_conn *conn, uint8_t err);
    void (*on_disconnected)(struct bt_conn *conn, uint8_t reason);
};

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
/**
 * @brief Initialize bluetooth core
 *
 * @return 0 on success, negative errno on failure
 */
int ble_core_init(void);

/**
 * @brief Start advertising if not already
 *
 * @return none
 */
void ble_core_start_advertising(void);

/**
 * @brief Return true if system is currently advertising, false otherwise
 *
 * @return true if advertising, false otherwise
 */
bool ble_core_is_currently_advertising(void);

/**
 * @brief Register callbacks for connection events
 *
 * Must be called before ble_core_init() to ensure callbacks are registered
 * before any connections can occur.
 *
 * @param callbacks Pointer to callbacks structure (can be NULL to clear)
 */
void ble_core_set_callbacks(const struct ble_core_callbacks *callbacks);

#ifdef __cplusplus
}
#endif
#endif /* OVYL_BT_CORE_H */
