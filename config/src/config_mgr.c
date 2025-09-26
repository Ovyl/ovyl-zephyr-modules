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
#include <zephyr/sys/util.h>
#include <string.h>

#include <zephyr/devicetree.h>

#include <ovyl/configs.h>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

LOG_MODULE_REGISTER(ovyl_cfg_mgr, CONFIG_OVYL_CFG_MGR_LOG_LEVEL);

#define CFG_OPT_FLASH_AREA nvs_storage

/*****************************************************************************
 * Variables
 *****************************************************************************/

static struct {
    struct nvs_fs fs; // NVS filesystem instance for config storage
} prv_inst;

/*****************************************************************************
 * Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

void ovyl_config_mgr_init(void) {
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

bool ovyl_config_mgr_get_value(config_key_t key, void *dst, size_t size) {
    if (dst == NULL) {
        return false;
    }

    config_entry_t *entry = ovyl_configs_get_entry(key);

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

bool ovyl_config_mgr_set_value(config_key_t key, const void *src, size_t size) {
    if (src == NULL) {
        return false;
    }

    config_entry_t *entry = ovyl_configs_get_entry(key);

    if (entry == NULL) {
        return false;
    }

    __ASSERT(size == entry->value_size_bytes,
             "Size of src buffer for %s incorrect.  Expected %u but got %u.",
             entry->human_readable_key,
             entry->value_size_bytes,
             size);

    ssize_t ret = nvs_write(&prv_inst.fs, key, src, size);

    if (ret < 0) {
        LOG_ERR("Failed to write config value for key %s: %d", entry->human_readable_key, ret);
        return false;
    }

    return true;
}

void ovyl_config_mgr_reset_nvs(void) {
    for (size_t i = 0; i < CFG_NUM_KEYS; i++) {
        int ret = nvs_delete(&prv_inst.fs, i);

        if (ret != 0) {
            LOG_ERR("Failed to reset %s to default: %d", ovyl_config_key_as_str(i), ret);
        }
    }
}

void ovyl_config_mgr_reset_configs(void) {
    for (size_t i = 0; i < CFG_NUM_KEYS; i++) {
        config_entry_t *entry = ovyl_configs_get_entry(i);
        if (entry == NULL) {
            continue;
        }

        // Only reset entries that are marked as resettable
        if (entry->resettable) {
            int ret = nvs_delete(&prv_inst.fs, i);

            if (ret != 0) {
                LOG_ERR("Failed to reset %s to default: %d", ovyl_config_key_as_str(i), ret);
            } else {
                LOG_DBG("Reset %s to default", ovyl_config_key_as_str(i));
            }
        }
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

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
        config_entry_t *entry = ovyl_configs_get_entry(i);
        if (entry == NULL) {
            continue;
        }

        const char *key_name = ovyl_config_key_as_str(i);

        if (entry->value_size_bytes == sizeof(uint8_t)) {
            uint8_t value;
            if (ovyl_config_mgr_get_value(i, &value, sizeof(value))) {
                shell_print(sh, "  %s: %u", key_name, value);
            } else {
                shell_print(sh, "  %s: <error reading>", key_name);
            }
        } else if (entry->value_size_bytes == sizeof(uint16_t)) {
            uint16_t value;
            if (ovyl_config_mgr_get_value(i, &value, sizeof(value))) {
                shell_print(sh, "  %s: %u", key_name, value);
            } else {
                shell_print(sh, "  %s: <error reading>", key_name);
            }
        } else if (entry->value_size_bytes == sizeof(uint32_t)) {
            uint32_t value;
            if (ovyl_config_mgr_get_value(i, &value, sizeof(value))) {
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

    ovyl_config_mgr_reset_nvs();

    shell_print(sh, "NVS reset completed");
    return 0;
}

/**
 * @brief Shell command to reset resettable configuration entries
 */
static int cmd_config_reset_configs(const struct shell *sh, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Resetting resettable config entries...");

    ovyl_config_mgr_reset_configs();

    shell_print(sh, "Resettable config entries reset completed");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(config_cmds,
                               SHELL_CMD_ARG(list,
                                             NULL,
                                             "List all configuration values.\n"
                                             "usage:\n"
                                             "$ ovyl_config list\n",
                                             cmd_config_list,
                                             1,
                                             0),
                               SHELL_CMD_ARG(reset_nvs,
                                             NULL,
                                             "Reset all NVS entries to defaults.\n"
                                             "This will delete ALL stored configuration values.\n"
                                             "usage:\n"
                                             "$ ovyl_config reset_nvs\n",
                                             cmd_config_reset_nvs,
                                             1,
                                             0),
                               SHELL_CMD_ARG(reset_config,
                                             NULL,
                                             "Reset resettable configuration entries to defaults.\n"
                                             "Only resets entries marked as resettable.\n"
                                             "usage:\n"
                                             "$ ovyl_config reset_config\n",
                                             cmd_config_reset_configs,
                                             1,
                                             0),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ovyl_config, &config_cmds, "Configuration management commands", NULL);

#endif /* CONFIG_SHELL */
