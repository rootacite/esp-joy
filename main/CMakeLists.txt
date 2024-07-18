idf_component_register(SRCS "esp-joy.cpp"  "esp_hid_gap.c"
                    INCLUDE_DIRS "." REQUIRES esp_hid driver
        PRIV_REQUIRES nvs_flash)
