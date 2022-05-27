#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

#include "state_machine.h"
#include "wifi_bt_status.h"
#include "audio_io.h"
#include "touch.h"
#include "menu.h"

static const char *TAG = "touch.c";

#define TOUCH_ISR_THRESH_NO_USE (0)
#define TOUCH_THRESH_PERCENT (90) // 97 when inside enclosure | 90 when using breakout
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

#define SELECTION_COUNT (1)

static uint16_t touch_pad_forward_calibrated_val;
static uint16_t touch_pad_backward_calibrated_val;

bool touch_pad_forward;
bool touch_pad_backward;

uint8_t menu_selection_idx = 0;

static void calibrate_touch_sensors(void)
{
    uint32_t forward_touch_value = 0, backward_touch_value = 0;
    const uint8_t calibration_increments = 255;

    for (uint8_t i = 0; i < calibration_increments; i++)
    {
        touch_pad_read_filtered(TOUCH_PAD_NUM1, &touch_pad_forward_calibrated_val);
        forward_touch_value += touch_pad_forward_calibrated_val;

        touch_pad_read_filtered(TOUCH_PAD_NUM5, &touch_pad_backward_calibrated_val);
        backward_touch_value += touch_pad_backward_calibrated_val;
    }

    touch_pad_forward_calibrated_val = forward_touch_value / calibration_increments;
    ESP_LOGI(TAG, "touch_pad_forward_calibrated_val: touch pad [%d] val is %d", TOUCH_PAD_NUM1, touch_pad_forward_calibrated_val);

    touch_pad_backward_calibrated_val = backward_touch_value / calibration_increments;
    ESP_LOGI(TAG, "touch_pad_backward_calibrated_val: touch pad [%d] val is %d", TOUCH_PAD_NUM5, touch_pad_backward_calibrated_val);
}

static void handle_forward_touch(void)
{
    if (state_machine == TUTORFISH_HOME)
    {
        // printf("handle_forward_touch() menu_selection_idx: %d\n", menu_selection_idx);
        menu_selection_idx >= SELECTION_COUNT ? menu_selection_idx = 0 : ++menu_selection_idx;
    }

    else if (state_machine == TUTORFISH_PLAYBACK_ANSWER)
    {
        repeat_tts_playback = true;
    }
}

static void handle_backward_touch(void)
{
    if (state_machine == TUTORFISH_HOME)
    {
        // printf("handle_backward_touch() menu_selection_idx: %d\n", menu_selection_idx);
        menu_selection_idx <= 0 ? menu_selection_idx = SELECTION_COUNT : --menu_selection_idx;
    }

    else if (state_machine == TUTORFISH_PLAYBACK_ANSWER)
    {
        repeat_tts_playback = true;
    }
}

static void tp_example_read_task(void *pvParameter)
{
    touch_pad_intr_enable();
    uint16_t forward_touch_value;
    uint16_t backward_touch_value;

    const uint16_t forward_touch_threshold = (touch_pad_forward_calibrated_val * 98) / 100;
    const uint16_t backward_touch_threshold = (touch_pad_backward_calibrated_val * 98) / 100;

    ESP_LOGI(TAG, "forward_touch_threshold: %d", forward_touch_threshold);
    ESP_LOGI(TAG, "backward_touch_threshold: %d", backward_touch_threshold);

    while (1)
    {
        touch_pad_read_filtered(TOUCH_PAD_NUM1, &forward_touch_value);
        touch_pad_read_filtered(TOUCH_PAD_NUM5, &backward_touch_value);

        if (forward_touch_value <= forward_touch_threshold && !touch_pad_forward)
        {
            if (audio_buf.system_i2s_playing)
            {
                audio_buf.system_i2s_stop = true;
            }

            ESP_LOGI(TAG, "forward_touch_value: %d", forward_touch_value);
            ESP_LOGI(TAG, "T%d activated!", TOUCH_PAD_NUM1);

            handle_forward_touch();

            vTaskDelay(250 / portTICK_PERIOD_MS);

            touch_pad_forward = true;
        }
        else if (backward_touch_value <= backward_touch_threshold && !touch_pad_backward)
        {
            if (audio_buf.system_i2s_playing)
            {
                audio_buf.system_i2s_stop = true;
            }

            ESP_LOGI(TAG, "backward_touch_value: %d", backward_touch_value);
            ESP_LOGI(TAG, "T%d activated!", TOUCH_PAD_NUM5);

            handle_backward_touch();
            
            vTaskDelay(250 / portTICK_PERIOD_MS);

            touch_pad_backward = true;
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

esp_err_t init_touch(void)
{
    esp_err_t err;
    BaseType_t task_err;

    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing touch pad");
    ESP_ERROR_CHECK(touch_pad_init());

    // Set reference voltage for charging/discharging
    // For most usage scenarios, we recommend using the following combination:
    // the high reference voltage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
    // err = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    err = touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_set_voltage() error: %s", esp_err_to_name(err));
        return err;
    }

    err = touch_pad_config(TOUCH_PAD_NUM1, TOUCH_ISR_THRESH_NO_USE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_config(TOUCH_PAD_NUM1) error: %s", esp_err_to_name(err));
        return err;
    }

    err = touch_pad_config(TOUCH_PAD_NUM5, TOUCH_ISR_THRESH_NO_USE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_config(TOUCH_PAD_NUM5) error: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize and start a software filter to detect slight change of capacitance.
    err = touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_filter_start() error: %s", esp_err_to_name(err));
        return err;
    }

    // TODO : check nvs for calibrated touch sensors values
    calibrate_touch_sensors();

    task_err = xTaskCreatePinnedToCore(
        &tp_example_read_task,
        "touch_pad_read_task",
        2048, // configMINIMAL_STACK_SIZE
        NULL,
        1,
        NULL,
        1);
    if (task_err != pdPASS)
    {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore(tp_example_read_task) err: %d", task_err);

        // attempt to create the home_button with xTaskCreate
        task_err = xTaskCreate(&tp_example_read_task, "tp_example_read_task", 2048, NULL, 10, NULL);
        if (task_err != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate(tp_example_read_task) err: %d", task_err);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Touch pad init complete.");

    return err;
}