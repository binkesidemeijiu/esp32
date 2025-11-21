#include "tfcard.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <dirent.h> 
extern void display_update(uint8_t* buf,uint32_t len);
static const char *TAG = "TFCARD_DEMO";


void tf_init(void)
{
    sdmmc_card_t *card = NULL;
         // 配置SPI总线
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = TF_MOSI,
        .miso_io_num = TF_MISO,
        .sclk_io_num = TF_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    // 初始化SPI总线
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus no!: %s", esp_err_to_name(ret));
        return;
    }
    // 配置SD卡
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = TF_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    
    // 挂载文件系统
   ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "sd mount no! if format_if_mount_failed = true?");
        } else {
            ESP_LOGE(TAG, "init sd card fail %s", esp_err_to_name(ret));
        }
        return;
    }
    
    // 打印SD卡信息
    sdmmc_card_print_info(stdout, card);
    // 创建并写入文件
#if 0
    ESP_LOGI(TAG, "open sdcard file");
    FILE *f = fopen("/sdcard/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "no file opened");
    } else {
        fprintf(f, "Hello %s!\n", card->cid.name);
        fclose(f);
        ESP_LOGI(TAG, "file written");
    }
#endif
    // 读取文件
    ESP_LOGI(TAG, "read sdcard file");
    FILE *f = fopen("/sdcard/data.txt", "r"); //
    if (f == NULL) {
        ESP_LOGE(TAG, "no file opened");
    } else {
#if 1
    fseek(f, 0, SEEK_END);
    uint32_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET); // 回到文件开头
    uint8_t *png_data = (uint8_t *)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (png_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate PSRAM for PNG data.");
        }
    size_t read_size = fread(png_data, 1, file_size, f);
    ESP_LOGE(TAG, "SEND DATA.TXT LLHHLL.read_size = %d\n",read_size);
    display_update(png_data,read_size);
    heap_caps_free(png_data);
    fclose(f);
#else
        char line[64];
        fgets(line, sizeof(line), f);
        fclose(f);
        // 去除换行符
        char *pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        ESP_LOGI(TAG, "read sdfile: %s", line);
#endif

    }
    
    // 列出根目录文件
#if 0
    ESP_LOGI(TAG, "list sdcard root dir");
    DIR *dir = opendir("/sdcard");
    if (dir == NULL) {
        ESP_LOGE(TAG, "failed to open dir");
    } else {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            ESP_LOGI(TAG, "[file]: %s", entry->d_name);
        }
        closedir(dir);
    }
#endif
    // 卸载文件系统
    esp_vfs_fat_sdcard_unmount("/sdcard", card);
    ESP_LOGI(TAG, "sdcard unmounted");
    
    // 释放SPI总线
    spi_bus_free(host.slot);
}
