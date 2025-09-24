# OVYL Modules top-level CMakeLists.txt
zephyr_library()

# IWDOG module
if(CONFIG_OVYL_IWDOG)
  add_subdirectory(iwdog)
endif()
