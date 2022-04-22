#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_littlefs.h"

static const char *LITTLE_FS = "littlefs_helper.c";

esp_err_t init_littlefs(esp_vfs_littlefs_conf_t conf)
{
    esp_err_t err;

    // init littlefs
    ESP_LOGI(LITTLE_FS, "Initializing littlefs");

    // Use settings defined above to initialize and mount littlefs filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    err = esp_vfs_littlefs_register(&conf);

    if (err != ESP_OK)
    {
        if (err == ESP_FAIL)
        {
            ESP_LOGE(LITTLE_FS, "Failed to mount or format filesystem");
        }
        else if (err == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(LITTLE_FS, "Failed to find littlefs partition");
        }
        else
        {
            ESP_LOGE(LITTLE_FS, "Failed to initialize littlefs (%s)", esp_err_to_name(err));
        }
        return err;
    }

    return err;
}

esp_err_t deinit_littlefs(const char *partition_label)
{
    esp_err_t err;

    // init littlefs
    ESP_LOGI(LITTLE_FS, "Deinitializing Little FS");

    // Use settings defined above to initialize and mount littlefs filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    err = esp_vfs_littlefs_unregister(partition_label);

    if (err != ESP_OK)
    {
        if (err == ESP_FAIL)
        {
            ESP_LOGE(LITTLE_FS, "Failed to unmount filesystem");
        }
        else if (err == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(LITTLE_FS, "Failed to find littlefs partition");
        }
        else
        {
            ESP_LOGE(LITTLE_FS, "Failed to initialize littlefs (%s)", esp_err_to_name(err));
        }
        return err;
    }

    return err;
}

esp_err_t littlefs_partition_info(esp_vfs_littlefs_conf_t conf)
{
    esp_err_t err;

    size_t total = 0, used = 0;
    err = esp_littlefs_info(conf.partition_label, &total, &used);
    if (err != ESP_OK)
    {
        ESP_LOGE(LITTLE_FS, "Failed to get littlefs partition information (%s)", esp_err_to_name(err));
        return err;
    }
    else
    {
        ESP_LOGI(LITTLE_FS, "Partition size: total: %d, used: %d", total, used);
    }

    return err;
}

esp_err_t format_littlefs_partition(esp_vfs_littlefs_conf_t conf)
{
    esp_err_t err;
    
    err = esp_littlefs_format(conf.partition_label);
    if (err != ESP_OK)
    {
        ESP_LOGE(LITTLE_FS, "esp_littlefs_format() error (%s)", esp_err_to_name(err));
        return err;
    }
    return err;
}