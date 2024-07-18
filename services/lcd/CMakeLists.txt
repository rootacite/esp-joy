idf_component_register(SRCS "st7735.cpp"
        INCLUDE_DIRS "." REQUIRES driver
        PRIV_REQUIRES nvs_flash)
