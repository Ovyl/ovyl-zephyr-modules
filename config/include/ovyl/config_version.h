/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file config_version.h
 * @brief Ovyl configuration module version definitions
 */

#ifndef OVYL_CONFIG_VERSION_H
#define OVYL_CONFIG_VERSION_H

#define OVYL_CONFIG_VERSION_MAJOR 1
#define OVYL_CONFIG_VERSION_MINOR 0
#define OVYL_CONFIG_VERSION_PATCH 0

#define OVYL_CONFIG_VERSION_STRING                                                                 \
    STRINGIFY(OVYL_CONFIG_VERSION_MAJOR)                                                           \
    "." STRINGIFY(OVYL_CONFIG_VERSION_MINOR) "." STRINGIFY(OVYL_CONFIG_VERSION_PATCH)

#endif /* OVYL_CONFIG_VERSION_H */
