# OVYL IWDOG Module Integration Guide

## Overview

The OVYL Internal Watchdog (IWDOG) module provides a high-level abstraction for hardware watchdog timer management in Zephyr RTOS applications. This module includes configurable automatic feeding, warning notifications, and integration with Zephyr's logging and Zbus subsystems.

## Integration Steps

### 1. Add Module to West Manifest

Add the OVYL modules repository the `west.yml`:

```yaml
manifest:
  remotes:
    - name: ovyl
      url-base: https://github.com/Ovyl/ovyl-zephyr-modules

  projects:
    - name: ovyl-zephyr-modules
      remote: ovyl
      repo-path: ovyl-zephyr-modules
      revision: main
      path: modules/ovyl
```

After updating `west.yml`, run:
```bash
west update ovyl-modules
```

### 2. Device Tree Configuration

The IWDOG module requires a hardware watchdog device. Ensure the device tree has a watchdog node with the `watchdog0` alias:

#### Example Device Tree Overlay

Create or update the board overlay file (e.g., `boards/your_board.overlay`):

```dts
/ {
    aliases {
        watchdog0 = &wdt0;
    };
};

&wdt0 {
    status = "okay";
};
```

### 3. Kconfig Configuration

Enable the module in your application's `prj.conf` and change any default kconfig values if desired:

```conf
# Enable OVYL IWDOG module
CONFIG_OVYL_IWDOG=y

```
