#ifndef CAMERA_H__
#define CAMERA_H__

esp_err_t init_camera_pwdn(uint8_t level);
esp_err_t toggle_camera_pwdn(uint8_t level);
esp_err_t init_camera(void);
void capture_image(void);

#define CAMERA_OFF (1)
#define CAMERA_ON (0)

#endif //CAMERA_H__