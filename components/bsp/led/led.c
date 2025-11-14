#include "led.h"
#include "id_define.h"
#include "init_common.h"

void __led_link__(void)//用于cmake别把这个点o直接洗白
{
    return;
}
static void ledio_init(void)
{
    gpio_config_t gpio_init_struct = {0};

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;         /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT;         /* 输入输出模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_DISABLE;      /* 失能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_ENABLE;   /* 使能下拉 */
    gpio_init_struct.pin_bit_mask = 1ull << LED_GPIO_PIN;   /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                         /* 配置GPIO */

    LED(1);                                                 /* 关闭LED */
}

static void led_ctrl_fcn(uint16_t total_len,uint8_t* buffer)
{
    DEBUG_INFO("\n");
    if(buffer[DATA_START] > 0)
        LED(1);
    else
        LED(0);
}
static list_os_deal_with_t led_entry = {
    .compare_id = LED_CTRL,
    .os_and_drv_switch_fcn = led_ctrl_fcn,
    .pNext = NULL
};

static Peripheral_Init_Entry_t led_init = {
    .pName          = "led",
    .device_id      = 0x00,
    .pfnPeriphInit  = ledio_init,
    .pfnNvicInit    = NULL, //
    .pfnTaskCreate  = NULL,
    .suspend        = NULL,
    .resume         = NULL,
    .debug_word     = NULL,
    .pNext          = NULL,
};

MODULE_INIT(&led_init,&led_entry)