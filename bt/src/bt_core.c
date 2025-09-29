/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt_core.c
 * @brief Ovyl BT module core implementation for BLE peripheral functionality
 */

#include <ovyl/bt_core.h>

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/logging/log.h>

#include <ovyl/bt_version.h>

#ifdef CONFIG_OVYL_BT_ZBUS_PUBLISH
#include <zephyr/zbus/zbus.h>
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

LOG_MODULE_REGISTER(ovyl_bt_core, CONFIG_OVYL_BT_LOG_LEVEL);

#ifdef CONFIG_OVYL_BT_ZBUS_PUBLISH
/* Define the Zbus channel for BT connection events */
ZBUS_CHAN_DEFINE(ovyl_bt_conn_chan,
                 struct ovyl_bt_conn_event,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(0));
#endif

/*****************************************************************************
 * Variables
 *****************************************************************************/

/**
 * @brief Array of data to advertise
 */
static const struct bt_data prv_advertising_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, CONFIG_OVYL_BT_ADV_FLAGS),
#ifdef CONFIG_OVYL_BT_ADV_INCLUDE_NAME
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, strlen(CONFIG_BT_DEVICE_NAME)),
#endif
};

/**
 * @brief Advertising parameters
 *
 */
static const struct bt_le_adv_param adv_params = {
#ifdef CONFIG_OVYL_BT_ADV_CONNECTABLE
    .options = BT_LE_ADV_OPT_CONN,
#else
    .options = 0,
#endif
    .interval_min = CONFIG_OVYL_BT_ADV_INTERVAL_MIN,
    .interval_max = CONFIG_OVYL_BT_ADV_INTERVAL_MAX,
    .id = CONFIG_OVYL_BT_ADV_ID,
};

/**
 * @brief Private static instance
 */
static struct {
    struct k_work advertising_worker;
    struct ble_core_callbacks callbacks;
    struct bt_conn *conn;
    uint16_t conn_handle;
    volatile bool is_advertising;
} prv_inst;

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
/**
 * @brief Start BLE advertising
 */
static void prv_advertising_start(void);

static void prv_advertising_worker_task(struct k_work *work);

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
int ble_core_init(void) {
    int err;

    prv_inst.conn = NULL;
    prv_inst.conn_handle = 0;

    k_work_init(&prv_inst.advertising_worker, prv_advertising_worker_task);

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth core initialization failed: %d", err);
        return err;
    }

#ifdef CONFIG_OVYL_BT_ADV_AUTO_START
    prv_advertising_start();
#endif

    LOG_INF("Ovyl BT module v%s initialized", OVYL_BT_VERSION_STRING);
    return 0;
}

void ble_core_start_advertising(void) {
    prv_advertising_start();
}

bool ble_core_is_currently_advertising(void) {
    return prv_inst.is_advertising;
}

void ble_core_set_callbacks(const struct ble_core_callbacks *callbacks) {
    if (callbacks) {
        prv_inst.callbacks = *callbacks;
    } else {
        memset(&prv_inst.callbacks, 0, sizeof(prv_inst.callbacks));
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * @brief Callback called when device connected
 *
 * @param conn Pointer to connection
 * @param err Error connecting to device
 */
static void prv_device_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        LOG_ERR("Failed to connect to BLE device: %u", err);
        return;
    } else {
        LOG_INF("Connected to BLE device.");

        prv_inst.conn = bt_conn_ref(conn);
        int ret = bt_hci_get_conn_handle(prv_inst.conn, &prv_inst.conn_handle);
        if (ret) {
            LOG_ERR("Failed to get connection handle: %d", ret);
            return;
        }
    }

    if (prv_inst.callbacks.on_connected) {
        prv_inst.callbacks.on_connected(conn, err);
    }

#ifdef CONFIG_OVYL_BT_ZBUS_PUBLISH
    /* Publish connection event via Zbus */
    struct ovyl_bt_conn_event evt = {
        .state = OVYL_BT_CONN_STATE_CONNECTED, .reason = err, .conn_handle = prv_inst.conn_handle};
    int ret_zbus = zbus_chan_pub(&ovyl_bt_conn_chan, &evt, K_NO_WAIT);
    if (ret_zbus != 0) {
        LOG_WRN("Failed to publish BT connection event: %d", ret_zbus);
    }
#endif

    prv_inst.is_advertising = false;
}

/**
 * @brief Callback called when device disconnected
 *
 * @param conn Pointer to connection
 * @param reason Reason for disconnection
 */
static void prv_device_disconnected(struct bt_conn *conn, uint8_t reason) {
    LOG_INF("Disconnected from device: %u", reason);

    // Explicitly unreference the connection only if we have a reference
    if (prv_inst.conn) {
        bt_conn_unref(prv_inst.conn);
        prv_inst.conn = NULL;
    }

    prv_inst.conn_handle = 0;

    if (prv_inst.callbacks.on_disconnected) {
        prv_inst.callbacks.on_disconnected(conn, reason);
    }

#ifdef CONFIG_OVYL_BT_ZBUS_PUBLISH
    /* Publish disconnection event via Zbus */
    struct ovyl_bt_conn_event evt = {
        .state = OVYL_BT_CONN_STATE_DISCONNECTED, .reason = reason, .conn_handle = 0};
    int ret = zbus_chan_pub(&ovyl_bt_conn_chan, &evt, K_NO_WAIT);
    if (ret != 0) {
        LOG_WRN("Failed to publish BT disconnection event: %d", ret);
    }
#endif

#ifdef CONFIG_OVYL_BT_ADV_RESTART_ON_DISCONNECT
    prv_advertising_start();
#endif
}

/**
 * @brief Worker task handling advertising
 *
 * @param work Pointer to worker instance
 */
static void prv_advertising_worker_task(struct k_work *work) {
    int err = bt_le_adv_start(
        &adv_params, prv_advertising_data, ARRAY_SIZE(prv_advertising_data), NULL, 0);

    if (err) {
        LOG_ERR("Failed to start BLE advertising: %d", err);
        return;
    }

    LOG_INF("BLE Advertising begun...");
    prv_inst.is_advertising = true;
}

/**
 * @brief Stop BLE advertising
 */
static void prv_advertising_stop(void) {
    int err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop BLE advertising: %d", err);
    } else {
        LOG_INF("BLE Advertising stopped");
        prv_inst.is_advertising = false;
    }
}

/**
 * @brief Start advertising
 */
static void prv_advertising_start(void) {
    k_work_submit(&prv_inst.advertising_worker);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = prv_device_connected,
    .disconnected = prv_device_disconnected,
};

/*****************************************************************************
 * Shell Commands
 *****************************************************************************/
#ifdef CONFIG_OVYL_BT_SHELL

#include <zephyr/shell/shell.h>
#include <stdlib.h>

/**
 * @brief Shell command to start advertising
 */
static int cmd_adv_start(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (prv_inst.is_advertising) {
        shell_print(shell, "Advertising already active");
        return 0;
    }

    prv_advertising_start();
    shell_print(shell, "Advertising start requested");
    return 0;
}

/**
 * @brief Shell command to stop advertising
 */
static int cmd_adv_stop(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (!prv_inst.is_advertising) {
        shell_print(shell, "Advertising not active");
        return 0;
    }

    prv_advertising_stop();
    shell_print(shell, "Advertising stopped");
    return 0;
}

/**
 * @brief Shell command to disconnect active connection
 */
static int cmd_disconnect(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (!prv_inst.conn) {
        shell_print(shell, "No active connection");
        return -ENOTCONN;
    }

    int err = bt_conn_disconnect(prv_inst.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        shell_print(shell, "Failed to disconnect: %d", err);
        return err;
    }

    shell_print(shell, "Disconnection initiated");
    return 0;
}

/**
 * @brief Shell command to show BT status
 */
static int cmd_status(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "BT Module Status:");
    shell_print(shell, "  Advertising: %s", prv_inst.is_advertising ? "Yes" : "No");
    shell_print(shell, "  Connected: %s", prv_inst.conn ? "Yes" : "No");
    if (prv_inst.conn) {
        shell_print(shell, "  Connection handle: 0x%04x", prv_inst.conn_handle);
    }
    return 0;
}

/* Advertising subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(ovyl_bt_adv_cmds,
                               SHELL_CMD(start, NULL, "Start BLE advertising", cmd_adv_start),
                               SHELL_CMD(stop, NULL, "Stop BLE advertising", cmd_adv_stop),
                               SHELL_SUBCMD_SET_END);

/* Main BT subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(
    ovyl_bt_cmds,
    SHELL_CMD(adv, &ovyl_bt_adv_cmds, "Advertising commands", NULL),
    SHELL_CMD(disconnect, NULL, "Disconnect active BLE connection", cmd_disconnect),
    SHELL_CMD(status, NULL, "Show BT module status", cmd_status),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ovyl_bt, &ovyl_bt_cmds, "Ovyl Bluetooth module commands", NULL);

#endif /* CONFIG_OVYL_BT_SHELL */
