#ifndef __INIT_COMMON_H__
#define __INIT_COMMON_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" 
#include "stdio.h"
#include "esp_log.h"
#include <stdint.h>
#define DATA_START 4
#define DATA_LEN 3
#define DATA_TYPE 2

#ifdef CONFIG_BUILD_DEBUG_MODE
    #define DEBUG_INFO(fmt, ...) printf("[DEBUG] [fun = %s: line = %d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #define debug_info(fmt, ...) printf("[DEBUG]" fmt "\n", ##__VA_ARGS__)
    #define debug_sam(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_INFO(fmt, ...) 
    #define debug_info(fmt, ...)
#endif

#define DEBUG_ERROR(fmt, ...) printf("[ERROR!] [fun = %s: line = %d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define DATA_PACT_ACK 0x06
#define DATA_PACT_ERROR() DEBUG_ERROR("Please resend the data\n")
#define SEND_DATA(UART_PORT,data) \
    do { \
         uart_write_bytes(UART_PORT,(const char*)data, strlen((const char*)data));\
    } while(0)

#if 1 //单模块太过独立，该死的编译器会直接给我优化干掉
#define MODULE_INIT(__pEntry, __handler_func) \
    __attribute__((constructor)) \
    static void __register_module(void) \
    { \
        list_add(__pEntry, __handler_func); \
    }
#endif



typedef void (*TaskCreateFunc_t)(void);
typedef void (*PeriphInitFunc_t)(void);
typedef void (*NvicInitFunc_t)(void);
typedef void (*Rtos_debug_word)(UBaseType_t uxHighWaterMark);

typedef void (*App_fcn_deal)(uint16_t total_len,uint8_t* buffer);

typedef struct Peripheral_Init_Entry
{
    const char* pName; // 模块名称
    uint16_t        device_id;       //用于标识唤醒源，1与1<<n (n<=16) 被唤醒源完全占用不可乱写
    PeriphInitFunc_t pfnPeriphInit; // 外设具体初始化函数 (如 UART_Init) (可为NULL)
    NvicInitFunc_t pfnNvicInit;     // 中断 (NVIC) 配置函数 (可为NULL)
    
    // --- FreeRTOS 任务相关 ---
    TaskCreateFunc_t pfnTaskCreate; // 任务创建函数 (可为NULL)
    Rtos_debug_word debug_word;

    void (*suspend)(void);
    void (*resume)(void);           //开始抄袭祖师爷的思想

    struct Peripheral_Init_Entry *pNext; // 链表指针
} Peripheral_Init_Entry_t;
typedef struct list_os_deal_with  //目的就一个单链表简化oc_to_drv_core中无穷进的宏控
{
    uint8_t compare_id; //匹配uart传下来的data_type
    App_fcn_deal os_and_drv_switch_fcn;
    struct list_os_deal_with *pNext; // 链表指针
} list_os_deal_with_t;


void Auto_Initialize_And_Start(void);
void task_start(void);
void resume_all(uint16_t wake_flage);
void suspend_all(uint16_t wake_flage);
void Calculate_the_remaining_space(UBaseType_t uxHighWaterMark);
void list_add(Peripheral_Init_Entry_t *pEntry,list_os_deal_with_t *handler_func);
bool default_switch(uint8_t device_type,uint16_t total_len,uint8_t* buffer);
void esp_log_hex(const char *tag, const uint8_t *data, size_t len);
void printf_hex(const char *tag, const uint8_t *data, size_t len);
#endif // __INIT_COMMON_H__
