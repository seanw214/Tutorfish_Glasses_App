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

static uint32_t s_pad_init_val[TOUCH_PAD_MAX];

bool touch_pad_forward;
bool touch_pad_backward;

uint8_t menu_selection_idx = 0;

static void handle_forward_touch(void)
{
    if (state_machine == TUTORFISH_HOME)
    {
        // printf("handle_forward_touch() menu_selection_idx: %d\n", menu_selection_idx);
        menu_selection_idx >= SELECTION_COUNT ? menu_selection_idx = 0 : ++menu_selection_idx;

        browse_menu();
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

        browse_menu();
    }

    else if (state_machine == TUTORFISH_PLAYBACK_ANSWER)
    {
        repeat_tts_playback = true;
    }
}

/*
  Read values sensed at all available touch pads.
  Use 2 / 3 of read value as the threshold
  to trigger interrupt when the pad is touched.
  Note: this routine demonstrates a simple way
  to configure activation threshold for the touch pads.
  Do not touch any pads when this routine
  is running (on application start).
 */
static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;

    // read filtered value
    touch_pad_read_filtered(TOUCH_PAD_NUM1, &touch_value);
    s_pad_init_val[TOUCH_PAD_NUM1] = touch_value;
    ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", TOUCH_PAD_NUM1, touch_value);

    // set interrupt threshold.
    ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_NUM1, (touch_value * TOUCH_THRESH_PERCENT) / 100));
    printf("TOUCH_PAD_NUM1 threshold: %d\n", (touch_value * TOUCH_THRESH_PERCENT) / 100);

    // read filtered value
    touch_pad_read_filtered(TOUCH_PAD_NUM5, &touch_value);
    s_pad_init_val[TOUCH_PAD_NUM5] = touch_value;
    ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", TOUCH_PAD_NUM5, touch_value);

    // set interrupt threshold.
    ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_NUM5, (touch_value * TOUCH_THRESH_PERCENT) / 100));
    printf("TOUCH_PAD_NUM1 threshold: %d\n", (touch_value * TOUCH_THRESH_PERCENT) / 100);

    // TODO : if the s_pad_init_val are less than a set threshold, recalibrate the touch sensors
}

static void tp_example_read_task(void *pvParameter)
{
    touch_pad_intr_enable();
    uint16_t touch_value;

    while (1)
    {
        if (touch_pad_forward == true)
        {
            if (audio_buf.system_i2s_playing)
            {
                audio_buf.system_i2s_stop = true;
            }

            // touch_pad_read_raw_data(TOUCH_PAD_NUM1, &touch_value);
            // ESP_LOGI(TAG, "touch_value: %d", touch_value);

            ESP_LOGI(TAG, "T%d activated!", TOUCH_PAD_NUM1);

            handle_forward_touch();

            // Clear information on pad activation
            touch_pad_forward = false;

            // Wait a while for the pad being released
            vTaskDelay(400 / portTICK_PERIOD_MS);
        }

        if (touch_pad_backward == true)
        {
            if (audio_buf.system_i2s_playing)
            {
                audio_buf.system_i2s_stop = true;
            }

            // ESP_LOGI(TAG, "touch_value: %d", touch_value);
            // touch_pad_read_raw_data(TOUCH_PAD_NUM5, &touch_value);

            ESP_LOGI(TAG, "T%d activated!", TOUCH_PAD_NUM5);

            handle_backward_touch();

            // Clear information on pad activation
            touch_pad_backward = false;

            // Wait a while for the pad being released
            vTaskDelay(400 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

/*
  Handle an interrupt triggered when a pad is touched.
  Recognize what pad has been touched and save it in a table.
 */
static void tp_example_rtc_intr(void *arg)
{
    uint32_t pad_intr = touch_pad_get_status();

    // ESP_LOGI(TAG, "pad_intr: %d", pad_intr);
    // ESP_LOGI(TAG, "pad_intr >> 5: %d", pad_intr >> 5);
    // ESP_LOGI(TAG, "(pad_intr >> 5) & 0x01: %d", (pad_intr >> 5) & 0x01);

    touch_pad_clear_status();
    if ((pad_intr >> TOUCH_PAD_NUM1) & 0x01)
    {
        touch_pad_forward = true;
    }

    if ((pad_intr >> TOUCH_PAD_NUM5) & 0x01)
    {
        touch_pad_backward = true;
    }
}

esp_err_t init_touch(void)
{
    esp_err_t err;
    BaseType_t task_err;

    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing touch pad");
    ESP_ERROR_CHECK(touch_pad_init());

    // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
    err = touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_set_fsm_mode() error: %s", esp_err_to_name(err));
        return err;
    }

    // Set reference voltage for charging/discharging
    // For most usage scenarios, we recommend using the following combination:
    // the high reference voltage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
    err = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_set_voltage() error: %s", esp_err_to_name(err));
        return err;
    }

    // Init touch pad IO
    // tp_example_touch_pad_init();
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

    // Set thresh hold
    tp_example_set_thresholds();

    touch_pad_forward = false;
    touch_pad_backward = false;

    // Register touch interrupt ISR
    err = touch_pad_isr_register(tp_example_rtc_intr, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "touch_pad_isr_register() error: %s", esp_err_to_name(err));
        return err;
    }

    // Start a task to show what pads have been touched
    // xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL);

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