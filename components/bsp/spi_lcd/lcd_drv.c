#include "lcd_config.h"

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
    debug_info("start display\n");
}
static void lcd_ctrl_fcn(uint16_t total_len,uint8_t* buffer)
{
 /*以后用于实现显示uart发送数据到lcd*/
}
static list_os_deal_with_t lcd_entry = {
    .compare_id = LED_CTRL,
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