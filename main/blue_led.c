#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "blue_led.h"

static const char *TAG = "blue_led.c";
static TaskHandle_t toggle_blue_led_task_handle = NULL;

static void toggle_blue_led_task(void *pvParameters);

esp_err_t set_blue_led(uint8_t level)
{
    return gpio_set_level(18, level);
}

int get_blue_led_level(void)
{
    return gpio_get_level(18);
}

void delete_blue_led_task(void)
{
    vTaskDelete(toggle_blue_led_task_handle);
}

esp_err_t start_blue_led_task(void)
{
    BaseType_t task_err;

    task_err = xTaskCreatePinnedToCore(
        toggle_blue_led_task,
        "toggle_blue_led_task",
        2048, //configMINIMAL_STACK_SIZE
        NULL,
        10,
        &toggle_blue_led_task_handle,
        1);
    if (task_err != pdPASS)
    {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore(toggle_blue_led_task) err: %d", task_err);

        // attempt to create the home_button with xTaskCreate
        task_err = xTaskCreate(toggle_blue_led_task, "toggle_blue_led_task", 8192, NULL, 5, NULL);
        if (task_err != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate(toggle_blue_led_task) err: %d", task_err);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

static void toggle_blue_led_task(void *pvParameters)
{
    esp_err_t err;
    uint8_t toggle = 1;

    for (;;)
    {
        toggle = 0 ? toggle == 1 : toggle == 0;

        err = gpio_set_level(18, toggle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "toggle_blue_led_task() gpio_set_level() err: %s", esp_err_to_name(err));
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

esp_err_t init_blue_led(void)
{
    esp_err_t err;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE, // disable interrupt
        .mode = GPIO_MODE_OUTPUT,       // set as output mode
        .pin_bit_mask = 1ULL << 18,     // bit mask of the pins that you want to set (e.g.GPIO18/19)
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

    err = gpio_set_level(18, BLUE_LED_OFF);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level() err: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}