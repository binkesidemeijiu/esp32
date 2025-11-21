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
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (s_retry_num < MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry connect AP");
        } 
        else 
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"connect AP fail");
        }    
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "get IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
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

#if 0
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
#endif
}
static void handle_client_session(int sock) 
{
    char rx_buffer[128];
    int len;
    
    while (1) {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len <= 0) {
            ESP_LOGI(TAG, "connect error!");
            break; // 退出会话，返回上一级
        }
        
        rx_buffer[len] = 0;
        ESP_LOGI(TAG, "recv: %s", rx_buffer);
        send(sock, "ACK", 3, 0);
    }
    // 会话结束，关闭 socket
    shutdown(sock, 0);
    close(sock);
}

static void tcp_server_task(void *pvParameters) {
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    char addr_str[128];
    while (1) {
        // 1. 等待 Wi-Fi 连接成功
        ESP_LOGI(TAG, "Waiting for WiFi connection...");
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(TCP_PORT);

        // 2. 创建 Socket
        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // 3. 设置 Socket 选项 (SO_REUSEADDR 是必须的，防止重启后端口占用)
        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 4. 绑定
        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            close(listen_sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        
        // 5. 监听
        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            close(listen_sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        
        ESP_LOGI(TAG, "Socket listening on port %d", TCP_PORT);

        // 6. 接受客户端循环
        for(;;) 
        {
            struct sockaddr_in source_addr;
            socklen_t addr_len = sizeof(source_addr);
            
            // accept 是阻塞的
            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                break; // 跳出内层循环，重建监听 Socket
            }
            
            // 打印客户端 IP

            inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
            ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

            handle_client_session(sock);
            //重新建立链接才会往下走
            shutdown(sock, 0);
            close(sock);
        }
        
        // 只有当 accept 失败或者需要重启服务时才会运行到这里
        close(listen_sock);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
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