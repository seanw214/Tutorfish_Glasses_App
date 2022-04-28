#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#include "littlefs_helper.h"
#include "camera.h"
#include "home_button.h"
#include "touch.h"
#include "menu.h"

#include "blue_led.h"
bool blue_led_task_init = false;

#include "wifi_station.h"
#include "wifi_bt_status.h"
wifi_bt_status_t wifi_bt_status;

#include "state_machine.h"
state_machine_t state_machine;

#include "init_variables.h"
bool tutorfish_home_init = false;
bool tutorfish_submit_question_init = false;
bool tutorfish_settings_init = false;

#include "audio_io.h"
bool repeat_tts_playback = false;

static const char *TAG = "main.c";

esp_err_t setup_sleep(void)
{
    if (blue_led_task_init)
    {
        delete_blue_led_task();
        blue_led_task_init = false;
        
        set_blue_led(0);
    }

    // setup wake trigger
    esp_err_t err = esp_sleep_enable_touchpad_wakeup();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_sleep_enable_touchpad_wakeup() error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "touch to wake enabled.");
    ESP_LOGI(TAG, "Going to light sleep...");

    return esp_light_sleep_start();
}

esp_err_t audio_malloc(void)
{
    esp_err_t err = malloc_attempt_wifi_conn_00_wav();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "malloc_attempt_wifi_conn_00_wav() err: %s", esp_err_to_name(err));
    }

    err = malloc_returning_home_wav_sfx();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "malloc_returning_home_wav_sfx() err: %s", esp_err_to_name(err));
    }

    err = malloc_home_instructions_00_wav();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "malloc_home_instructions_00_wav() err: %s", esp_err_to_name(err));
    }

    err = malloc_exit_this_app_00_wav();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "malloc_home_instructions_00_wav() err: %s", esp_err_to_name(err));
    }

    malloc_tutorfish_home();
    malloc_submit_a_question_00_wav();
    malloc_tutor_fish_settings_00_wav();
    malloc_look_at_your_question_01_wav();
    malloc_the_camera_take_a_pic_01_wav();
    malloc_to_conserve_battery_01_wav();

    malloc_p_wav();

    return err;
}

void app_main(void)
{
    esp_err_t err;

    // Initialize NVS
    ESP_LOGI(TAG, "nvs_flash_init()");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
    }
    ESP_ERROR_CHECK(err);

    // LittleFs configuration struct
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/audio",
        .partition_label = "audio",
        .format_if_mount_failed = true};

    err = init_littlefs(conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_littlefs() err: %s", esp_err_to_name(err));
    }

    err = init_i2s();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_i2s() err: %s", esp_err_to_name(err));
    }

    err = audio_malloc();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "audio_malloc() err: %s", esp_err_to_name(err));
    }

    /*
    err = littlefs_partition_info(conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "littlefs_partition_info() err: %s", esp_err_to_name(err));
    }
    */

    err = init_camera_pwdn(CAMERA_ON);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_camera_pwdn() err: %s", esp_err_to_name(err));
    }

    err = init_camera();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_camera() err: %s", esp_err_to_name(err));
    }

    err = toggle_camera_pwdn(CAMERA_OFF);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
    }

    err = init_home_button();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_home_button() err: %s", esp_err_to_name(err));
    }

    err = init_touch();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_touch() err: %s", esp_err_to_name(err));
    }

    err = init_blue_led();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_blue_led() err: %s", esp_err_to_name(err));
    }

    err = playback_audio_file(audio_buf.welcome_01_wav_audio_buf, audio_buf.welcome_01_audio_len, 0.15f, false);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "playback_audio_file(audio_buf.home_sfx_audio_buf) err: %s", esp_err_to_name(err));
    }

    err = playback_audio_file(audio_buf.home_instructions_00_wav_audio_buf, audio_buf.home_instructions_00_wav_audio_len, 0.15f, true);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "playback_audio_file(audio_buf.home_sfx_audio_buf) err: %s", esp_err_to_name(err));
    }

    err = playback_audio_file(audio_buf.exit_this_app_00_wav_audio_buf, audio_buf.exit_this_app_00_wav_audio_len, 0.15f, true);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "playback_audio_file(audio_buf.home_sfx_audio_buf) err: %s", esp_err_to_name(err));
    }

    state_machine = TUTORFISH_HOME;

    while (true)
    {
        switch (state_machine)
        {
        case CONNECT_TO_WIFI:

            if (!wifi_bt_status.wifi_init)
            {
                err = _esp_wifi_init();
                if (err == ESP_OK)
                {
                    wifi_bt_status.wifi_init = true;
                }
                else
                {
                    ESP_LOGE(TAG, "_esp_wifi_init() err: %s", esp_err_to_name(err));
                }
            }

            if (!wifi_bt_status.wifi_sta_init)
            {
                err = wifi_init_sta();
                if (err == ESP_OK)
                {
                    wifi_bt_status.wifi_sta_init = true;
                }
                else
                {
                    ESP_LOGE(TAG, "wifi_init_sta() error: %s", esp_err_to_name(err));
                    return;
                }
            }

            if (!blue_led_task_init)
            {
                start_blue_led_task() == ESP_OK ? blue_led_task_init = true : blue_led_task_init == false; // true == 1
            }

            wifi_bt_status.user_end_wifi_conn = false;

            err = start_wifi();
            if (err == ESP_OK)
            {
                if (blue_led_task_init)
                {
                    delete_blue_led_task();
                    blue_led_task_init = false;
                }

                set_blue_led(1);

                wifi_bt_status.wifi_init = true;
                wifi_bt_status.wifi_conn = true;

                state_machine = TUTORFISH_CHECK_ACCOUNT;
                break;
            }
            else
            {
                // wifi connection retry timed-out

                if (blue_led_task_init)
                {
                    delete_blue_led_task();
                    blue_led_task_init = false;
                }

                set_blue_led(0);

                err = stop_wifi();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "stop_wifi() error: %s", esp_err_to_name(err));
                }

                tutorfish_home_init = false;
                wifi_bt_status.wifi_init = true;
                wifi_bt_status.wifi_conn = false;

                if (!wifi_bt_status.user_end_wifi_conn)
                {
                    ESP_LOGW(TAG, "err: %s, Wifi connection unsuccessful. Make sure that your wifi is turned on and try again.", esp_err_to_name(err));
                }

                err = playback_audio_file(audio_buf.returning_home_wav_audio_buf, audio_buf.returning_home_wav_audio_len, 0.2f, false);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
                }

                state_machine = TUTORFISH_HOME;
                break;
            }

            break;
        case TUTORFISH_HOME:
            if (wifi_bt_status.wifi_init && !wifi_bt_status.wifi_conn)
            {
                err = _wifi_deinit();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "_wifi_deinit() err: %s", esp_err_to_name(err));

                    // TODO : play audio "An error occurred with your wifi connection."

                    // happens when mobile hotspot is disconnected abruptly
                    if (err == ESP_ERR_WIFI_BASE + 3) // err = ESP_ERR_WIFI_NOT_STOPPED (ESP_ERR_WIFI_BASE + 3)
                    {
                        ESP_LOGI(TAG, "stop_wifi()");

                        err = stop_wifi();
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "stop_wifi() error: %s", esp_err_to_name(err));
                        }

                        ESP_LOGI(TAG, "_wifi_deinit()");

                        err = _wifi_deinit();
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "_wifi_deinit() err: %s", esp_err_to_name(err));
                        }
                    }
                }

                wifi_bt_status.wifi_init = false;

                if (blue_led_task_init)
                {
                    delete_blue_led_task();
                    blue_led_task_init = false;
                }

                set_blue_led(0);

                tutorfish_home_init = false;
            }

            if (!tutorfish_home_init)
            {
                ESP_LOGI(TAG, "TUTORFISH_HOME");

                tutorfish_home_init = true;
                tutorfish_submit_question_init = false;
            }

            break;
        case TUTORFISH_SUBMIT_QUESTION:

            if (!tutorfish_submit_question_init)
            {
                ESP_LOGI(TAG, "TUTORFISH_SUBMIT_QUESTION");

                play_submit_question_instructions();

                tutorfish_submit_question_init = true;

                if (setup_sleep() != ESP_OK)
                {
                    ESP_LOGE(TAG, "TUTORFISH_SUBMIT_QUESTION setup_sleep() err: %s", esp_err_to_name(err));
                }
            }
            else
            {
                // the glasses are being awaken after selecting TUTORFISH_SUBMIT_QUESTION
                state_machine = CONNECT_TO_WIFI;
            }

            break;
        case TUTORFISH_SETTINGS:

            if (!tutorfish_settings_init)
            {
                ESP_LOGI(TAG, "TUTORFISH_SETTINGS");

                tutorfish_settings_init = true;
            }

            break;
        case TUTORFISH_CHECK_ACCOUNT:

            // TODO : check user account for credits

            // ESP_LOGI(TAG, "cheking user account for question credits..");
            // vTaskDelay(2000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_CAPTURE_PIC;
            break;
        case TUTORFISH_CAPTURE_PIC:
            ESP_LOGI(TAG, "capturing picture..");
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_POLL_DB;
            break;
        case TUTORFISH_POLL_DB:
            // handle next state depending on the status field
            ESP_LOGI(TAG, "polling db status");
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_DOWNLOAD_TTS;
            break;
        case TUTORFISH_DOWNLOAD_TTS:
            ESP_LOGI(TAG, "downloading the tts from the db link");
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_PLAYBACK_ANSWER;
            break;
        case TUTORFISH_PLAYBACK_ANSWER:

            repeat_tts_playback = false;

            ESP_LOGI(TAG, "playing the tts audio");
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            ESP_LOGI(TAG, "tap the right stem to repeat this answer, otherwise glasses will sleep");
            vTaskDelay(3000 / portTICK_PERIOD_MS);

            if (!repeat_tts_playback)
            {
                state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
            }

            break;

        case TUTORFISH_SUBMIT_QUESTION_COMPLETE:
            ESP_LOGI(TAG, "TUTORFISH_SUBMIT_QUESTION_COMPLETE cleaning up and setting up wake trigger");
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            ESP_LOGI(TAG, "going to sleep");
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_SUBMIT_QUESTION;

            if (setup_sleep() != ESP_OK)
            {
                ESP_LOGE(TAG, "TUTORFISH_SUBMIT_QUESTION_COMPLETE setup_sleep() err: %s", esp_err_to_name(err));
            }

            break;
        default:
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
