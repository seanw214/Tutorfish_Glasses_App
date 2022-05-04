#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"
#include "nvs_data_struct.h"

#define BOARD_WROVER_KIT 1

// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN 23 //-1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 22
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define Y9_GPIO_NUM 14
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 38
#define Y5_GPIO_NUM 33
#define Y4_GPIO_NUM 21
#define Y3_GPIO_NUM 13
#define Y2_GPIO_NUM 15

#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 4
#define CAM_PIN_PCLK 32

#endif

static const char *TAG = "camera.c";

/*
    FRAMESIZE_96X96,    // 96x96
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_HQVGA,    // 240x176
    FRAMESIZE_240X240,  // 240x240
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_CIF,      // 400x296
    FRAMESIZE_HVGA,     // 480x320
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_XGA,      // 1024x768
    FRAMESIZE_HD,       // 1280x720
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_UXGA,     // 1600x1200
    // 3MP Sensors
    FRAMESIZE_FHD,      // 1920x1080
    FRAMESIZE_P_HD,     //  720x1280
    FRAMESIZE_P_3MP,    //  864x1536
    FRAMESIZE_QXGA,     // 2048x1536
    // 5MP Sensors
    FRAMESIZE_QHD,      // 2560x1440
    FRAMESIZE_WQXGA,    // 2560x1600
    FRAMESIZE_P_FHD,    // 1080x1920
    FRAMESIZE_QSXGA,    // 2560x1920
*/

camera_config_t setup_camera_config(void)
{
    const uint8_t base_jpg_quality = 6;
    uint8_t jpg_quality = base_jpg_quality;

    if (nvs_data.jpeg_quality_exponent > 0 && nvs_data.jpeg_quality_exponent < 6)
    {
        // exponential growth formula
        // x(t) = x0 * (1 + (r / 100))t

        // x(t) = growth
        // x0 = intial value
        // r = rate of growth (converts to percentage eg. 33% growth rate)
        // t = time

        // 0 jpeg_quality_exponent = 6
        // 6 * 1.33^1 = 8
        // 6 * 1.33^2 = 11
        // 6 * 1.33^3 = 14
        // 6 * 1.33^4 = 19
        // 6 * 1.33^5 = 25

        jpg_quality = (uint8_t)round(base_jpg_quality * pow(1.33, nvs_data.jpeg_quality_exponent));
    }

    ESP_LOGI(TAG, "setup_camera_config()");
    ESP_LOGI(TAG, "nvs_data.jpeg_quality_exponent: %d", nvs_data.jpeg_quality_exponent);
    ESP_LOGI(TAG, "jpg_quality: %d", jpg_quality);

    const camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sscb_sda = CAM_PIN_SIOD,
        .pin_sscb_scl = CAM_PIN_SIOC,

        .pin_d0 = Y2_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d7 = Y9_GPIO_NUM,

        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG,PIXFORMAT_RGB565
        .frame_size = FRAMESIZE_UXGA,   // FRAMESIZE_FHD, //FRAMESIZE_QVGA,  //FRAMESIZE_UXGA,  //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
        .jpeg_quality = jpg_quality,    // 0-63 lower number means higher quality

        .fb_count = 1, // if more than one, i2s runs in continuous mode. Use only with JPEG
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    return camera_config;
}

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d0 = Y2_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,

    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    // NOTE : I (2045) cam_hal: Allocating 629145 Byte frame buffer in PSRAM
    // TODO : try to allocate more RAM

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG,PIXFORMAT_RGB565
    .frame_size = FRAMESIZE_UXGA,   // FRAMESIZE_FHD, //FRAMESIZE_QVGA,  //FRAMESIZE_UXGA,  //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 24,             // 0-63 lower number means higher quality

    // FRAMESIZE_XGA 8
    // FRAMESIZE_VGA 8

    // FRAMESIZE_HD 8
    // FRAMESIZE_HD 4
    // FRAMESIZE_HD 2 looks similar to < 2

    // FRAMESIZE_QXGA 16
    // FRAMESIZE_QXGA 14 // works for screen pics

    // FRAMESIZE_QXGA 10 // works for screen pics, wayyy better than 14

    // FRAMESIZE_UXGA 6 // works on screen, the image is sensor cropped (which is better)
    // FRAMESIZE_UXGA 5 // works but difference is minute

    // TODO : create function that decreases the jpeg_quality if the image does not capture (read/write from nvs)
    // TODO : sepearate shutter from countdown, play shutter when the image has actually captured

    // NOTE : time (t) increases by 1 after every failed camera capture until it reaches 5
    //
    // upon successful capture, decrease the (t) variable by 1

    /*
      // GRAYSCALE
      .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
      .frame_size = FRAMESIZE_HD, //FRAMESIZE_WQXGA <- max_resolution FRAMESIZE_QHD FRAMESIZE_UXGA
      .jpeg_quality = 12,
      */

    .fb_count = 1, // if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

esp_err_t init_camera_pwdn(uint8_t level)
{
    esp_err_t err;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,       // disable interrupt
        .mode = GPIO_MODE_OUTPUT,             // set as output mode
        .pin_bit_mask = 1ULL << CAM_PIN_PWDN, // bit mask of the pins that you want to set (e.g.GPIO18/19)
        .pull_down_en = 0,                    // disable pull-down mode
        .pull_up_en = 1                       // disable pull-up mode
    };

    // configure GPIO with the given settings
    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_config() err: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_set_level(CAM_PIN_PWDN, level);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

esp_err_t toggle_camera_pwdn(uint8_t level)
{
    esp_err_t err;

    err = gpio_set_level(CAM_PIN_PWDN, level);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

esp_err_t init_camera(void)
{
    // initialize the camera
    //esp_err_t err = esp_camera_init(&camera_config);

    camera_config_t camera_config = setup_camera_config();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

// void capture_image(void)
// {
//     ESP_LOGI(TAG, "Taking picture...");
//     camera_fb_t *pic = esp_camera_fb_get();

//     // use pic->buf to access the image
//     ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
//     esp_camera_fb_return(pic);

//     vTaskDelay(5000 / portTICK_PERIOD_MS);
// }
