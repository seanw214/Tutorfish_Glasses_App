#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "http_request.h"

#include "nvs.h"
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
bool tutorfish_submit_question_init;
bool tutorfish_settings_init = false;

#include "audio_io.h"
bool repeat_tts_playback = false;

#include "nvs_data_struct.h"
nvs_data_t nvs_data;

static const char *TAG = "main.c";

esp_err_t get_new_session_cookie(void)
{
    const char *json_obj_beginning = "{\"userEmail\":\"";
    const char *json_obj_mid = "\",\"userPassword\":\"";
    const char *json_obj_ending = "\"}";
    char *post_data = malloc(strlen(json_obj_beginning) + strlen(nvs_data.user_email) + strlen(json_obj_mid) + strlen(nvs_data.user_pass) + strlen(json_obj_ending));

    strcpy(post_data, json_obj_beginning);
    strncat(post_data, nvs_data.user_email, nvs_data.user_email_len);
    strcat(post_data, json_obj_mid);
    strncat(post_data, nvs_data.user_pass, nvs_data.user_pass_len);
    strcat(post_data, json_obj_ending);

    // retreive session cookie for account access
    size_t http_status = http_post_request("tutorfish-env.eba-tdamw63n.us-east-1.elasticbeanstalk.com", "/session-smartglasses-login", post_data, NULL);
    if (http_status == 600)
    {
        // TODO : handle esp error
        ESP_LOGE(TAG, "http_post_request() ESP error");
        return ESP_FAIL;
    }

    free(post_data);
    post_data = NULL;

    return ESP_OK;
}

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

    err = read_nvs_email_pass();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_email_pass() err: %s", esp_err_to_name(err));
    }

    err = read_nvs_session_cookie();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_session_cookie() err: %s", esp_err_to_name(err));
    }

    print_nvs_credentials();

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

    tutorfish_submit_question_init = false;

    if (audio_buf.welcome_01_wav_audio_buf == NULL)
    {
        err = malloc_welcome_to_tutor_fish_01();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_welcome_to_tutor_fish_01() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.welcome_01_wav_audio_buf, audio_buf.welcome_01_audio_len, 0.2f, false);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(welcome_01_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_welcome_to_tutor_fish_01();
        }
    }

    if (audio_buf.home_instructions_00_wav_audio_buf == NULL)
    {
        err = malloc_home_instructions_00_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_home_instructions_00_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.home_instructions_00_wav_audio_buf, audio_buf.home_instructions_00_wav_audio_len, 0.2f, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(home_instructions_00_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_home_instructions_00();
        }
    }

    if (audio_buf.exit_this_app_00_wav_audio_buf == NULL)
    {
        err = malloc_exit_this_app_00_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_home_instructions_00_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.exit_this_app_00_wav_audio_buf, audio_buf.exit_this_app_00_wav_audio_len, 0.2f, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(exit_this_app_00_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_exit_this_app_00();
        }
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

            // playback submit a question before connecting to wifi
            if (tutorfish_settings_init)
            {
                if (audio_buf.submit_a_question_00_wav_audio_buf == NULL)
                {
                    err = malloc_submit_a_question_00_wav();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "malloc_tutor_fish_settings_00_wav() err: %s", esp_err_to_name(err));
                    }

                    if (err == ESP_OK)
                    {
                        playback_audio_file(audio_buf.submit_a_question_00_wav_audio_buf, audio_buf.submit_a_question_00_wav_len, 0.2f, false);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "playback_audio_file(submit_a_question_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                        }

                        free_submit_a_question_00();
                    }
                }
            }

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

                state_machine = TUTORFISH_VALIDATE_SESSION;
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

                // if the wifi times out before connection while the user is submitting a question
                if (tutorfish_submit_question_init)
                {
                    if (setup_sleep() != ESP_OK)
                    {
                        ESP_LOGE(TAG, "CONNECT_TO_WIFI setup_sleep() err: %s", esp_err_to_name(err));
                    }

                    break;
                }

                if (audio_buf.returning_home_wav_audio_buf == NULL)
                {
                    err = malloc_returning_home_wav();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "malloc_returning_home_wav() err: %s", esp_err_to_name(err));
                    }

                    if (err == ESP_OK)
                    {
                        playback_audio_file(audio_buf.returning_home_wav_audio_buf, audio_buf.returning_home_wav_audio_len, 0.2f, false);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "playback_audio_file(returning_home_wav_audio_buf) err: %s", esp_err_to_name(err));
                        }

                        free_returning_home_wav();
                    }
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
        case TUTORFISH_VALIDATE_SESSION:
            // check if the session is valid
            ESP_LOGI(TAG, "Checking session validity...");
            size_t http_status = http_get_request("tutorfish-env.eba-tdamw63n.us-east-1.elasticbeanstalk.com", "/validate-session", "", nvs_data.session_cookie);
            // Ok: session cookie valid
            if (http_status == 200)
            {
                state_machine = TUTORFISH_CAPTURE_PIC;
                break;
            }
            // Unauthorized/Bad Request: session cookie has expired/missing session_cookie "Cookie" header
            else if (http_status == 401 || http_status == 400)
            {
                ESP_LOGW(TAG, "TUTORFISH_VALIDATE_SESSION http_get_request() http_status: %d", http_status);
                ESP_LOGI(TAG, "Getting new session cookie...");

                err = get_new_session_cookie();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "TUTORFISH_VALIDATE_SESSION get_new_session_cookie() err: %s", esp_err_to_name(err));
                    break;
                }

                break;
            }
            // Forbidden: useragent was not SmartGlassesOS
            else if (http_status == 403)
            {
                // notify the user to return to Smart Glasses Home
                ESP_LOGE(TAG, "TUTORFISH_VALIDATE_SESSION http_get_request() http_status: %d", http_status);
                state_machine = TUTORFISH_HOME;
                break;
            }
            else
            {
                ESP_LOGE(TAG, "TUTORFISH_VALIDATE_SESSION http_get_request() http_status: %d", http_status);
                state_machine = TUTORFISH_HOME;
                break;
            }
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
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_PLAYBACK_ANSWER;
            break;
        case TUTORFISH_PLAYBACK_ANSWER:

            repeat_tts_playback = false;

            ESP_LOGI(TAG, "playing the tts audio");
            vTaskDelay(2000 / portTICK_PERIOD_MS);

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

            if (blue_led_task_init)
            {
                delete_blue_led_task();
                blue_led_task_init = false;
            }

            set_blue_led(0);

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
