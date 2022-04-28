// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_ota.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <errno.h>
#include <sys/param.h>

//#include "esp_littlefs.h"
#include "littlefs_helper.h"
#include "nvs_flash.h"

#define IMAGE_HEADER_SIZE sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t) + 1
#define DEFAULT_OTA_BUF_SIZE IMAGE_HEADER_SIZE
static const char *TAG = "esp_https_ota";

static const char *LITTLE_FS = "LITTLE_FS";
int bin_size = 0;

typedef enum
{
    ESP_HTTPS_OTA_INIT,
    ESP_HTTPS_OTA_BEGIN,
    ESP_HTTPS_OTA_IN_PROGRESS,
    ESP_HTTPS_OTA_SUCCESS,
} esp_https_ota_state;

struct esp_https_ota_handle
{
    esp_ota_handle_t update_handle;
    const esp_partition_t *update_partition;
    esp_http_client_handle_t http_client;
    char *ota_upgrade_buf;
    size_t ota_upgrade_buf_size;
    int binary_file_len;
    esp_https_ota_state state;
    bool bulk_flash_erase;
};

typedef struct esp_https_ota_handle esp_https_ota_t;

struct esp_littlefs_ota_handle
{
    esp_ota_handle_t update_handle;
    const esp_partition_t *update_partition;
    //char *ota_upgrade_buf;
    //size_t ota_upgrade_buf_size;
    int binary_file_len;
};

typedef struct esp_littlefs_ota_handle esp_littlefs_ota_t;


esp_err_t esp_littlefs_ota(void)
{
    esp_err_t err;

    esp_littlefs_ota_t *littlefs_ota_handle = calloc(1, sizeof(esp_littlefs_ota_t));
    if (!littlefs_ota_handle)
    {
        ESP_LOGE(TAG, "Couldn't allocate memory to upgrade data buffer");
        //*handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    // get the partition to be written into
    littlefs_ota_handle->update_partition = NULL;
    ESP_LOGI(TAG, "Starting OTA...");
    littlefs_ota_handle->update_partition = esp_ota_get_next_update_partition(NULL);
    if (littlefs_ota_handle->update_partition == NULL)
    {
        ESP_LOGE(TAG, "Passive OTA partition not found");
        return err = ESP_FAIL;
    }

   err = deinit_littlefs("audio");
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "deinit_littlefs() err: %s", esp_err_to_name(err));
    }

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/firmware",
        .partition_label = "firmware",
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

    FILE *f = fopen("/firmware/firmware.bin", "r");
    if (f == NULL)
    {
        ESP_LOGE(LITTLE_FS, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    uint8_t f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(LITTLE_FS, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get firmware length
    littlefs_ota_handle->binary_file_len = ftell(f);

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(LITTLE_FS, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    err = esp_ota_begin(littlefs_ota_handle->update_partition, littlefs_ota_handle->binary_file_len, &littlefs_ota_handle->update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        return err;
    }

    char *firmware_buf = malloc(1024);
    int bytes_written = 0;

    while (bytes_written <= littlefs_ota_handle->binary_file_len)
    {
        if ((bytes_written + 1024) > littlefs_ota_handle->binary_file_len)
        {
            int remaining_bytes = littlefs_ota_handle->binary_file_len % 1024;

            // fill buffer with .bin bytes
            int remaining_bytes_read = fread(firmware_buf, sizeof(char), remaining_bytes, f);
            if (remaining_bytes_read != remaining_bytes)
            {
                ESP_LOGE(TAG, "Error: fread() remaining_bytes_read error");
                err = ESP_FAIL;
                break;
            }

            err = esp_ota_write(littlefs_ota_handle->update_handle, firmware_buf, remaining_bytes);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                break;
            }

            bytes_written += remaining_bytes;

            break;
        }

        // fill buffer with .bin bytes
        int bytes_read = fread(firmware_buf, sizeof(char), 1024, f);
        if (bytes_read != 1024)
        {
            ESP_LOGE(TAG, "Error: fread() error");
            err = ESP_FAIL;
            break;
        }

        // write the filled buffer to ota_write handle and repeat
        err = esp_ota_write(littlefs_ota_handle->update_handle, firmware_buf, 1024);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
            break;
        }

        bytes_written += 1024;
    }

    if (err != ESP_OK)
    {
        // TODO : remove erroneous file from littlefs
        fclose(f);
        free(firmware_buf);

        err = esp_ota_abort(littlefs_ota_handle->update_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(LITTLE_FS, "Error: esp_ota_abort failed! err=0x%x", err);
        }

        return err;
    }

    fclose(f);
    free(firmware_buf);

    ESP_LOGI(LITTLE_FS, "bytes_written: %d", bytes_written);
    ESP_LOGI(LITTLE_FS, "Finishing OTA...");

    err = esp_ota_end(littlefs_ota_handle->update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(LITTLE_FS, "Error: esp_ota_end failed! err=0x%x", err);
        return err;
    }

    err = esp_ota_set_boot_partition(littlefs_ota_handle->update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        return err;
    }

    return ESP_OK;
}

esp_err_t get_current_running_app(void)
{
    const esp_app_desc_t *get_app_description = esp_ota_get_app_description();
    printf("get_app_description->project_name: %s\n", get_app_description->project_name);

    if (strcmp(get_app_description->project_name, "smart_glasses_os") == 0)
    {
        ESP_LOGI(TAG, "User is already home.");
        return ESP_FAIL;
    }

    // TODO : if the user requested app is the same as get_app_description->project_name, inform
    // them that "requested app is already open". If not continue...

    //const esp_partition_t *current_partition = esp_ota_get_running_partition();

    //esp_app_desc_t *app_description;
    //memcpy(app_description, get_app_description, sizeof(get_app_description))


    /*
    esp_err_t err = esp_ota_get_partition_description(current_partition, app_description);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_get_partition_description() failed err: %s", esp_err_to_name(err));
        return err;
    }

    printf("app_description->project_name: %s", app_description->project_name);
    */

    return ESP_OK;
}
