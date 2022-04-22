#ifndef LITTLEFS_HELPERS_H__
#define LITTLEFS_HELPERS_H__

#include "esp_littlefs.h"

esp_err_t init_littlefs(esp_vfs_littlefs_conf_t conf);
esp_err_t littlefs_partition_info(esp_vfs_littlefs_conf_t conf);
esp_err_t format_littlefs_partition(esp_vfs_littlefs_conf_t conf);
esp_err_t deinit_littlefs(const char *partition_label);

#endif //LITTLEFS_HELPERS_H__