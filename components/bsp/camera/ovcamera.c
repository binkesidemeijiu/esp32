#include "ovcamera.h"
#include "id_define.h"
static const char *TAG = "CAM_JPEG_DEC";
static uint8_t *rgb565_out_buf = NULL;
static volatile uint8_t custom_cmd;
static TaskHandle_t cameraTaskHandle = NULL;
static CAMERA_CTL_FLAGES g_camera_flags = CAMERA_NO_INIT;
void __camera_link__(void)
{
    return;
}
static esp_err_t init_camera(void) 
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    
    // 核心配置：使用 JPEG 格式
    config.pixel_format = PIXFORMAT_JPEG; 
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 10; // 质量越低(数字越小)，体积越大，解码越慢；建议10-12
    config.fb_count = 2; // 双缓冲建议开启
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }
    return ESP_OK;
}
static jpeg_dec_config_t dec_config;
static jpeg_dec_handle_t jpeg_dec;
static jpeg_dec_io_t *jpeg_io;
static int8_t camera_ctl_algorithm(CAMERA_CTL camera_status)
{

    //esp_err_t ret;
    jpeg_error_t jpeg_err; 
    switch (camera_status)
    {
    case CAMERA_STOP:
    {
         if (jpeg_dec)
        {
            jpeg_dec_close(jpeg_dec);
            jpeg_dec = NULL;
        }
        if (jpeg_io)
        {
            free(jpeg_io);
            jpeg_io = NULL; 
        }
        if (rgb565_out_buf != NULL)
        {
            free(rgb565_out_buf);
            rgb565_out_buf = NULL; 
        }

        esp_camera_deinit();
        g_camera_flags = CAMERA_NO_INIT;
    }
    break;
    case CAMERA_START:
    {
         // 防止重复初始化
        if (g_camera_flags == CAMERA_INIT_SUCCESS) return 0;

        if(init_camera() != ESP_OK) return -1;

        rgb565_out_buf = (uint8_t*)heap_caps_malloc(OUT_BUF_SIZE, MALLOC_CAP_SPIRAM);
        if (rgb565_out_buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory");
            camera_ctl_algorithm(CAMERA_STOP); // 失败时回滚
            return -1;
        }
        //这里可调解码后大小端
        dec_config.output_type = JPEG_PIXEL_FORMAT_RGB565_BE;
        dec_config.rotate = JPEG_ROTATE_0D;

        jpeg_err = jpeg_dec_open(&dec_config,&jpeg_dec);
        if (jpeg_err != JPEG_ERR_OK)
        {
            ESP_LOGE(TAG, "Failed to create decoder");
            camera_ctl_algorithm(CAMERA_STOP); // 失败时回滚
            return -1;
        }

        jpeg_io = (jpeg_dec_io_t *)calloc(1, sizeof(jpeg_dec_io_t));
        if (!jpeg_io) {
             camera_ctl_algorithm(CAMERA_STOP);
             return -1;
        }
        g_camera_flags = CAMERA_INIT_SUCCESS;

    }
    break;
    case CAMERA_CAP:
    {
        if (g_camera_flags != CAMERA_INIT_SUCCESS) return -1;

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) 
        {
            ESP_LOGE(TAG, "Camera Capture Failed");
            return -1;
        }
        
        // 确保指针有效
        if (jpeg_io && jpeg_dec && rgb565_out_buf) {
            jpeg_io->inbuf = fb->buf;
            jpeg_io->inbuf_len = fb->len;
            jpeg_io->outbuf = rgb565_out_buf;
            
            jpeg_err = jpeg_dec_process(jpeg_dec, jpeg_io);
            if (jpeg_err == JPEG_ERR_OK)
                display_update(rgb565_out_buf, OUT_BUF_SIZE);
            else
                ESP_LOGE(TAG, "Decode Failed: %d",jpeg_err);
        }

        esp_camera_fb_return(fb);

    }
    break;
    default:
        break;
    }
    return 0;
}

static void camera_main_task(void *pvParameters)
{
    int8_t ret;
    // 预分配一个临时指针用于丢弃废帧（如果需要）
    camera_fb_t *fb_warmup = NULL; 
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // ---------------- 启动阶段 ----------------
        if(g_camera_flags == CAMERA_NO_INIT)
        {
            // 尝试启动摄像头
            ret = camera_ctl_algorithm(CAMERA_START);
            if(ret < 0)
            {
                ESP_LOGE(TAG, "Camera Start Failed");
                g_camera_flags = CAMERA_INIT_FAIL;
                // 启动失败，本次命令作废，继续等待下一次信号
                continue; 
            }

            // 刚启动后，传感器需要适应光线，丢弃前1-2帧，或者延时一小会儿，否则第一张图可能是全黑或偏色的
            vTaskDelay(pdMS_TO_TICKS(50)); // 给自动曝光一点时间
            fb_warmup = esp_camera_fb_get();
            if(fb_warmup) esp_camera_fb_return(fb_warmup); // 丢弃第一帧热身帧
        }

        // ---------------- 执行阶段 ----------------
        if(g_camera_flags == CAMERA_INIT_SUCCESS)
        {
            if(custom_cmd == CAMERA_CAP)
            {
                camera_ctl_algorithm(CAMERA_CAP);
                camera_ctl_algorithm(CAMERA_STOP);
            }
            else if (custom_cmd == CAMERA_PREVIEW)
            {
                // 预览模式：死循环连拍
                // 只有当 custom_cmd 被外部任务修改（例如收到 STOP 信号）时才会跳出
                while (custom_cmd == CAMERA_PREVIEW)
                {
                    ret = camera_ctl_algorithm(CAMERA_CAP);
                    if(ret != 0) break; // 捕捉失败保护
                    
                    // 控制帧率，给解码和发送留喘息时间，也让 CPU 有机会响应外部变量变化
                    vTaskDelay(pdMS_TO_TICKS(10)); 
                }
                custom_cmd = CAMERA_WILL_CLOSE; 
                // 退出预览循环，执行关机
                camera_ctl_algorithm(CAMERA_STOP);
            }
            else if (custom_cmd == CAMERA_WILL_CLOSE)
            {
                camera_ctl_algorithm(CAMERA_STOP);
            }
        }
        else
        {
            // 如果初始化失败了，确保状态复位，防止死锁
            if(g_camera_flags != CAMERA_NO_INIT) {
                 camera_ctl_algorithm(CAMERA_STOP);
            }
        }
    }
            
}


static void camera_task_create(void)
{
    BaseType_t xReturn;
    xReturn = xTaskCreate(camera_main_task, "camera_task",10240, NULL,(UBaseType_t)8, &cameraTaskHandle);
    if (xReturn != pdPASS)
    {
        DEBUG_ERROR("camera task create failed!\n");
    }
}
static void camera_cap_ctrl_fcn(uint16_t total_len,uint8_t* buffer)
{
    /*获取一张图片*/ //02 00 30 01 02 00 03 cap 02 00 30 01 03 00 03 pre
    if(buffer[DATA_START] == CAMERA_CAP || buffer[DATA_START] == CAMERA_PREVIEW) //当前仅支持cap和preview2种模式
    {
        custom_cmd = buffer[DATA_START];
        xTaskNotifyGive(cameraTaskHandle);
    }
    else if(buffer[DATA_START] == CAMERA_WILL_CLOSE)
    {
        custom_cmd = CAMERA_WILL_CLOSE;//
    }
    else
    {
        DEBUG_ERROR("camera cmd no !!! \n");
    }
    
}
#ifdef CONFIG_BUILD_DEBUG_MODE
static void loguart_word_debug(UBaseType_t uxHighWaterMark)
{
    uxHighWaterMark = uxTaskGetStackHighWaterMark(cameraTaskHandle);
    debug_info("uart rx task: %u words remaining\n", uxHighWaterMark);
}
#endif
static list_os_deal_with_t camera_entry = {
    .compare_id = CAMERA_CTRL,
    .os_and_drv_switch_fcn = camera_cap_ctrl_fcn,
    .pNext = NULL
};

static Peripheral_Init_Entry_t camera_list_init = {
    .pName          = "lcd_drv",
    .device_id      = 0x00,
    .pfnPeriphInit  = NULL,
    .pfnNvicInit    = NULL, //
    .pfnTaskCreate  = camera_task_create,
    .suspend        = NULL,
    .resume         = NULL,
#ifdef CONFIG_BUILD_DEBUG_MODE
    .debug_word     = loguart_word_debug,
#else
    .debug_word     = NULL,
#endif
    .pNext          = NULL,
};

MODULE_INIT(&camera_list_init,&camera_entry)