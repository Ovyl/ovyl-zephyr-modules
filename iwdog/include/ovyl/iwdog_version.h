/*
 * Copyright (c) 2025 Ovyl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ovyl_iwdog_version.h
 * @brief Ovyl Internal watchdog version definitions
 */

#ifndef OVYL_IWDOG_VERSION_H
#define OVYL_IWDOG_VERSION_H

#define OVYL_IWDOG_VERSION_MAJOR 1
#define OVYL_IWDOG_VERSION_MINOR 0
#define OVYL_IWDOG_VERSION_PATCH 0

#define OVYL_IWDOG_VERSION_STRING                                                                  \
    STRINGIFY(OVYL_IWDOG_VERSION_MAJOR)                                                            \
    "." STRINGIFY(OVYL_IWDOG_VERSION_MINOR) "." STRINGIFY(OVYL_IWDOG_VERSION_PATCH)

#endif /* OVYL_IWDOG_VERSION_H */
