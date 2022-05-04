/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"

#include "camera.h"
#include "esp_camera.h"
#include "audio_io.h"

#define NO_DATA_TIMEOUT_SEC 30

static const char *TAG = "WEBSOCKET";

static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;

// ws://echo.websocket.org

static void shutdown_signaler(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
static void get_string(char *line, size_t size)
{
    int count = 0;
    while (count < size)
    {
        int c = fgetc(stdin);
        if (c == '\n')
        {
            line[count] = '\0';
            break;
        }
        else if (c > 0 && c < 127)
        {
            line[count] = c;
            ++count;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

#endif /* CONFIG_WEBSOCKET_URI_FROM_STDIN */

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x08 && data->data_len == 2)
        {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        }
        else
        {
            ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        }
        ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

bool websocket_app_start(camera_fb_t *pic)
{
    esp_websocket_client_config_t websocket_cfg = {};

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS, pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
    char line[128];

    ESP_LOGI(TAG, "Please enter uri of websocket endpoint");
    get_string(line, sizeof(line));

    websocket_cfg.uri = line;
    ESP_LOGI(TAG, "Endpoint uri: %s\n", line);

#else
    websocket_cfg.uri = CONFIG_WEBSOCKET_URI;

#endif /* CONFIG_WEBSOCKET_URI_FROM_STDIN */

    /*
    const char *json_obj_beginning = "{\"client_type\":\"STUDENT\",\"session_cookie\":\"";
    const char *session_cookie = "session=123";

    const char *json_obj_mid_0 = "\",\"user_email\":\"";
    const char *user_email = "student@email.com";

    const char *json_obj_mid_1 = "\",\"data\":\"";
    char *data = "0x00, 0x00, 0x00";

    const char *json_obj_ending = "\"}";
    char *post_data = malloc(strlen(json_obj_beginning) + strlen(session_cookie) + strlen(json_obj_mid_0) + strlen(user_email) + strlen(json_obj_mid_1) + strlen(data) + strlen(json_obj_ending));

    strcpy(post_data, json_obj_beginning);
    strcat(post_data, session_cookie);
    strcat(post_data, json_obj_mid_0);
    strcat(post_data, user_email);
    strcat(post_data, json_obj_mid_1);
    strcat(post_data, data);
    strcat(post_data, json_obj_ending);
    */

    /*
    const char *json_obj_beginning = "{\"client_type\":\"STUDENT\",\"session_cookie\":\"";
    const char *session_cookie = "session=123";

    const char *json_obj_mid_0 = "\",\"user_email\":\"";
    const char *user_email = "student@email.com";

    const char *json_obj_mid_1 = "\",\"data\":\"";
    char *data;

    const char *json_obj_ending = "\"}";
    */

    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(client);
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);

    /*
    esp_err_t err = toggle_camera_pwdn(CAMERA_ON);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
    }

    // give camera time to warm up
    //vTaskDelay(500 / portTICK_PERIOD_MS);

    if (audio_buf.taking_a_picture321_02_wav_audio_buf == NULL)
    {
        err = malloc_taking_a_picture321_02_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_taking_a_picture321_02_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            err = playback_audio_file(audio_buf.taking_a_picture321_02_wav_audio_buf, audio_buf.taking_a_picture321_02_wav_len, 0.2f, false);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(taking_a_picture321_02_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_taking_a_picture321_02_wav();
        }
    }

    bool pic_sent = false;

    int pic_sent_increment = 0;
    int pic_null_increment = 0;
    int pic_taken_increment = 0;
    while (pic_sent_increment < 1)
    {
        if (esp_websocket_client_is_connected(client))
        {
            ESP_LOGI(TAG, "Taking picture...");
            camera_fb_t *pic = esp_camera_fb_get();
            if (pic != NULL)
            {
                if (pic->len > 0 && pic_taken_increment++ >= 1)
                {
                    if (audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf == NULL)
                    {
                        err = malloc_uploading_the_picture_please_wait_00_wav();
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "malloc_uploading_the_picture_please_wait_00_wav() err: %s", esp_err_to_name(err));
                        }

                        if (err == ESP_OK)
                        {
                            err = playback_audio_file(audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf, audio_buf.uploading_the_picture_please_wait_00_wav_len, 0.2f, false);
                            if (err != ESP_OK)
                            {
                                ESP_LOGE(TAG, "playback_audio_file(uploading_the_picture_please_wait_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                            }

                            free_uploading_the_picture_please_wait_00_wav();
                        }
                    }

                    ESP_LOGI(TAG, "Picture sending! Its size is: %zu bytes", pic->len);
                    if (esp_websocket_client_send(client, &pic->buf, pic->len, portMAX_DELAY) > -1)
                    {
                        pic_sent = true;
                        pic_sent_increment++;
                    }


                    // const int post_data_len = strlen(json_obj_beginning) + strlen(session_cookie) + strlen(json_obj_mid_0) + strlen(user_email) + strlen(json_obj_mid_1) + pic->len + strlen(json_obj_ending);
                    // char *post_data = malloc(post_data_len);

                    // strcpy(post_data, json_obj_beginning);
                    // strcat(post_data, session_cookie);
                    // strcat(post_data, json_obj_mid_0);
                    // strcat(post_data, user_email);
                    // strcat(post_data, json_obj_mid_1);
                    // strcat(post_data, &pic->buf);
                    // strcat(post_data, json_obj_ending);

                    // //esp_websocket_client_send(client, post_data, post_data_len, portMAX_DELAY);

                    // esp_camera_fb_return(pic);
                    // free(post_data);
                    // post_data = NULL;
                }
            }
            else
            {
                printf("pic_null_increment: %d\n", pic_null_increment);
                if (pic_null_increment++ >= 1)
                {
                    pic_sent = false;
                    break;
                }
            }

            esp_camera_fb_return(pic);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    err = toggle_camera_pwdn(CAMERA_OFF);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
    }
    */

    if (audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf == NULL)
    {
        esp_err_t err = malloc_uploading_the_picture_please_wait_00_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_uploading_the_picture_please_wait_00_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            err = playback_audio_file(audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf, audio_buf.uploading_the_picture_please_wait_00_wav_len, 0.2f, false);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(uploading_the_picture_please_wait_00_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_uploading_the_picture_please_wait_00_wav();
        }
    }

    bool pic_sent = false;

    if (esp_websocket_client_is_connected(client))
    {
        if (esp_websocket_client_send(client, &pic->buf, pic->len, portMAX_DELAY) > -1)
        {
            pic_sent = true;
        }
    }

    if (pic != NULL)
    {
        esp_camera_fb_return(pic);
    }

    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    esp_websocket_client_close(client, portMAX_DELAY);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(client);

    return pic_sent;
}
