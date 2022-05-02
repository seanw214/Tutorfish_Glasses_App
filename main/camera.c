#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"

#define BOARD_WROVER_KIT 1

// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN 23 //-1
#define CAM_PIN_RESET -1 
#define CAM_PIN_XCLK 22
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define Y9_GPIO_NUM      14
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      38
#define Y5_GPIO_NUM      33
#define Y4_GPIO_NUM      21
#define Y3_GPIO_NUM      13
#define Y2_GPIO_NUM      15

#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF  4
#define CAM_PIN_PCLK 32

#endif

static const char *TAG = "camera.c";

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

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000, // causes NO-SOI jpg error
    //.xclk_freq_hz = 10000000,
    //.xclk_freq_hz = 8000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    
    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG,PIXFORMAT_RGB565
    .frame_size = FRAMESIZE_VGA, //FRAMESIZE_QVGA,  //FRAMESIZE_UXGA,  //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 12, //0-63 lower number means higher quality

   /*
    // GRAYSCALE
    .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA, //FRAMESIZE_WQXGA <- max_resolution FRAMESIZE_QHD FRAMESIZE_UXGA
    .jpeg_quality = 12,
    */

    .fb_count = 2,      //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


esp_err_t init_camera_pwdn(uint8_t level)
{
    esp_err_t err;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE, // disable interrupt
        .mode = GPIO_MODE_OUTPUT,       // set as output mode
        .pin_bit_mask = 1ULL << CAM_PIN_PWDN,     // bit mask of the pins that you want to set (e.g.GPIO18/19)
        .pull_down_en = 0,              // disable pull-down mode
        .pull_up_en = 1                 // disable pull-up mode
    };

    //configure GPIO with the given settings
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

// esp_err_t toggle_camera_pwdn(uint8_t level)
// {
//     esp_err_t err;

//     char camera_level[5] = "";
//     level == 1 ? strcat(camera_level, "HIGH") : strcat(camera_level, "LOW");

//     ESP_LOGI(TAG, "setting camera level %s", camera_level);

//     err = gpio_set_level(CAM_PIN_PWDN, level);
//     if (err != ESP_OK)
//     {
//         ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
//         return err;
//     }

//     return err;
// }

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
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

void capture_image(void)
{
    ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();

    // use pic->buf to access the image
    ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
    esp_camera_fb_return(pic);

    vTaskDelay(5000 / portTICK_PERIOD_MS);
}
