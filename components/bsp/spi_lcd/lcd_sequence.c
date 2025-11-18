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

void lcd_fill_screen_psram(spi_device_handle_t spi,uint16_t color)
{
    size_t buffer_size = LCD_WIDTH * LCD_HEIGHT * LCD_BPP;
    uint16_t *frame_buffer = NULL,i;
    //color = RGB_TO_RBG(color);
    debug_info("Attempting to allocate %d bytes in PSRAM for framebuffer...", buffer_size);
    frame_buffer = (uint16_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

    if (frame_buffer == NULL) {
        debug_info("FATAL: Failed to allocate framebuffer in PSRAM!");
        return; // 内存分配失败，直接返回
    }

    debug_info("Framebuffer allocated in PSRAM at %p", frame_buffer);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        frame_buffer[i] = color;
    }

    debug_info("Framebuffer filled with color 0x%04X", color);
    lcd_send(spi, 0x2C, NULL, 0); // 发送内存写命令
    debug_info("Sending framebuffer to LCD...");

    for(i = 0;i<LCD_WIDTH;i++)
        lcd_send_raw(spi, frame_buffer,LCD_HEIGHT*2, true);

    debug_info("Sending complete.");

    heap_caps_free(frame_buffer);
}
static void lcd_read_reg(spi_device_handle_t spi, uint8_t reg, uint8_t* data, int len) {
    // 发送要读取的命令
    LCD_SEND_CMD(spi, reg);
    // 读取返回的数据
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * len; // len in bytes
    t.rxlength = 8 * len;
    t.rx_buffer = data;
    spi_device_polling_transmit(spi, &t);

}
void lcd_sequence_init(spi_device_handle_t spi)
{
 // 初始化lcd的指令序列
 #if 1
 uint8_t id_data[4] = {0};
    lcd_read_reg(spi,0x04,id_data,3);
    ESP_LOGI("LCD_DETECT", "ID from 0x04: 0x%02X 0x%02X 0x%02X", id_data[0], id_data[1], id_data[2]);
    lcd_send(spi,0x11, NULL, 0); 
    LCD_DELAY_MS(255);
    //lcd_send(spi,0x13, NULL, 0);
    LCD_SEND_CMD_DATA(spi,0x36,0x00);
   // LCD_SEND_CMD_DATA(spi,0xB6,0x0A,0x82); 
  //  LCD_SEND_CMD_DATA(spi,0xB0,0x00,0xE0); // 5 to 6 bit conversion: r0 = r5, b0 = b5
    LCD_SEND_CMD_DATA(spi,0x3a,0x05);
    LCD_DELAY_MS(10);
    LCD_SEND_CMD_DATA(spi,0xB2, 0x0c, 0x0c, 0x00, 0x33, 0x33); 
    LCD_SEND_CMD_DATA(spi,0xB7, 0x35); // Voltages: VGH / VGL
    LCD_SEND_CMD_DATA(spi,0xBB, 0x19); // JLX240 display datasheet	
    LCD_SEND_CMD_DATA(spi,0xC0, 0x2C); 
    LCD_SEND_CMD_DATA(spi,0xC2, 0x01); 
    LCD_SEND_CMD_DATA(spi,0xC3, 0x13); // voltage VRHS
    LCD_SEND_CMD_DATA(spi,0xC4, 0x20);
    LCD_SEND_CMD_DATA(spi,0xC6, 0x0f); 
    LCD_SEND_CMD_DATA(spi,0xD0, 0xa4, 0xa1); 
    //LCD_SEND_CMD_DATA(spi,0xD6, 0xa1); 
    LCD_SEND_CMD_DATA(spi,0xE0, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23);
    LCD_SEND_CMD_DATA(spi,0xE1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23);
    //LCD_SEND_CMD_DATA(spi,0xE0, 0xF0, 0x00, 0x04, 0x04, 0x04, 0x05, 0x29, 0x33, 0x3e, 0x38, 0x12, 0x12, 0x28, 0x30);
    //LCD_SEND_CMD_DATA(spi,0xE1, 0xF0, 0x07, 0x0A, 0x0D, 0x0B, 0x07, 0x28, 0x33, 0x3e, 0x36, 0x14, 0x14, 0x29, 0x32);
    lcd_send(spi,0x20, NULL, 0); 
    LCD_SEND_CMD_DATA(spi,0x2A, 0x00, 0x00, 0x00, 0xEF); // 0xEF = 239
    LCD_SEND_CMD_DATA(spi,0x2B, 0x00, 0x00, 0x01, 0x3F); // 0x13F = 319 (如果显示是 240x320)
    LCD_DELAY_MS(120);
#else
    LCD_SEND_CMD(spi, 0x01);
    LCD_DELAY_MS(150);
    LCD_SEND_CMD(spi, 0x11);
    LCD_DELAY_MS(255); // A long delay is required after sleep out
    LCD_SEND_CMD_DATA(spi, 0x36, 0x08);
    LCD_SEND_CMD_DATA(spi, 0x3A, 0x55);
    LCD_SEND_CMD_DATA(spi, 0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33);
    LCD_SEND_CMD_DATA(spi, 0xB7, 0x35);
    LCD_SEND_CMD_DATA(spi, 0xBB, 0x19);
    LCD_SEND_CMD_DATA(spi, 0xC0, 0x2C);
    LCD_SEND_CMD_DATA(spi, 0xC2, 0x01);
    LCD_SEND_CMD_DATA(spi, 0xC3, 0x12);
    LCD_SEND_CMD_DATA(spi, 0xC4, 0x20);
    LCD_SEND_CMD_DATA(spi, 0xC6, 0x0F);
    LCD_SEND_CMD_DATA(spi, 0xD0, 0xA4, 0xA1);
#if 1
    LCD_SEND_CMD_DATA(spi, 0xE0, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23);
    LCD_SEND_CMD_DATA(spi, 0xE1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23);
#endif
    // 有些屏幕需要反色(0x21)，有些不需要(0x20)。如果颜色看起来像底片，就用 0x20。
    LCD_SEND_CMD(spi, 0x20);
#endif
    lcd_send(spi,0x29, NULL, 0); //Display on
    LCD_DELAY_MS(120);
    lcd_set_backlight(true);
}
