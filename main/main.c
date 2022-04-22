/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "littlefs_helper.h"

void app_main(void)
{
    esp_err_t err;

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
}
