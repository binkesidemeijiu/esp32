#include "usart.h"
#include "init_common.h"


static QueueHandle_t uart0_queue;
unsigned char g_rx_buffer[RX_BUF_SIZE] = {0};
void usart_init(uint32_t baudrate)
{
    uart_config_t uart_config;                          /* 串口配置句柄 */

    uart_config.baud_rate = baudrate;                   /* 波特率 */
    uart_config.data_bits = UART_DATA_8_BITS;           /* 字长为8位数据格式 */
    uart_config.parity = UART_PARITY_DISABLE;           /* 无奇偶校验位 */
    uart_config.stop_bits = UART_STOP_BITS_1;           /* 一个停止位 */
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;   /* 无硬件控制流 */
    uart_config.source_clk = UART_SCLK_APB;             /* 配置时钟源 */
    uart_config.rx_flow_ctrl_thresh = 122;              /* 硬件控制流阈值 */
    uart_param_config(UART_UX, &uart_config);          /* 配置uart端口 */

    /* 配置uart引脚 */
    uart_set_pin(UART_UX, UART_TX_GPIO_PIN, UART_RX_GPIO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /* 安装串口驱动 */
    uart_driver_install(UART_UX, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20,&uart0_queue, 0);

    memset(g_rx_buffer, 0, RX_BUF_SIZE);                   /* 对缓冲区清零 */
}
static TaskHandle_t xuartTaskHandle = NULL;
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint16_t len = 0;
    for(;;)
    {
        // 等待UART事件
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY))
        {
          //data [0x02 1b][trng 1b][data type 1b][data len 1b][core data][trng 1b][0x03 1b]
            switch(event.type)
            {
                // 处理接收数据事件
                case UART_DATA:
                {
                    // 读取接收到的数据
                    uart_get_buffered_data_len(UART_UX, (size_t*) &len);
                    if (len > 0)                                           /* 判断数据长度 */
                    {
                        uart_read_bytes(UART_UX,g_rx_buffer, len, 100);
                        if(g_rx_buffer[0] != 0x02 || 
                            g_rx_buffer[len - 1] != 0x03 ||
                            g_rx_buffer[DATA_LEN] != (len - 6))
                        {
                            DEBUG_ERROR("data is no! \n");
                            continue;
                        }
                        default_switch(g_rx_buffer[DATA_TYPE],len,g_rx_buffer);
                        g_rx_buffer[len] = '\0'; // 添加字符串结束符
                        printf_hex("data is",&g_rx_buffer[DATA_START],g_rx_buffer[DATA_LEN]);
                        
                    }
                // 其他事件处理
                default:
                    break;
                }
            }
         }
    }
    vTaskDelete(xuartTaskHandle);
}
static void UART_Rx_Task_Create(void)
{
    BaseType_t xReturn;
    xReturn = xTaskCreate(uart_event_task, "uart_event_task",3072, NULL,(UBaseType_t)8, &xuartTaskHandle);
    if (xReturn != pdPASS)
    {
        DEBUG_ERROR("uart rx task create failed!\n");
    }
}
#ifdef CONFIG_BUILD_DEBUG_MODE
static void loguart_word_debug(UBaseType_t uxHighWaterMark)
{
    uxHighWaterMark = uxTaskGetStackHighWaterMark(xuartTaskHandle);
    debug_info("uart rx task: %u words remaining\n", uxHighWaterMark);
}
#endif

static Peripheral_Init_Entry_t logcmd_uart_init = {
    .pName          = "logcmd_uart",
    .device_id      = 0x00,
    .pfnPeriphInit  = NULL,
    .pfnNvicInit    = NULL, //
    .pfnTaskCreate  = UART_Rx_Task_Create,
    .suspend        = NULL,
    .resume         = NULL,
#ifdef CONFIG_BUILD_DEBUG_MODE
    .debug_word     = loguart_word_debug,
#else
    .debug_word     = NULL,
#endif
    .pNext          = NULL,
};



MODULE_INIT(&logcmd_uart_init,NULL)