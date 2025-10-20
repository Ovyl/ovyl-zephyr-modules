# Copyright (c) 2025 Ovyl
# SPDX-License-Identifier: Apache-2.0

# Ovyl Config Module Integration Guide

## Overview

The Ovyl Config module provides a persistent configuration storage system using NVS (Non-Volatile Storage) with support for resettable and non-resettable configuration entries.

## Configuration Definition

Create a configuration definition file (e.g., `app/app_configs.def`) with your application's configuration entries using the `CFG_DEFINE` macro:

```c
// CFG_DEFINE(key_name, type, default_value, resettable)
// - key_name: Unique identifier for the configuration entry
// - type: Data type (uint8_t, uint16_t, uint32_t, or custom struct)
// - default_value: Initial value when not set in NVS
// - resettable: true if entry can be reset via shell command, false to protect it

CFG_DEFINE(DEVICE_ID, uint32_t, 0x1234, false)         // Non-resettable device ID
CFG_DEFINE(SAMPLE_RATE, uint16_t, 1000, true)          // Resettable sample rate
CFG_DEFINE(DEBUG_MODE, uint8_t, 0, true)               // Resettable debug flag
```

## Integration Steps

### 1. Add Module to West Manifest

Add the Ovyl modules repository the `west.yml`:

```yaml
manifest:
  remotes:
    - name: ovyl
      url-base: https://github.com/Ovyl

  projects:
    - name: ovyl-zephyr-modules
      remote: ovyl
      repo-path: ovyl-zephyr-modules
      revision: main            # or a branch/tag/SHA
      path: modules/ovyl        # folder in your workspace

```

After updating `west.yml`, run:
```bash
west update ovyl-zephyr-modules
```

### 3. Flash Partition Configuration

The config module requires an NVS (Non-Volatile Storage) partition to persist configuration data. You need to create or update a `pm_static.yml` file in your project root:

```yaml
nvs_storage:
  address: 0x3e000    # Adjust based on your flash layout
  size: 0x2000        # 8KB for config storage
```

**Important**: The partition name in `pm_static.yml` must match `nvs_storage`

### 4. Kconfig Configuration

Enable the module in your application's `prj.conf` and set the path to the config define file:

```conf
# Enable Ovyl Config module
CONFIG_OVYL_CONFIG=y

# Set path to your application's config definitions
CONFIG_OVYL_CONFIG_APP_DEF_PATH="config.def"

# (Optional) Path to custom type definitions header
CONFIG_OVYL_CONFIG_TYPES_DEF_PATH="app/app_config_types.h"
```

This path needs to be visible to the build system. Make sure the path is included in the app CMakeLists.txt with

`zephyr_include_directories`

When providing custom configuration types, place them in the header identified by `CONFIG_OVYL_CONFIG_TYPES_DEF_PATH`. Add any required `#include` directives for those types inside that header.

## Usage

### Initialization

Initialize the configuration manager during system startup:

```c
#include <ovyl/config_mgr.h>

void main(void) {
    // Initialize the config manager
    ovyl_config_mgr_init();

    // Your application code...
}
```

### Reading Configuration Values

```c
#include <ovyl/config_mgr.h>

uint16_t sample_rate;
if (ovyl_config_mgr_get_value(SAMPLE_RATE, &sample_rate, sizeof(sample_rate))) {
    // Use the configuration value
    LOG_INF("Sample rate: %u", sample_rate);
}
```

### Writing Configuration Values

```c
#include <ovyl/config_mgr.h>

uint16_t new_rate = 2000;
if (ovyl_config_mgr_set_value(SAMPLE_RATE, &new_rate, sizeof(new_rate))) {
    LOG_INF("Sample rate updated");
}
```

### Resetting Configuration Values

```c
#include <ovyl/config_mgr.h>

// Reset all configuration values to defaults
ovyl_config_mgr_reset_nvs();

// Reset only resettable configuration values
// (entries with resettable=true in their definition)
ovyl_config_mgr_reset_configs();
```

### Shell Commands

The module provides shell commands for configuration management:

- `ovyl_config list` - List all configuration values as a hex dump from the device memory.
- `ovyl_config reset_nvs` - Reset all NVS entries to defaults
- `ovyl_config reset_config` - Reset only resettable entries to defaults

**Note:** Shell commands to get/set individual configuration values are not included in the module. If you need this functionality, you'll need to implement application-specific shell commands using the `ovyl_config_mgr_get_value()` and `ovyl_config_mgr_set_value()` functions.

## Known Limitations

1. **Hardcoded Partition Name**: The NVS partition name `nvs_storage` is hardcoded in the module due to Zephyr's flash map macro requirements. This cannot be made configurable through Kconfig.

2. **Shell Commands Not Auto-Generated**: Shell commands for getting/setting individual configuration values are not automatically generated. Each application must implement its own shell commands if this functionality is needed.

