if(CONFIG_ZMK_VIA)
  zephyr_library()
  zephyr_library_sources(pipar_custom_keycodes.c)
endif()
