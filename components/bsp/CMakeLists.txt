set(src_dirs
            usart
            led
            spi_lcd
            sdcard
            wifi
            camera
            common)

set(include_dirs
            usart
            led
            spi_lcd
            sdcard
            wifi
            camera
            common)

set(requires
            driver
            fatfs
            esp_event
            esp_wifi
            esp_netif
            esp32-camera 
            esp_new_jpeg
            esp_timer)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})



component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)