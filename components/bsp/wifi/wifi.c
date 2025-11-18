#include "wifi.h"

static const char *TAG = "wifi_sta";
static EventGroupHandle_t s_wifi_event_group;
static TaskHandle_t xwifiTaskHandle;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
void __wifi_link__(void)//用于cmake别把这个点o直接洗白
{
    return;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "重试连接到 AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"连接到 AP 失败");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "获取到 IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
static void wifi_init_sta(void)
{
    // 创建 WiFi 事件组
    s_wifi_event_group = xEventGroupCreate();

    // 初始化底层 TCP/IP 适配器
    ESP_ERROR_CHECK(esp_netif_init());

    // 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 创建默认 WiFi station 接口
    esp_netif_create_default_wifi_sta();

    // 初始化 WiFi 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // 配置 WiFi 连接参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* 设置认证模式为 WPA2-PSK
             * 如果设置为 WIFI_AUTH_OPEN，则表示连接开放网络 */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    // 设置 WiFi 工作模式为 Station 模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // 设置 WiFi 配置
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // 启动 WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta 完成");

    /* 等待连接建立或超过最大重试次数 */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* 处理连接结果 */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "已连接到 SSID:%s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "连接到 SSID:%s 失败", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "意外事件");
    }

    // 注销事件处理函数（如果不再需要）
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    // vEventGroupDelete(s_wifi_event_group);
}



static void tcp_server_task(void *pvParameters) {
    char rx_buffer[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int sock,len;
    // 1. 创建 Socket
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TCP_PORT);

    // 2. 绑定地址
    bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    // 3. 监听端口
    listen(listen_sock, 1); // 允许 1 个连接排队

    while (1) {
        // 4. 接受连接
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        
        if (sock < 0) {
            ESP_LOGE("TCP_SERVER", "Unable to accept connection: errno %d", errno);
            continue;
        }
        ESP_LOGI("TCP_SERVER", "Client connected");

        // 5. 收发数据循环
        while (1) {
            len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
          //  vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (len <= 0) {
                // 客户端断开连接或错误
                ESP_LOGI("TCP_SERVER", "Client disconnected or error: %d", len);
                continue;
            } else {
                rx_buffer[len] = 0; // 确保是C字符串
                ESP_LOGI("TCP_SERVER", "Received: %s", rx_buffer);
                
                // 示例：向客户端发送回执
                send(sock, "ACK: Received!", strlen("ACK: Received!"), 0);
            }
        }
        // 6. 关闭连接
        close(sock);
    }
    close(listen_sock);
    vTaskDelete(NULL);
}
static void wifi_task_init(void)
{
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, &xwifiTaskHandle);
}
#ifdef CONFIG_BUILD_DEBUG_MODE
static void logwifi_word_debug(UBaseType_t uxHighWaterMark)
{
    uxHighWaterMark = uxTaskGetStackHighWaterMark(xwifiTaskHandle);
    debug_info("wifi: %u words remaining\n", uxHighWaterMark);
}
#endif
static Peripheral_Init_Entry_t wifi_init = {
    .pName          = "wifi sta",
    .device_id      = 0x00,
    .pfnPeriphInit  = wifi_init_sta,
    .pfnNvicInit    = NULL, //
    .pfnTaskCreate  = wifi_task_init,
    .suspend        = NULL,
    .resume         = NULL,
#ifdef CONFIG_BUILD_DEBUG_MODE
    .debug_word     = logwifi_word_debug,
#else
    .debug_word     = NULL,
#endif
    .pNext          = NULL,
};

MODULE_INIT(&wifi_init,NULL)