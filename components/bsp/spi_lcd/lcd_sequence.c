#include "lcd_config.h"

esp_err_t lcd_send_raw(spi_device_handle_t spi, const void *data, size_t len, bool is_data)
{
    if (len == 0) {
        return ESP_OK;
    }
    
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       // 清零事务结构体
    t.length = len * 8;             // 长度（位）
    t.tx_buffer = data;             // 发送缓冲区
    t.user = (void*)(is_data ? 1 : 0); // 在回调函数中控制 LCD_RS
    t.flags = 0;                    // 可以添加 SPI_TRANS_CS_KEEP_ACTIVE 等标志，如果需要

    // 使用阻塞式发送
    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    return ret;
}
esp_err_t lcd_send(spi_device_handle_t spi, uint8_t cmd, const uint8_t *data, size_t data_len)
{
    esp_err_t ret;

    // 1. 发送命令 (Command)
    ret = lcd_send_raw(spi, &cmd, 1, false);
    if (ret != ESP_OK) {
        return ret;
    }

    // 2. 发送随后的数据 (Data)，如果存在
    if (data != NULL && data_len > 0) {
        ret = lcd_send_raw(spi, data, data_len, true);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}
void lcd_set_backlight(bool on)
{
    gpio_set_level(LCD_BL, on ? 1 : 0);
}
void lcd_sequence_init(spi_device_handle_t spi)
{
 // 初始化lcd的指令序列
    lcd_send(spi,0x11, NULL, 0); 
    LCD_DELAY_MS(120);
    lcd_send(spi,0x13, NULL, 0);
    LCD_SEND_CMD_DATA(spi,0x36,0x00);
    LCD_SEND_CMD_DATA(spi,0xB6,0x0A,0x82); 
    LCD_SEND_CMD_DATA(spi,0xB0,0x00,0xE0); // 5 to 6 bit conversion: r0 = r5, b0 = b5
    LCD_SEND_CMD_DATA(spi,0x3a,0x55);
    LCD_DELAY_MS(10);
    LCD_SEND_CMD_DATA(spi,0xB2, 0x0c, 0x0c, 0x00, 0x33, 0x33); 
    LCD_SEND_CMD_DATA(spi,0xB7, 0x35); // Voltages: VGH / VGL
    LCD_SEND_CMD_DATA(spi,0xBB, 0x28); // JLX240 display datasheet	
    LCD_SEND_CMD_DATA(spi,0xC0, 0x0C); 
    LCD_SEND_CMD_DATA(spi,0xC2, 0x01, 0xFF); 
    LCD_SEND_CMD_DATA(spi,0xC3, 0x10); // voltage VRHS
    LCD_SEND_CMD_DATA(spi,0xC4, 0x20);
    LCD_SEND_CMD_DATA(spi,0xC6, 0x0f); 
    LCD_SEND_CMD_DATA(spi,0xD0, 0xa4, 0xa1); 
    LCD_SEND_CMD_DATA(spi,0xE0, 0xd0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0e, 0x12, 0x14, 0x17);
    LCD_SEND_CMD_DATA(spi,0xE1, 0xd0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x31, 0x54, 0x47, 0x0e, 0x1c, 0x17, 0x1b, 0x1e);
    lcd_send(spi,0x21, NULL, 0); 
    LCD_SEND_CMD_DATA(spi,0x2A, 0x00, 0x00, 0x00, 0xEF); // 0xEF = 239
    LCD_SEND_CMD_DATA(spi,0x2B, 0x00, 0x00, 0x01, 0x3F); // 0x13F = 319 (如果显示是 240x320)
    LCD_DELAY_MS(120);
    lcd_send(spi,0x29, NULL, 0); //Display on
    LCD_DELAY_MS(120);
    lcd_set_backlight(true);
}