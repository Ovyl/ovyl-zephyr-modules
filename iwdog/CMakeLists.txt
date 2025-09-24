# OVYL IWDOG Module

# Only compile if the feature is enabled
zephyr_library_sources_ifdef(CONFIG_OVYL_IWDOG src/ovyl_iwdog.c)

# Export headers to the whole app
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
