# Copyright (c) 2025 Ovyl
# SPDX-License-Identifier: Apache-2.0

# OVYL Modules top-level CMakeLists.txt
zephyr_library()

# IWDOG module
if(CONFIG_OVYL_IWDOG)
  add_subdirectory(iwdog)
endif()
