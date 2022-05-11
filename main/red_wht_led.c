#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "red_wht_led.c";

esp_err_t set_tx_level(uint8_t level)
{
    esp_err_t err = gpio_set_level(GPIO_NUM_1, level);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }
    return err;
}

esp_err_t set_rx_level(uint8_t level)
{
    esp_err_t err = gpio_set_level(GPIO_NUM_3, level);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }
    return err;
}

esp_err_t init_red_wht_led(void)
{
    esp_err_t err;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,                              // disable interrupt
        .mode = GPIO_MODE_OUTPUT,                                    // set as output mode
        .pin_bit_mask = (1ULL << GPIO_NUM_1) | (1ULL << GPIO_NUM_3), // bit mask of the pins that you want to set (e.g.GPIO18/19)
        .pull_down_en = 1,                                           // enable pull-down mode
        .pull_up_en = 0                                              // disable pull-up mode
    };

    //configure GPIO with the given settings
    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_config() err: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_set_level(GPIO_NUM_1, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_set_level(GPIO_NUM_3, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}