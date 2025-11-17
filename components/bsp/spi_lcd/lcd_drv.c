#include "lcd_config.h"
#include "tfcard.h"
spi_device_handle_t spi;
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(LCD_RS, dc);
}
static void ctl_io_init(void)
{
    /*初始化lcd相关io口*/
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL<<LCD_RS) | (1ULL<<LCD_BL)),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
}
static void lcd_spi_init(void)
{
    /*spi初始化代码*/
    esp_err_t ret;
    
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * 320 * 2 + 8
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,     
        .mode = 0,                              
        .spics_io_num = PIN_NUM_CS,             
        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    uget_sd_data(NULL,0);
    ESP_ERROR_CHECK(ret);
}
static void lcd_drv_init(void)
{
    /*初始化lcd*/
    ctl_io_init();
    debug_info("lcd io init");
    lcd_spi_init();
    debug_info("lcd spi init");
    lcd_sequence_init(spi);
    debug_info("lcd sequence init");
}

void lcd_display_start(void)
{
    // 常见的 RGB56
    //02 00 02 02 0f 0f 00 03
    // 红色: 0xF800
    // 绿色: 0x07E0
    // 蓝色: 0x001F
    // 白色: 0xFFFF
    // 黑色: 0x0000

  lcd_fill_screen_psram(spi,0xF800);
#if 0
  LCD_DELAY_MS(5000);
  lcd_fill_screen_psram(spi,0x07E0);
  LCD_DELAY_MS(5000);
  lcd_fill_screen_psram(spi,0x001F);
  LCD_DELAY_MS(5000);
  lcd_fill_screen_psram(spi,0xFFE0);
  LCD_DELAY_MS(5000);
  lcd_fill_screen_psram(spi,0xBD19);
#endif
  // LCD_DELAY_MS(10000);
  // lcd_fill_screen_psram(spi,0x07E0);
}   
void display_update(uint8_t* buf,uint32_t len)
{
    // 占位符函数，用于将显示内容更新到 LCD
    for(;;)
    {
        if(len > PARALLEL_LINES * 320 * 2)
        {
            lcd_send_raw(spi, buf,PARALLEL_LINES * 320 * 2, true);
            len -= PARALLEL_LINES * 320 * 2;
            buf += PARALLEL_LINES * 320 * 2;
        }
        else
        {
            lcd_send_raw(spi, buf,len, true);
            break;
        }
    }
        
}
static void lcd_ctrl_fcn(uint16_t total_len,uint8_t* buffer)
{
    uint16_t color_trnsfer;
    debug_info("total_len=%d",total_len);
 if(total_len >= 7)
 {
    color_trnsfer = (buffer[4]<<8)|buffer[5];
    lcd_fill_screen_psram(spi,color_trnsfer); //绿色
 }
}
static list_os_deal_with_t lcd_entry = {
    .compare_id = LCD_CTRL,
    .os_and_drv_switch_fcn = lcd_ctrl_fcn,
    .pNext = NULL
};

static Peripheral_Init_Entry_t lcd_init = {
    .pName          = "lcd_drv",
    .device_id      = 0x00,
    .pfnPeriphInit  = lcd_drv_init,
    .pfnNvicInit    = NULL, //
    .pfnTaskCreate  = NULL,
    .suspend        = NULL,
    .resume         = NULL,
    .debug_word     = NULL,
    .pNext          = NULL,
};

MODULE_INIT(&lcd_init,&lcd_entry)
