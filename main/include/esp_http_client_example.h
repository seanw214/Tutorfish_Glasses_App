#ifndef ESP_HTTP_CLIENT_EXAMPLE_H__
#define ESP_HTTP_CLIENT_EXAMPLE_H__

#include "esp_camera.h"

size_t http_get_request_(char *hostname, char *path, char *query, bool cookie);
size_t http_post_request_(char *hostname, char *path, char *post_data, bool cookie);

#endif //ESP_HTTP_CLIENT_EXAMPLE_H__