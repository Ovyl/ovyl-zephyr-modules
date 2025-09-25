/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file config_mgr.c
 * @brief Configuration manager implementation
 */

#include <ovyl/config_mgr.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/zbus/zbus.h>

#include <ovyl/config_options.h>
#include <ovyl/configs.h>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

LOG_MODULE_REGISTER(ovyl_cfg_mgr, CONFIG_OVYL_CFG_MGR_LOG_LEVEL);

/*****************************************************************************
 * Variables
 *****************************************************************************/

/**
 * @brief Private instance
 */
static struct {
    struct nvs_fs fs;
} prv_inst;

/*****************************************************************************
 * Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Public Function Definitions
 *****************************************************************************/

void config_mgr_init(void) {
    const struct flash_area *fa;
    int rc = flash_area_open(FLASH_AREA_ID(CFG_OPT_FLASH_AREA), &fa);
    if (rc < 0) {
        LOG_ERR("Failed to open NVS flash area: %s", STRINGIFY(CFG_OPT_FLASH_AREA));
        return;
    }

    struct flash_pages_info info;
    rc = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off, &info);

    prv_inst.fs.offset = fa->fa_off;
    prv_inst.fs.flash_device = fa->fa_dev;
    prv_inst.fs.sector_size = info.size;
    prv_inst.fs.sector_count = fa->fa_size / info.size;

    rc = nvs_mount(&prv_inst.fs);
    if (rc != 0) {
        LOG_ERR("NVS failed to mount: %d", rc);
    }
}

bool config_mgr_get_value(config_key_t key, void *dst, size_t size) {
    if (dst == NULL) {
        return false;
    }

    config_entry_t *entry = configs_get_entry(key);

    if (entry == NULL) {
        return false;
    }

    __ASSERT(size == entry->value_size_bytes,
             "Size of dst buffer for %s incorrect.  Expected %u but got %u.",
             entry->human_readable_key,
             entry->value_size_bytes,
             size);

    ssize_t ret = nvs_read(&prv_inst.fs, key, dst, size);

    // Configuration not in flash, so use default
    if (ret == -ENOENT) {
        memcpy(dst, entry->default_value, entry->value_size_bytes);

        return true;
    }

    if (ret < 0) {
        LOG_ERR("Failed to read config for key %s: %d", entry->human_readable_key, ret);
        return false;
    }

    return true;
}

bool config_mgr_set_value(config_key_t key, const void *src, size_t size) {
    if (src == NULL) {
        return false;
    }

    config_entry_t *entry = configs_get_entry(key);

    if (entry == NULL) {
        return false;
    }

    __ASSERT(size == entry->value_size_bytes,
             "Size of src buffer for %s incorrect.  Expected %u but got %u.",
             entry->human_readable_key,
             entry->value_size_bytes,
             size);

    /* Validate ambient light sample rate multiplier cannot be 0 */
    if (key == CFG_AMBIENT_LIGHT_SAMPLE_RATE_MULTIPLIER) {
        uint32_t value = *(const uint32_t *)src;
        if (value == 0) {
            LOG_ERR("Ambient light sample rate multiplier cannot be 0");
            return false;
        }
    }

    ssize_t ret = nvs_write(&prv_inst.fs, key, src, size);

    if (ret < 0) {
        LOG_ERR("Failed to write config value for key %s: %d", entry->human_readable_key, ret);
        return false;
    }

    /* Publish config change event */
    prv_publish_config_change(key, src);

    return true;
}

/*****************************************************************************
 * Private Function Definitions
 *****************************************************************************/

static void prv_reset_nvs(void) {
    for (size_t i = 0; i < CFG_NUM_KEYS; i++) {
        int ret = nvs_delete(&prv_inst.fs, i);

        if (ret != 0) {
            LOG_ERR("Failed to reset %s to default: %d", config_key_as_str(i), ret);
        }
    }
}

static void prv_reset_config_values(void) {
}

/*****************************************************************************
 * Shell Commands
 *****************************************************************************/
#ifdef CONFIG_SHELL

#include <zephyr/shell/shell.h>
#include <stdlib.h>

/**
 * @brief Shell command to list all configuration values
 */
static int cmd_config_list(const struct shell *sh, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Configuration Values:");
    shell_print(sh, "====================");

    for (size_t i = 0; i < CFG_NUM_KEYS; i++) {
        config_entry_t *entry = configs_get_entry(i);
        if (entry == NULL) {
            continue;
        }

        const char *key_name = config_key_as_str(i);

        if (entry->value_size_bytes == sizeof(uint8_t)) {
            uint8_t value;
            if (config_mgr_get_value(i, &value, sizeof(value))) {
                shell_print(sh, "  %s: %u", key_name, value);
            } else {
                shell_print(sh, "  %s: <error reading>", key_name);
            }
        } else if (entry->value_size_bytes == sizeof(uint16_t)) {
            uint16_t value;
            if (config_mgr_get_value(i, &value, sizeof(value))) {
                shell_print(sh, "  %s: %u", key_name, value);
            } else {
                shell_print(sh, "  %s: <error reading>", key_name);
            }
        } else if (entry->value_size_bytes == sizeof(uint32_t)) {
            uint32_t value;
            if (config_mgr_get_value(i, &value, sizeof(value))) {
                shell_print(sh, "  %s: %u", key_name, value);
            } else {
                shell_print(sh, "  %s: <error reading>", key_name);
            }

        } else {
            /* For complex structures, just show the size */
            shell_print(sh, "  %s: <complex type, %u bytes>", key_name, entry->value_size_bytes);
        }
    }

    return 0;
}

/**
 * @brief Shell command to reset all NVS entries
 */
static int cmd_config_reset_nvs(const struct shell *sh, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Resetting all NVS entries...");

    prv_reset_nvs();

    shell_print(sh, "NVS reset completed");
    return 0;
}

/**
 * @brief Shell command to reset all NVS entries
 */
static int cmd_config_reset_configs(const struct shell *sh, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Resetting config NVS entries...");

    prv_reset_config_values();

    shell_print(sh, "NVS config reset completed");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    config_cmds,
    SHELL_CMD(list, NULL, "List all configuration values", cmd_config_list),
    SHELL_CMD(reset_nvs, NULL, "Reset all NVS entries", cmd_config_reset_nvs),
    SHELL_CMD(reset_config, NULL, "Reset NVS config entries", cmd_config_reset_configs),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(config,
                   &config_cmds,
                   "Configuration management - type 'config help' for usage",
                   cmd_config_help);

#endif /* CONFIG_SHELL */
