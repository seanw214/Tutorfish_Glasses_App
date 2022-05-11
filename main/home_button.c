#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "audio_io.h"
//#include "http_request.h"
#include "state_machine.h"
#include "nvs.h"
#include "esp_ota.h"
#include "wifi_bt_status.h"
#include "wifi_station.h"
#include "blue_led.h"
#include "menu.h"
#include "init_variables.h"

//#include "esp_intr_alloc.h"
#define ESP_INTR_FLAG_DEFAULT (0)

#define GPIO_HOME_BUTTON (37)

static const char *TAG = "home_button";

static xQueueHandle gpio_evt_queue = NULL;
static TaskHandle_t gpio_task_example_handle = NULL;

static bool button_pressed = false;

static void handle_single_press(void)
{
    esp_err_t err;

    if (state_machine == TUTORFISH_HOME)
    {
        err = menu_selection();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "select_app() err: %s", esp_err_to_name(err));
        }
    }
}

static void handle_double_press(void)
{
    esp_err_t err;

    if (state_machine == TUTORFISH_HOME)
    {
        if (audio_buf.returning_home_wav_audio_buf == NULL)
        {
            err = malloc_returning_home_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_returning_home_wav() err: %s", esp_err_to_name(err));
            }

            if (err == ESP_OK)
            {
                playback_audio_file(audio_buf.returning_home_wav_audio_buf, audio_buf.returning_home_wav_audio_len, audio_volume, false);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "playback_audio_file(returning_home_wav_audio_buf) err: %s", esp_err_to_name(err));
                }

                free_returning_home_wav();
            }
        }

        if (audio_buf.wait_app_loads_00_wav_audio_buf == NULL)
        {
            esp_err_t err_ = malloc_wait_app_loads_00_wav();
            if (err_ != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_wait_app_loads_00_wav() err: %s", esp_err_to_name(err_));
            }

            if (err_ == ESP_OK)
            {
                playback_audio_file(audio_buf.wait_app_loads_00_wav_audio_buf, audio_buf.wait_app_loads_00_wav_audio_len, audio_volume, true);
                if (err_ != ESP_OK)
                {
                    ESP_LOGE(TAG, "playback_audio_file(wait_app_loads_00_wav_audio_buf) err: %s", esp_err_to_name(err_));
                }

                free_wait_app_loads_00_wav();
            }
        }

        err = get_current_running_app();
        if (err == ESP_OK)
        {
            err = esp_littlefs_ota();
            if (err == ESP_OK)
            {
                esp_restart();
            }
            else
            {
                ESP_LOGE(TAG, "esp_littlefs_ota() failed err: %s", esp_err_to_name(err));
            }
        }
        else if (err == ESP_FAIL)
        {
            // error
            playback_error_message();
        }
    }
    else if (state_machine == CONNECT_TO_WIFI)
    {
        wifi_bt_status.user_end_wifi_conn = true;

        tutorfish_home_init = false;
        wifi_bt_status.wifi_conn = false;
        state_machine = TUTORFISH_HOME;

        err = stop_wifi();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "stop_wifi() err: %s", esp_err_to_name(err));
        }
    }

    else if (state_machine == TUTORFISH_SETTINGS)
    {
        if (audio_buf.returning_home_wav_audio_buf == NULL)
        {
            err = malloc_returning_home_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_returning_home_wav() err: %s", esp_err_to_name(err));
            }

            if (err == ESP_OK)
            {
                playback_audio_file(audio_buf.returning_home_wav_audio_buf, audio_buf.returning_home_wav_audio_len, audio_volume, false);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "playback_audio_file(returning_home_wav_audio_buf) err: %s", esp_err_to_name(err));
                }

                free_returning_home_wav();
            }
        }

        tutorfish_settings_init = false;
        tutorfish_home_init = false;
        state_machine = TUTORFISH_HOME;
    }
    else
    {
        ESP_LOGW(TAG, "state_machine: %d does not exists", state_machine);
    }
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    if (!button_pressed)
    {
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    int i = 0;
    int ii = 0;
    static const uint8_t single_press_interval = 5;
    static const uint8_t double_press_interval = 70;
    static bool double_pressed = false;

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            button_pressed = true;

            ESP_LOGI(TAG, "button_pressed");

            while (i++ <= single_press_interval && !double_pressed)
            {
                // printf("i: %d\n", i);

                if (gpio_get_level(io_num) == 1)
                {
                    while (ii++ <= double_press_interval)
                    {
                        // printf("ii: %d\n", ii);

                        // wait for second button press
                        if (gpio_get_level(io_num) == 0)
                        {
                            double_pressed = true;
                            break;
                        }

                        vTaskDelay(10 / portTICK_PERIOD_MS);
                    }
                }

                vTaskDelay(10 / portTICK_PERIOD_MS);
            }

            if (double_pressed)
            {
                // if the current loaded partition is not he glasses firmware, ota load the glasses firmware
                printf("double pressed\n");

                handle_double_press();
            }
            else
            {
                printf("single press\n");
                // https_send_file();

                handle_single_press();
            }

            double_pressed = false;
            button_pressed = false;
            i = 0;
            ii = 0;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

esp_err_t init_home_button(void)
{
    esp_err_t err;
    BaseType_t task_err;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,           // interrupt on falling edge
        .mode = GPIO_MODE_INPUT,                  // set as input mode
        .pin_bit_mask = 1ULL << GPIO_HOME_BUTTON, // bit mask of the pins that you want to set
        .pull_up_en = GPIO_PULLUP_DISABLE         // disable pull-up mode (pulled up by resistor)
    };

    // configure GPIO with the given settings
    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_config() err: %s", esp_err_to_name(err));
        return err;
    }

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

    task_err = xTaskCreatePinnedToCore(
        gpio_task_example,
        "gpio_task_example",
        10000, // configMINIMAL_STACK_SIZE
        NULL,
        5,
        &gpio_task_example_handle,
        1);
    if (task_err != pdPASS)
    {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore(gpio_task_example) err: %d", task_err);

        // attempt to create the home_button with xTaskCreate
        task_err = xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
        if (task_err != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate(gpio_task_example) err: %d", task_err);
            return ESP_FAIL;
        }
    }

    // NOTE : disabled due to camera init call to gpio_install_isr_service
    // install gpio isr service
    // err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "gpio_install_isr_service() err: %s", esp_err_to_name(err));
    //     return err;
    // }

    // hook isr handler for specific gpio pin
    err = gpio_isr_handler_add(GPIO_HOME_BUTTON, gpio_isr_handler, (void *)GPIO_HOME_BUTTON);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_install_isr_service() err: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "init_home_button() complete");

    return err;
}