#include "init_common.h"
static list_os_deal_with_t *listHead = NULL;
static Peripheral_Init_Entry_t *g_pInitListHead = NULL;

void Register_Peripheral_Task(Peripheral_Init_Entry_t *pEntry)
{
    if (pEntry == NULL) return;

    // 2. 将新节点插入到链表头部
    pEntry->pNext = g_pInitListHead;
    g_pInitListHead = pEntry;
    
}
void Auto_Initialize_And_Start(void)
{
    Peripheral_Init_Entry_t *pCurrent;
    pCurrent = g_pInitListHead;
    while (pCurrent != NULL)
    {
        debug_sam("Registered module: %s\n", pCurrent->pName);
        
        if (pCurrent->pfnPeriphInit)
        {
            pCurrent->pfnPeriphInit();
        }

        if (pCurrent->pfnNvicInit)
        {
            pCurrent->pfnNvicInit();
        }

        pCurrent = pCurrent->pNext;
    }

 
}
void task_start(void)
{
    //任务创建
    Peripheral_Init_Entry_t *pCurrent;
    pCurrent = g_pInitListHead;
    while (pCurrent != NULL)
    {
        if (pCurrent->pfnTaskCreate)
        {
            pCurrent->pfnTaskCreate(); // 内部调用 xTaskCreate
        }
        pCurrent = pCurrent->pNext;
    }
}
void resume_all(uint16_t wake_flage)
{
    Peripheral_Init_Entry_t *pCurrent;
    pCurrent = g_pInitListHead;
    while (pCurrent != NULL)    //strcmp(str1, str2) == 0
    {
        if (pCurrent->resume && !(wake_flage & pCurrent->device_id))
        {
            pCurrent->resume();
        }
        pCurrent = pCurrent->pNext;
    }
}
void suspend_all(uint16_t wake_flage)
{
    Peripheral_Init_Entry_t *pCurrent;
    pCurrent = g_pInitListHead;
    while (pCurrent != NULL)
    {
        if (pCurrent->suspend && !(wake_flage & pCurrent->device_id))
        {
            pCurrent->suspend();
        }
        pCurrent = pCurrent->pNext;
    }
}
void Calculate_the_remaining_space(UBaseType_t uxHighWaterMark)
{
    Peripheral_Init_Entry_t *pCurrent;
    pCurrent = g_pInitListHead;
    while (pCurrent != NULL)
    {
        if (pCurrent->debug_word)
        {
            pCurrent->debug_word(uxHighWaterMark);
        }
        pCurrent = pCurrent->pNext;
    }
}


static void RegisterDeviceHandler(list_os_deal_with_t *handler_func)
{
    if (handler_func == NULL) {
        return;
    }
    handler_func->pNext = listHead;
    listHead = handler_func;
}

void list_add(Peripheral_Init_Entry_t *pEntry,list_os_deal_with_t *handler_func)
{
    Register_Peripheral_Task(pEntry);
    RegisterDeviceHandler(handler_func);
}
#define list_for_each(pos, head) \
	for (pos = head;pos != NULL;pos = pos->pNext)

bool default_switch(uint8_t device_type,uint16_t total_len,uint8_t* buffer)
{
    list_os_deal_with_t *pCurrent = listHead;
    // 遍历链表
    list_for_each(pCurrent,listHead)
    {
        if (pCurrent->compare_id == device_type) 
        {
            // 找到匹配项，执行对应的回调函数
            if (pCurrent->os_and_drv_switch_fcn != NULL) {
                pCurrent->os_and_drv_switch_fcn(total_len,buffer);
                return true; // 成功执行
            }
            return true; 
        }
    }
    return false;
    
}

void esp_log_hex(const char *tag, const uint8_t *data, size_t len) 
{ //log不容易出问题版本

    if (data == NULL || len == 0) 
    {
        ESP_LOGI(tag, "Data: (null)");
        return;
    }
    // 每个字节需要3个字符 (如 "AB ")，最后还需要一个 '\0' 结尾符
    size_t str_len = len * 3 + 1;

    char *hex_string = (char *)malloc(str_len);

    if (hex_string == NULL) 
    {
        ESP_LOGE(tag, "Failed to allocate memory for hex string");
        return;
    }

    for (size_t i = 0; i < len; ++i) 
    {
        sprintf(hex_string + i * 3, "%02X ", data[i]);
    }

    ESP_LOGI(tag, "Data: %s", hex_string);
    free(hex_string);

}
void printf_hex(const char *tag, const uint8_t *data, size_t len) 
{

    printf("%s: \n", tag);
    for (size_t i = 0; i < len; ++i) 
    {
        printf("%02X ", data[i]);
    }
    printf("\n");

}