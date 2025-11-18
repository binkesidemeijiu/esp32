#ifndef __LCD_CONFIG_H__
#define __LCD_CONFIG_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "id_define.h"
#include "init_common.h"
/*
            (GND)-->(GND)
            (VCC)-->(3V3)
            (SCL)-->(21)
            (SDI)-->(47)
            (RST)-->(EN)
            (RS)-->(41)
            (BL)-->(14)
            (SDO)-->(48)
            (CS)-->(42)
            (TP_CS)-->(46)
            (TP_PEN)-->(2)
*/
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
#define LCD_BPP    2 // Bytes Per Pixel for RGB565
#define PIN_NUM_MISO 48
#define PIN_NUM_MOSI 47
#define PIN_NUM_CLK  21
#define PIN_NUM_CS   42
#define LCD_HOST    SPI3_HOST
#define PARALLEL_LINES 16
#define LCD_RS 41  //cmd or data
#define LCD_BL 14 //背光

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define LCD_SEND_CMD(_spi, _cmd) lcd_send(_spi, _cmd, NULL, 0)


#define LCD_SEND_CMD_DATA(_spi, _cmd, ...) \
    do { \
        const uint8_t temp_data[] = {__VA_ARGS__}; \
        lcd_send(_spi, _cmd, temp_data, ARRAY_SIZE(temp_data)); \
    } while (0)

// 延时宏
#define LCD_DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))

#define RGB_TO_RBG(color) (((color) & 0xF800) | (((color) & 0x07E0) >> 5) | (((color) & 0x001F) << 5))
esp_err_t lcd_send_raw(spi_device_handle_t spi, const void *data, size_t len, bool is_data);
void lcd_sequence_init(spi_device_handle_t spi);
esp_err_t lcd_send(spi_device_handle_t spi, uint8_t cmd, const uint8_t *data, size_t data_len);
void lcd_display_start(void);
void lcd_set_backlight(bool on);
void lcd_fill_screen_psram(spi_device_handle_t spi,uint16_t color);
#endif
