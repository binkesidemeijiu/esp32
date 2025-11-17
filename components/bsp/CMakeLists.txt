set(src_dirs
            usart
            led
            spi_lcd
            sdcard
            common)

set(include_dirs
            usart
            led
            spi_lcd
            sdcard
            common)

set(requires
            driver
            fatfs)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})



component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)