/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file config_keys.h
 * @brief Definition of configuration keys
 */

#ifndef OVYL_CONFIG_KEYS_H
#define OVYL_CONFIG_KEYS_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Structs, Unions, Enums, & Typedefs
 *****************************************************************************/

#define CFG_DEFINE(key, type, default_val) key,
/**
 * @brief Definition of configuration keys
 */
typedef enum {
#include <ovyl/configs.def>
    CFG_NUM_KEYS
} config_key_t;
#undef CFG_DEFINE

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* OVYL_CONFIG_KEYS_H */
