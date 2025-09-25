# Copyright (c) 2025 Ovyl
# SPDX-License-Identifier: Apache-2.0

# Ovyl Config Module

# Only compile if the feature is enabled
zephyr_library_sources_ifdef(CONFIG_OVYL_CONFIG src/config.c)

# Export headers to the whole app
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
