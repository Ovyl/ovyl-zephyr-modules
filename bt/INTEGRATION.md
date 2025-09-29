# Copyright (c) 2025 Ovyl
# SPDX-License-Identifier: Apache-2.0

# Ovyl BT Module Integration Guide

## Overview

The Ovyl BT module provides a Bluetooth Low Energy (BLE) peripheral implementation with configurable advertising, connection management, and optional Zbus event publishing for connection state changes.

## Features

- Configurable BLE advertising parameters
- Connection state management
- Zbus integration for publishing connection events
- Shell commands for runtime control
- Automatic advertising restart on disconnect (configurable)
- Support for multiple Bluetooth identities

## Integration Steps

### 1. Add Module to West Manifest

Add the Ovyl modules repository to your `west.yml`:

```yaml
manifest:
  remotes:
    - name: ovyl
      url-base: https://github.com/Ovyl

  projects:
    - name: ovyl-zephyr-modules
      remote: ovyl
      repo-path: ovyl-zephyr-modules
      revision: main            # or a specific branch/tag/SHA
      path: modules/ovyl        # folder in your workspace
```

After updating `west.yml`, run:
```bash
west update ovyl-zephyr-modules
```

### 2. Kconfig Configuration

Enable the module and configure options in your application's `prj.conf`:

```conf
# Enable Bluetooth stack (required)
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="My Device"

# Enable Ovyl BT module
CONFIG_OVYL_BT=y

# Advertising configuration (optional - defaults shown)
CONFIG_OVYL_BT_ADV_FLAGS=0x06                    # BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR
CONFIG_OVYL_BT_ADV_CONNECTABLE=y                 # Enable connectable advertising
CONFIG_OVYL_BT_ADV_INTERVAL_MIN=1600             # 1 second (in 0.625ms units)
CONFIG_OVYL_BT_ADV_INTERVAL_MAX=2400             # 1.5 seconds (in 0.625ms units)
CONFIG_OVYL_BT_ADV_AUTO_START=y                  # Auto-start advertising on init
CONFIG_OVYL_BT_ADV_RESTART_ON_DISCONNECT=y       # Auto-restart after disconnect
CONFIG_OVYL_BT_ADV_INCLUDE_NAME=y                # Include device name in advertising
CONFIG_OVYL_BT_ADV_ID=0                          # Bluetooth identity ID (0-15)

# Enable Zbus event publishing (optional)
CONFIG_ZBUS=y
CONFIG_OVYL_BT_ZBUS_PUBLISH=y

# Enable shell commands (optional)
CONFIG_SHELL=y
```

## Usage

### Initialization

Initialize the BT module during system startup:

```c
#include <ovyl/bt_core.h>

int main(void) {
    // Initialize the BT module
    int err = ble_core_init();
    if (err) {
        LOG_ERR("Failed to initialize BT: %d", err);
        return err;
    }

    // Your application code...
    return 0;
}
```

### Using Callbacks (Alternative to Zbus)

Register callbacks to handle connection events directly:

```c
#include <ovyl/bt_core.h>

static void on_bt_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        LOG_ERR("Connection failed: %u", err);
    } else {
        LOG_INF("Device connected!");
    }
}

static void on_bt_disconnected(struct bt_conn *conn, uint8_t reason) {
    LOG_INF("Device disconnected (reason: %u)", reason);
}

int main(void) {
    // Register callbacks before initialization
    struct ble_core_callbacks callbacks = {
        .on_connected = on_bt_connected,
        .on_disconnected = on_bt_disconnected
    };
    ble_core_set_callbacks(&callbacks);

    // Initialize the BT module
    int err = ble_core_init();
    if (err) {
        LOG_ERR("Failed to initialize BT: %d", err);
        return err;
    }

    // Your application code...
    return 0;
}
```

### Manual Advertising Control

If `CONFIG_OVYL_BT_ADV_AUTO_START` is disabled, manually control advertising:

```c
#include <ovyl/bt_core.h>

// Start advertising
ble_core_start_advertising();

// Check if currently advertising
if (ble_core_is_currently_advertising()) {
    LOG_INF("Device is advertising");
}
```

### Zbus Event Subscription

Subscribe to BT connection events when `CONFIG_OVYL_BT_ZBUS_PUBLISH` is enabled:

```c
#include <zephyr/zbus/zbus.h>
#include <ovyl/bt_core.h>

// Define listener callback
static void bt_conn_listener(const struct zbus_channel *chan) {
    const struct ovyl_bt_conn_event *evt = zbus_chan_const_msg(chan);

    if (evt->state == OVYL_BT_CONN_STATE_CONNECTED) {
        LOG_INF("Device connected (handle: 0x%04x)", evt->conn_handle);
    } else if (evt->state == OVYL_BT_CONN_STATE_DISCONNECTED) {
        LOG_INF("Device disconnected (reason: %u)", evt->reason);
    }
}

// Define and register listener
ZBUS_LISTENER_DEFINE(bt_conn_listener_node, bt_conn_listener);
ZBUS_CHAN_ADD_OBS(ovyl_bt_conn_chan, bt_conn_listener_node, 3);
```

### Shell Commands

When `CONFIG_SHELL` is enabled, the following commands are available:

- `ovyl_bt status` - Show BT module status (advertising/connection state)
- `ovyl_bt adv start` - Start BLE advertising
- `ovyl_bt adv stop` - Stop BLE advertising
- `ovyl_bt disconnect` - Disconnect active BLE connection

Example usage:
```bash
uart:~$ ovyl_bt status
BT Module Status:
  Advertising: Yes
  Connected: No

uart:~$ ovyl_bt adv stop
Advertising stopped

uart:~$ ovyl_bt adv start
Advertising start requested
```

## Limitations

1. **Single Connection**: Currently supports only one active BLE connection at a time
2. **Peripheral Only**: Module is designed for BLE peripheral role only
3. **Fixed Advertising Data**: Advertising data structure is fixed (flags + optional name)
4. **No GATT Services**: Module provides connection management only; GATT services must be implemented separately
