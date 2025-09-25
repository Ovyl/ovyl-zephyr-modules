/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file config_mgr.h
 * @brief Configuration manager interface
 */

#ifndef OVYL_CONFIG_MGR_H
#define OVYL_CONFIG_MGR_H

#include <ovyl/configs.h>

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Structs, Unions, Enums, & Typedefs
 *****************************************************************************/

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Initialize configuration manager
 */
void config_mgr_init(void);

/**
 * @brief Get value of configuration for key
 *
 * @param key Key
 * @param dst Buffer to write value to
 * @param size Size of buffer
 * @return Returns true on success
 */
bool config_mgr_get_value(config_key_t key, void *dst, size_t size);

/**
 * @brief Set value for given key
 *
 * @param key Key
 * @param src Source buffer
 * @param size Size of source
 * @return Returns true on success
 */
bool config_mgr_set_value(config_key_t key, const void *src, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* OVYL_CONFIG_MGR_H */
