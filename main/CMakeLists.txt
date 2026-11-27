idf_component_register(SRCS "mesh_main.c"
                        "mesh_v2_link.c"
                        "garland_node.c"
                        "legacy_proto.c"
                        "stack_monitor.c"
                    PRIV_REQUIRES keemash_mesh_core esp_wifi esp_driver_gpio nvs_flash esp_timer esp_system esp_app_format app_update spi_flash bootloader_support
                    INCLUDE_DIRS "." "include")
