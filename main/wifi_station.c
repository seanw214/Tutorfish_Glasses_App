/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_bt_status.h"
//#include "home_button.h"
#include "audio_io.h"
#include "state_machine.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char *ssid = "Sean's iPhone";
const char *pass = "uzx8hnkddbn5t";

#define EXAMPLE_ESP_MAXIMUM_RETRY 10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();

        if (audio_buf.attempt_wifi_conn_00_wav_audio_buf == NULL)
        {
            esp_err_t err = malloc_attempt_wifi_conn_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_attempt_wifi_conn_00_wav() err: %s", esp_err_to_name(err));
            }

            if (err == ESP_OK)
            {
                playback_audio_file(audio_buf.attempt_wifi_conn_00_wav_audio_buf, audio_buf.attempt_wifi_conn_00_wav_audio_len, 0.2f, false);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "playback_audio_file(submit_a_question_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                }

                free_attempt_wifi_conn_00();
            }
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
            wifi_bt_status.wifi_conn = false;
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            s_retry_num = 0;
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    // NOTE : fixes main.c glasses_state not switching
    else if (state_machine == TUTORFISH_HOME)
    {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        s_retry_num = 0;
    }
}

esp_err_t wifi_ps_mode(void)
{
    return esp_wifi_set_ps(WIFI_PS_NONE);
}

esp_err_t stop_wifi(void)
{
    return esp_wifi_stop();
}

esp_err_t start_wifi(void)
{
    // disable_home_button_isr(); // BUG FIX : gpio36 trigger after WiFi start. https://www.esp32.com/viewtopic.php?t=5161

    // NOTE : if brownout disabled, comment out attempt_wifi_conn audio playback inside main.c
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    ESP_ERROR_CHECK(esp_wifi_start());
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); //enable brownout detector

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    s_wifi_event_group = xEventGroupCreate();

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        wifi_bt_status.wifi_conn = true;
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ssid, pass);
        return ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        // home_init = false;
        wifi_bt_status.wifi_conn = false;
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ssid, pass);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);

    return ESP_FAIL;
}

esp_err_t _wifi_deinit(void)
{
    return esp_wifi_deinit();
}

esp_err_t _esp_wifi_init(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    return esp_wifi_init(&cfg);
}

void wifi_init_sta(void)
{
    esp_err_t err;

    // s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    // printf("esp_wifi_init()\n");
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {

            // Setting a password implies station will connect to all security modes including WEP/WPA.
            // However these modes are deprecated and not advisable to be used. Incase your Access point
            // doesn't support WPA2, these mode can be enabled by commenting below line

            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, pass);

    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode() error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode() error: %s", esp_err_to_name(err));
        return err;
    }

    return err;
}
