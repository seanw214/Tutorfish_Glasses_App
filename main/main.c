#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "littlefs_helper.h"
#include "audio_io.h"

static const char *TAG = "main.c";

void app_main(void)
{
    esp_err_t err;

    // Initialize NVS
    ESP_LOGI(TAG, "nvs_flash_init()");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
    }
    ESP_ERROR_CHECK(err);

    // LittleFs configuration struct
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/audio",
        .partition_label = "audio",
        .format_if_mount_failed = true};

    err = init_littlefs(conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_littlefs() err: %s", esp_err_to_name(err));
    }

    err = littlefs_partition_info(conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "littlefs_partition_info() err: %s", esp_err_to_name(err));
    }

    err = init_i2s();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_i2s() err: %s", esp_err_to_name(err));
    }

    err = malloc_home_sfx();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "malloc_home_sfx() err: %s", esp_err_to_name(err));
    }

    err = playback_audio_file(audio_buf.home_sfx_audio_buf, audio_buf.home_sfx_audio_len, 0.05f, false);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "playback_audio_file(audio_buf.home_sfx_audio_buf) err: %s", esp_err_to_name(err));
    }
}
