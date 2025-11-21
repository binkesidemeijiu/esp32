#ifndef __OV_2640_H__
#define __OV_2640_H__
#include "init_common.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

// 引入组件头文件
#include "esp_camera.h"
//#include "jpeg_decoder.h" // 对应 espressif/esp_jpeg 组件
#include "esp_jpeg_dec.h"

extern void display_update(uint8_t* buf, uint32_t len);
#define DEC_IMAGE_WIDTH  320
#define DEC_IMAGE_HEIGHT 240
#define OUT_BUF_SIZE  (DEC_IMAGE_WIDTH*DEC_IMAGE_HEIGHT*2)

#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D7 16
#define CAM_PIN_D6 17
#define CAM_PIN_D5 18
#define CAM_PIN_D4 12
#define CAM_PIN_D3 10
#define CAM_PIN_D2 8
#define CAM_PIN_D1 9
#define CAM_PIN_D0 11
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13


typedef enum {
    CAMERA_START,
    CAMERA_STOP,
    CAMERA_CAP,
    CAMERA_PREVIEW,
    CAMERA_VIDEO,
    CAMERA_WILL_CLOSE,
} CAMERA_CTL;

typedef enum {
    CAMERA_INIT_SUCCESS,
    CAMERA_INIT_FAIL,
    CAMERA_NO_INIT,
    CAMERA_POLLING,
} CAMERA_CTL_FLAGES;
#endif