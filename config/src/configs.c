/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file configs.c
 * @brief Configuration entries implementation
 */

#include <ovyl/configs.h>

#include <zephyr/logging/log.h>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Variables
 *****************************************************************************/

/**
 * @Brief Define default values for each of the keys
 */
#define CFG_DEFINE(key, type, default_val) static type key##_def_val = default_val;
#include "configs.def"
#undef CFG_DEFINE

#define CFG_DEFINE(key, type, default_val)                                                         \
    [key] = {.value_size_bytes = sizeof(type),                                                     \
             .default_value = &key##_def_val,                                                      \
             .human_readable_key = #key},

static config_entry_t prv_config_entries[] = {
#include "configs.def"
#undef CFG_DEFINE
};

/*****************************************************************************
 * Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Functions
 *****************************************************************************/

config_entry_t *configs_get_entry(config_key_t key) {
    if (key >= CFG_NUM_KEYS) {
        return NULL;
    }

    return &prv_config_entries[key];
}

const char *config_key_as_str(config_key_t key) {
    static const char *unknown_key = "Unknown key";

    config_entry_t *entry = configs_get_entry(key);

    if (entry == NULL) {
        return unknown_key;
    }

    return entry->human_readable_key;
}
