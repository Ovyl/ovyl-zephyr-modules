/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bt_version.h
 * @brief Ovyl BT module version definitions
 */

#ifndef OVYL_BT_VERSION_H
#define OVYL_BT_VERSION_H

#include <zephyr/sys/util_macro.h>

#define OVYL_BT_VERSION_MAJOR 1
#define OVYL_BT_VERSION_MINOR 0
#define OVYL_BT_VERSION_PATCH 0

#define OVYL_BT_VERSION_STRING                                                                     \
    STRINGIFY(OVYL_BT_VERSION_MAJOR)                                                               \
    "." STRINGIFY(OVYL_BT_VERSION_MINOR) "." STRINGIFY(OVYL_BT_VERSION_PATCH)
#endif /* OVYL_BT_VERSION_H */
