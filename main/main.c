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
#include "websocket.h"

#include "nvs.h"
#include "littlefs_helper.h"
#include "home_button.h"
#include "touch.h"
#include "menu.h"
#include "red_wht_led.h"

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
static bool tutorfish_playback_answer_init = false;

#include "audio_io.h"
bool repeat_tts_playback = false;
float audio_volume;

#include "nvs_data_struct.h"
nvs_data_t nvs_data;

#include "camera.h"
#include "esp_camera.h"
camera_fb_t *pic;

static const uint8_t db_poll_limit = 10;
static uint8_t db_poll_attempts = 0;

static const uint8_t tts_download_limit = 2;
static uint8_t tts_download_attempts = 0;

static const uint8_t validate_session_cookie_get_limit = 4;
static uint8_t validate_session_cookie_get_attempts = 0;

static bool enable_pic12 = false;

size_t http_status = 0;

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

    if (enable_pic12)
    {
        // put the pic12 to sleep
        err = init_red_wht_led();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "init_red_wht_led() err: %s", esp_err_to_name(err));
        }
    }

    // Initialize NVS
    ESP_LOGI(TAG, "nvs_flash_init()");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
    }
    ESP_ERROR_CHECK(err);

    err = write_nvs_email_pass("student@email.com", "!Password1");
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_email_pass() err: %s", esp_err_to_name(err));
    }

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

    // get the audio volume level set by the user
    err = read_nvs_int("volume_lvl", &nvs_data.volume_lvl);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_int() err: %s", esp_err_to_name(err));

        err = write_nvs_int("volume_lvl", 10);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "write_nvs_int(attempt_pic) err: %s", esp_err_to_name(err));
        }

        err = read_nvs_int("volume_lvl", &nvs_data.volume_lvl);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "read_nvs_int(attempt_pic) read_nvs_int(attempt_pic) err: %s", esp_err_to_name(err));
        }
    }

    // convert to float
    audio_volume = nvs_data.volume_lvl * 0.01f;
    ESP_LOGI(TAG, "audio_volume: %f", audio_volume);

    /*
    err = erase_nvs_key("jpg_exponent");
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "erase_nvs_key() err: %s", esp_err_to_name(err));
    }
    */

    err = read_nvs_int("jpg_exponent", &nvs_data.jpeg_quality_exponent);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_int(jpg_exponent) err: %s", esp_err_to_name(err));

        err = write_nvs_int("jpg_exponent", 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "write_nvs_int(jpg_exponent) err: %s", esp_err_to_name(err));
        }

        err = read_nvs_int("jpg_exponent", &nvs_data.jpeg_quality_exponent);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "read_nvs_int(jpg_exponent) read_nvs_int(jpg_exponent) err: %s", esp_err_to_name(err));
        }
    }

    err = read_nvs_int("attempt_pic", &nvs_data.attempting_pic_capture);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_int() err: %s", esp_err_to_name(err));

        err = write_nvs_int("attempt_pic", 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "write_nvs_int(attempt_pic) err: %s", esp_err_to_name(err));
        }

        err = read_nvs_int("attempt_pic", &nvs_data.attempting_pic_capture);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "read_nvs_int(attempt_pic) read_nvs_int(attempt_pic) err: %s", esp_err_to_name(err));
        }
    }

    ESP_LOGI(TAG, "jpg_exponent: %d", nvs_data.jpeg_quality_exponent);
    ESP_LOGI(TAG, "attempt_pic: %d", nvs_data.attempting_pic_capture);

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
        esp_restart();
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
        esp_restart();
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
        esp_restart();
    }

    err = init_touch();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_touch() err: %s", esp_err_to_name(err));
        esp_restart();
    }

    err = init_blue_led();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "init_blue_led() err: %s", esp_err_to_name(err));
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

            ESP_LOGI(TAG, "tutorfish_submit_question_init: %d", tutorfish_submit_question_init);

            // playback submit a question before connecting to wifi
            if (tutorfish_submit_question_init)
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
                        playback_audio_file(audio_buf.submit_a_question_00_wav_audio_buf, audio_buf.submit_a_question_00_wav_len, audio_volume, false);
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
            // wifi connection retry timed-out
            else
            {
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
                        playback_audio_file(audio_buf.returning_home_wav_audio_buf, audio_buf.returning_home_wav_audio_len, audio_volume, false);
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

                if (audio_buf.welcome_01_wav_audio_buf == NULL)
                {
                    err = malloc_welcome_to_tutor_fish_01();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "malloc_welcome_to_tutor_fish_01() err: %s", esp_err_to_name(err));
                    }

                    if (err == ESP_OK)
                    {
                        playback_audio_file(audio_buf.welcome_01_wav_audio_buf, audio_buf.welcome_01_audio_len, audio_volume, false);
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
                        playback_audio_file(audio_buf.home_instructions_00_wav_audio_buf, audio_buf.home_instructions_00_wav_audio_len, audio_volume, true);
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
                        playback_audio_file(audio_buf.exit_this_app_00_wav_audio_buf, audio_buf.exit_this_app_00_wav_audio_len, audio_volume, true);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "playback_audio_file(exit_this_app_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                        }

                        free_exit_this_app_00();
                    }
                }

                tutorfish_home_init = true;
                tutorfish_submit_question_init = false;
            }

            break;
        case TUTORFISH_SUBMIT_QUESTION:

            if (!tutorfish_submit_question_init && nvs_data.attempting_pic_capture == 0)
            {
                ESP_LOGI(TAG, "TUTORFISH_SUBMIT_QUESTION");

                play_submit_question_instructions();

                tutorfish_submit_question_init = true;

                if (setup_sleep() != ESP_OK)
                {
                    ESP_LOGE(TAG, "TUTORFISH_SUBMIT_QUESTION setup_sleep() err: %s", esp_err_to_name(err));
                }
            }
            // capture the picture
            else
            {
                err = toggle_camera_pwdn(CAMERA_ON);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
                }

                if (audio_buf.taking_a_picture321_02_wav_audio_buf == NULL)
                {
                    err = malloc_taking_a_picture321_02_wav();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "malloc_taking_a_picture321_02_wav() err: %s", esp_err_to_name(err));
                    }

                    if (err == ESP_OK)
                    {
                        err = playback_audio_file(audio_buf.taking_a_picture321_02_wav_audio_buf, audio_buf.taking_a_picture321_02_wav_len, audio_volume, false);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "playback_audio_file(taking_a_picture321_02_wav_audio_buf) err: %s", esp_err_to_name(err));
                        }

                        free_taking_a_picture321_02_wav();
                    }
                }

                bool pic_taken = false;
                uint8_t pic_null_increment = 0;
                uint8_t pic_taken_increment = 0;

                while (true)
                {
                    ESP_LOGI(TAG, "Taking picture...");
                    pic = esp_camera_fb_get();
                    if (pic != NULL)
                    {
                        if (pic->len > 0 && pic_taken_increment++ >= 1)
                        {
                            ESP_LOGI(TAG, "Picture taken! Its size is: %zu bytes", pic->len);
                            pic_taken = true;
                            break;
                        }
                    }
                    else
                    {
                        printf("pic_null_increment: %d\n", pic_null_increment);
                        if (pic_null_increment++ >= 1)
                        {
                            pic_taken = false;
                            break;
                        }
                    }

                    esp_camera_fb_return(pic);
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }

                err = toggle_camera_pwdn(CAMERA_OFF);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
                }

                if (pic_taken)
                {
                    // lower the jpg_quality of the pic to capture image reliably
                    if (nvs_data.jpeg_quality_exponent > 0)
                    {
                        nvs_data.jpeg_quality_exponent -= 1;
                        ESP_LOGI(TAG, "pic_taken nvs_data.jpeg_quality_exponent: %d", nvs_data.jpeg_quality_exponent);

                        // decrease jpg_quality inside nvs
                        err = write_nvs_int("jpg_exponent", nvs_data.jpeg_quality_exponent);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "write_nvs_int(jpeg_quality_exponent) err: %s", esp_err_to_name(err));
                        }

                        // TODO : deinit/init camera?
                    }

                    // write the state to nvs
                    err = write_nvs_int("attempt_pic", 0);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "write_nvs_int(attempting_pic_capture) err: %s", esp_err_to_name(err));
                    }

                    // NOTE : the improved jpg quality wont take effect until the user restarts the esp32

                    state_machine = CONNECT_TO_WIFI;
                    break;
                }
                else
                {
                    // pic error, usually happens when lighting conditions are poor
                    // lowering the camera_config jpg_quality value fixes the issue

                    // playback error message
                    if (audio_buf.error_message_00_wav_audio_buf == NULL)
                    {
                        err = malloc_error_message_00_wav();
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                        }

                        if (err == ESP_OK)
                        {
                            err = playback_audio_file(audio_buf.error_message_00_wav_audio_buf, audio_buf.error_message_00_wav_audio_len, audio_volume, false);
                            if (err != ESP_OK)
                            {
                                ESP_LOGE(TAG, "playback_audio_file(error_message_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                            }

                            free_error_message_00_wav();
                        }
                    }

                    if (nvs_data.jpeg_quality_exponent < 5)
                    {
                        nvs_data.jpeg_quality_exponent += 1;
                        ESP_LOGI(TAG, "!pic_taken nvs_data.jpeg_quality_exponent: %d", nvs_data.jpeg_quality_exponent);

                        // increase jpg_quality inside nvs
                        err = write_nvs_int("jpg_exponent", nvs_data.jpeg_quality_exponent);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "write_nvs_int(jpg_exponent) err: %s", esp_err_to_name(err));
                        }
                    }
                    else if (nvs_data.jpeg_quality_exponent == 5)
                    {
                        ESP_LOGE(TAG, "User needs better lighting conditions");
                    }

                    // write the state to nvs
                    err = write_nvs_int("attempt_pic", 1);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "write_nvs_int(attempt_pic) err: %s", esp_err_to_name(err));
                    }

                    // deinit the camera then reinit to let the new jpg_quality settings take effect
                    err = esp_camera_deinit();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "esp_camera_deinit() err: %s", esp_err_to_name(err));
                    }

                    err = init_camera();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "init_camera() err: %s", esp_err_to_name(err));
                    }

                    // go to sleep and remain in the picture capture state
                    if (setup_sleep() != ESP_OK)
                    {
                        ESP_LOGE(TAG, "TUTORFISH_SUBMIT_QUESTION_COMPLETE setup_sleep() err: %s", esp_err_to_name(err));
                    }
                    break;
                }
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
            if (validate_session_cookie_get_attempts++ <= validate_session_cookie_get_limit)
            {
                ESP_LOGI(TAG, "Checking session validity...");
                http_status = http_get_request("tutorfish-env.eba-tdamw63n.us-east-1.elasticbeanstalk.com", "/validate-session", NULL, true);
                // Ok: session cookie valid
                if (http_status == 200)
                {
                    validate_session_cookie_get_attempts = 0;
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

                    print_nvs_credentials();

                    break;
                }
                // Forbidden: useragent was not SmartGlassesOS
                else if (http_status == 403)
                {
                    // notify the user to return to Smart Glasses Home
                    ESP_LOGE(TAG, "TUTORFISH_VALIDATE_SESSION http_get_request() http_status: %d", http_status);
                    validate_session_cookie_get_attempts = 0;
                    state_machine = TUTORFISH_HOME;
                    break;
                }
                else
                {
                    ESP_LOGE(TAG, "TUTORFISH_VALIDATE_SESSION http_get_request() http_status: %d", http_status);
                    // TODO : sometimes http_get_request returns with  ESP_ERR_HTTP_FETCH_HEADER -> E (26495) main.c: TUTORFISH_VALIDATE_SESSION http_get_request() http_status: -1
                    // try to validate again...
                    break;
                }
            }
            else
            {
                validate_session_cookie_get_attempts = 0;
                state_machine = TUTORFISH_HOME;
            }
            break;
        case TUTORFISH_CAPTURE_PIC: //; // ; fixes random err

            if (audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf == NULL)
            {
                err = malloc_uploading_the_picture_please_wait_00_wav();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "malloc_uploading_the_picture_please_wait_00_wav() err: %s", esp_err_to_name(err));
                }

                if (err == ESP_OK)
                {
                    err = playback_audio_file(audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf, audio_buf.uploading_the_picture_please_wait_00_wav_len, audio_volume, false);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "playback_audio_file(uploading_the_picture_please_wait_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                    }

                    free_uploading_the_picture_please_wait_00_wav();
                }
            }

            // send pic buffer to server via http
            const int ret_code = https_send_pic(true, pic);

            if (ret_code == -1)
            {
                // http req missing cookie
                state_machine = TUTORFISH_VALIDATE_SESSION;
                break;
            }
            else if (ret_code != 200)
            {
                // playback error message
                if (audio_buf.error_message_00_wav_audio_buf == NULL)
                {
                    err = malloc_error_message_00_wav();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                    }

                    if (err == ESP_OK)
                    {
                        err = playback_audio_file(audio_buf.error_message_00_wav_audio_buf, audio_buf.error_message_00_wav_audio_len, audio_volume, false);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "playback_audio_file(error_message_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                        }

                        free_error_message_00_wav();
                    }
                }

                ESP_LOGE(TAG, "https_send_pic() ret_code: %d", ret_code);

                state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                break;
            }

            state_machine = TUTORFISH_POLL_DB;
            break;
        case TUTORFISH_POLL_DB:

            // TODO : the sum of poll limit * vTaskDelay time should be equal to the question expiration time
            // TODO : exponentially decrease the vTaskDelay time  until its sum is equal to the expiration time + a bit of time to get expired status

            printf("db_poll_attempts: %d\n", db_poll_attempts);

            if (db_poll_attempts++ <= db_poll_limit)
            {
                // should never happen but just in case
                if (nvs_data.documentId == NULL || nvs_data.documentId_len <= 0)
                {
                    // playback error message
                    if (audio_buf.error_message_00_wav_audio_buf == NULL)
                    {
                        err = malloc_error_message_00_wav();
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                        }

                        if (err == ESP_OK)
                        {
                            err = playback_audio_file(audio_buf.error_message_00_wav_audio_buf, audio_buf.error_message_00_wav_audio_len, audio_volume, false);
                            if (err != ESP_OK)
                            {
                                ESP_LOGE(TAG, "playback_audio_file(error_message_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                            }

                            free_error_message_00_wav();
                        }
                    }

                    db_poll_attempts = 0;
                    state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                    break;
                }

                http_status = http_get_request("tutorfish-env.eba-tdamw63n.us-east-1.elasticbeanstalk.com", "/student-question-status", "documentId", true);
                if (http_status == 200)
                {
                    // memset(nvs_data.question_status, 0, 255);
                    // const char unanswered[10] = "unanswered";
                    // strncpy(nvs_data.question_status,unanswered, 10);

                    printf("nvs_data.question_status: %s\n", nvs_data.question_status);

                    // check for empty response due to http_request.c local_response_buffer bug
                    if (strcmp(nvs_data.question_status, "") == 0)
                    {
                        ESP_LOGE(TAG, "nvs_data.question_status is empty");
                        break;
                    }
                    else if (strcmp(nvs_data.question_status, "issue") == 0)
                    {
                        db_poll_attempts = 0;
                        state_machine = TUTORFISH_PIC_ISSUE;
                        break;
                    }
                    else if (strcmp(nvs_data.question_status, "unanswered") == 0 || strcmp(nvs_data.question_status, "pending") == 0)
                    {
                        // playback "unanswered" audio
                        if (audio_buf.tutors_look_for_answer_00_wav_audio_buf == NULL)
                        {
                            err = malloc_tutors_look_for_answer_00_wav();
                            if (err != ESP_OK)
                            {
                                ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                            }

                            if (err == ESP_OK)
                            {
                                err = playback_audio_file(audio_buf.tutors_look_for_answer_00_wav_audio_buf, audio_buf.tutors_look_for_answer_00_wav_len, audio_volume, false);
                                if (err != ESP_OK)
                                {
                                    ESP_LOGE(TAG, "playback_audio_file(tutors_look_for_answer_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                                }

                                free_tutors_look_for_answer_00_wav();
                            }
                        }
                    }
                    else if (strcmp(nvs_data.question_status, "expired") == 0)
                    {
                        // playback "expired" audio
                        db_poll_attempts = 0;
                        state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                        break;
                    }
                    else if (strcmp(nvs_data.question_status, "canceled") == 0)
                    {
                        // playback "canceled" audio
                        db_poll_attempts = 0;
                        state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                        break;
                    }
                    else
                    {
                        // NOTE:
                        // server does not return "answered" instead the server
                        // returns the tts download link
                        // check TutorFish website source, "student_question_status.js" ~line 49 for more details

                        // playback audio
                        if (audio_buf.tutors_found_answer_00_wav_audio_buf == NULL)
                        {
                            err = malloc_tutors_found_answer_00_wav();
                            if (err != ESP_OK)
                            {
                                ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                            }

                            if (err == ESP_OK)
                            {
                                err = playback_audio_file(audio_buf.tutors_found_answer_00_wav_audio_buf, audio_buf.tutors_found_answer_00_wav_len, audio_volume, false);
                                if (err != ESP_OK)
                                {
                                    ESP_LOGE(TAG, "playback_audio_file(tutors_found_answer_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                                }

                                free_tutors_found_answer_00_wav();
                            }
                        }

                        memset(nvs_data.question_ttsKey, 0, 255);
                        strcpy(nvs_data.question_ttsKey, nvs_data.question_status);

                        db_poll_attempts = 0;
                        state_machine = TUTORFISH_DOWNLOAD_TTS;
                        break;
                    }
                }
                else
                {
                    // http error
                    // if the http_status == 401; the users session token is invalid
                    if (http_status == 401)
                    {
                        state_machine = TUTORFISH_VALIDATE_SESSION;
                        break;
                    }
                }

                // wait until the question status has changed
                vTaskDelay(30000 / portTICK_PERIOD_MS);
            }
            else
            {
                // playback error message
                if (audio_buf.error_message_00_wav_audio_buf == NULL)
                {
                    err = malloc_error_message_00_wav();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                    }

                    if (err == ESP_OK)
                    {
                        err = playback_audio_file(audio_buf.error_message_00_wav_audio_buf, audio_buf.error_message_00_wav_audio_len, audio_volume, false);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "playback_audio_file(error_message_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                        }

                        free_error_message_00_wav();
                    }
                }

                // db_poll_attempts has reach its limit
                db_poll_attempts = 0;
                state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                break;
            }

            break;
        case TUTORFISH_PIC_ISSUE:
            ESP_LOGI(TAG, "there was an issue with your picture, please try again");
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
            break;
        case TUTORFISH_DOWNLOAD_TTS:

            ESP_LOGI(TAG, "downloading the tts from the db link. attempt #%d", tts_download_attempts);

            if (tts_download_attempts++ <= tts_download_limit)
            {
                // should never happen but just in case
                if (strcmp(nvs_data.question_ttsKey, "") == 0)
                {
                    // playback error message
                    if (audio_buf.error_message_00_wav_audio_buf == NULL)
                    {
                        err = malloc_error_message_00_wav();
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
                        }

                        if (err == ESP_OK)
                        {
                            err = playback_audio_file(audio_buf.error_message_00_wav_audio_buf, audio_buf.error_message_00_wav_audio_len, audio_volume, false);
                            if (err != ESP_OK)
                            {
                                ESP_LOGE(TAG, "playback_audio_file(error_message_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                            }

                            free_error_message_00_wav();
                        }
                    }

                    tts_download_attempts = 0;
                    state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                    break;
                }

                http_status = http_download_file("tutorfish-env.eba-tdamw63n.us-east-1.elasticbeanstalk.com", "/student-download-tts", "ttsKey", true);
                if (http_status == ESP_OK)
                {
                    if (audio_buf.tts_audio_len > 0)
                    {
                        tts_download_attempts = 0;
                        state_machine = TUTORFISH_PLAYBACK_ANSWER;
                        break;
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "http_download_file() http_status: %d", http_status);
                }
            }

            break;
        case TUTORFISH_PLAYBACK_ANSWER:

            // disable the wifi
            if (!tutorfish_playback_answer_init)
            {
                if (blue_led_task_init)
                {
                    delete_blue_led_task();
                    blue_led_task_init = false;
                }

                set_blue_led(0);

                if (wifi_bt_status.wifi_conn)
                {
                    err = stop_wifi();
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "stop_wifi() error: %s", esp_err_to_name(err));
                    }
                    else
                    {
                        wifi_bt_status.wifi_conn = false;
                    }
                }

                tutorfish_playback_answer_init = true;
            }

            repeat_tts_playback = false;

            // playback downloaded TTS audio
            if (audio_buf.tts_audio_buf != NULL)
            {
                err = playback_audio_file(audio_buf.tts_audio_buf, audio_buf.tts_audio_len, audio_volume, false);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "playback_audio_file(tts_audio_buf) err: %s", esp_err_to_name(err));
                }
            }

            // playback repeat TTS audio
            if (audio_buf.to_hear_the_answer_again_00_wav_audio_buf == NULL)
            {
                err = malloc_to_hear_the_answer_again_00_wav();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "malloc_to_hear_the_answer_again_00_wav() err: %s", esp_err_to_name(err));
                }

                if (err == ESP_OK)
                {
                    err = playback_audio_file(audio_buf.to_hear_the_answer_again_00_wav_audio_buf, audio_buf.to_hear_the_answer_again_00_wav_len, audio_volume, true);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "playback_audio_file(to_hear_the_answer_again_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                    }

                    free_to_hear_the_answer_again_00_wav();
                }
            }

            ESP_LOGI(TAG, "tap the right stem to repeat this answer, otherwise glasses will sleep");

            // give the user 5 seconds after the message to decide
            vTaskDelay(5000 / portTICK_PERIOD_MS);

            // check touch.c for repeat_tts_playback
            if (!repeat_tts_playback)
            {
                free(audio_buf.tts_audio_buf);
                audio_buf.tts_audio_buf = NULL;

                tutorfish_playback_answer_init = false;

                state_machine = TUTORFISH_SUBMIT_QUESTION_COMPLETE;
                break;
            }

            break;
        case TUTORFISH_SUBMIT_QUESTION_COMPLETE:
            ESP_LOGI(TAG, "TUTORFISH_SUBMIT_QUESTION_COMPLETE cleaning up and setting up wake trigger");

            // free up documentId for new allocation in the future
            if (nvs_data.documentId != NULL)
            {
                free(nvs_data.documentId);
                nvs_data.documentId = NULL;
            }

            state_machine = TUTORFISH_SUBMIT_QUESTION;

            if (blue_led_task_init)
            {
                delete_blue_led_task();
                blue_led_task_init = false;
            }

            set_blue_led(0);

            if (wifi_bt_status.wifi_conn)
            {
                err = stop_wifi();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "stop_wifi() error: %s", esp_err_to_name(err));
                }
                else
                {
                    wifi_bt_status.wifi_conn = false;
                }
            }

            vTaskDelay(100 / portTICK_PERIOD_MS);

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
