#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "usart.h"
#include "led.h"
#include "init_common.h"
#include "lcd_config.h"
void app_main(void)
{
    esp_err_t ret;
   // uint8_t len = 0;
   // uint16_t times = 0;
  //  unsigned char data[RX_BUF_SIZE] = {0};
#ifdef  CONFIG_BUILD_DEBUG_MODE
    UBaseType_t uxHighWaterMark = 0;
#endif
    ret = nvs_flash_init(); /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    usart_init(115200); //uart 脱离auto 最先初始化
    //is_link();
    Auto_Initialize_And_Start();    //外设初始化
    task_start();       //任务创建
    lcd_display_start(); 
    for(;;)
    {
#ifdef CONFIG_BUILD_DEBUG_MODE
        Calculate_the_remaining_space(uxHighWaterMark); //计算剩余空间
#endif
        vTaskDelay(6000);
    }
    
#if 0
	led_init();	/* 初始化LED */
    usart_init(115200);      /* 初始化串口 */

    while(1)
    {
        uart_get_buffered_data_len(UART_UX, (size_t*) &len);  /* 获取环形缓冲区数据长度 */

        if (len > 0)                                           /* 判断数据长度 */
        {
            memset(data, 0, RX_BUF_SIZE);                    /* 对缓冲区清零 */
            uart_read_bytes(UART_UX, data, len, 100);             /* 读数据 */
            uart_write_bytes(UART_UX, (const char*)data, strlen((const char*)data));   /* 写数据 */
        }
        else
        {

            if (times % 30 == 0)
            {
                LED_TOGGLE();
            }

            vTaskDelay(10);
        }
    }
#endif
}
