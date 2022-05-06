#ifndef HTTP_REQUEST_H__
#define HTTP_REQUEST_H__

#include "esp_camera.h"

size_t http_get_request(char *hostname, char *path, char *query, bool cookie);
size_t http_get_request2(char *hostname, char *path, char *query, bool cookie);
size_t http_post_request(char *hostname, char *path, char *post_data, char *session_cookie);

int https_send_pic(bool cookie, camera_fb_t *pic);

#endif //HTTP_REQUEST_H__