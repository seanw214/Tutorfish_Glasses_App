/* ESP HTTP Client Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"

#include "driver/i2s.h"
#include "esp_camera.h"
#include "esp_ota.h"
#include "audio_io.h"
#include "nvs_data_struct.h"

#include "camera.h" // <- remove after test camera PWDN toggle before/after using camera
// #include "icm_20948_driver.h" // <- remove after test icm-20948 i2c while capturing camera frames
// #include "red_wht_led.h"      // <- remove after testing

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";

#define HTTP_BOUNDARY "----123"
#define HTTP_BUFFER_SIZE 1024

#define TXRX_TO_PIC12_EN (0)

/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem
   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null
   The CA root cert is the last cert given in the chain of certs.
   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[] asm("_binary_howsmyssl_com_root_cert_pem_end");

extern const char postman_root_cert_pem_start[] asm("_binary_postman_root_cert_pem_start");
extern const char postman_root_cert_pem_end[] asm("_binary_postman_root_cert_pem_end");

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");

static char *img_buf = NULL;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");

        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);

            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");

        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

size_t http_firmware_get_request(char *hostname, char *path, char *query, char *session_cookie)
{
    // char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    char *local_response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    char version[26] = "firmwareVersion=";
    strcat(version, query);

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .query = version,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer, // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        .user_agent = "SmartGlassesOS/1.0.0",
        .buffer_size_tx = 2048 // fix HTTP_HEADER: Buffer length is small to fit all the headers error. https://www.reddit.com/r/esp32/comments/krwajq/esp32_http_post_fails_when_using_a_long_header/
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);

    if (session_cookie != NULL)
    {
        esp_http_client_set_header(client, "Cookie", session_cookie);
    }

    // GET
    esp_err_t err = esp_http_client_perform(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", http_status, esp_http_client_get_content_length(client));

        // the status code from the server that triggers ota
        if (http_status == HttpStatus_Ok)
        {

            char url[100] = "http://nosystems-env.eba-mpgixwp9.us-east-1.elasticbeanstalk.com/firmware-download";
            // strcat(url, local_response_buffer);
            // ESP_LOGI(TAG, "firmware url: %s", url);
            // ESP_LOGI(TAG, "local_response_buffer: %s", local_response_buffer);

            esp_http_client_config_t config = {
                .url = url,
                .cert_pem = (char *)howsmyssl_com_root_cert_pem_start,
                .event_handler = _http_event_handler,
                .keep_alive_enable = true,
                .user_agent = "SmartGlassesOS/1.0.0"};

            esp_err_t ret = esp_https_ota(&config, local_response_buffer);
            if (ret == ESP_OK)
            {
                esp_restart();
            }
            else
            {
                ESP_LOGE(TAG, "Firmware upgrade failed");
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));

        // TODO : handle E (22079) HTTP_CLIENT: HTTP GET request failed: ESP_ERR_HTTP_CONNECT
        // failed due to timeout

        http_status == HttpStatus_Forbidden ? http_status = HttpStatus_Forbidden : http_status == 600;
    }

    printf("GET local_response_buffer:\n");
    for (int i = 0; i <= strlen(local_response_buffer); i++)
    {
        printf("%c", local_response_buffer[i]);
    }
    printf("\n");

    free(local_response_buffer);
    local_response_buffer = NULL;

    esp_http_client_cleanup(client);

    return http_status;
}

esp_err_t http_app_get_request(char *app_name, char *session_cookie)
{
    char url[100] = "http://nosystems-env.eba-mpgixwp9.us-east-1.elasticbeanstalk.com/app-download?appName=";

    // char _query[100] = "?appName=tutorfish";
    strcat(url, app_name);

    ESP_LOGI(TAG, "http_app_get_request(): %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (char *)howsmyssl_com_root_cert_pem_start,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
        .user_agent = "SmartGlassesOS/1.0.0"};

    esp_err_t ret = esp_https_app_ota(&config, app_name);
    if (ret == ESP_OK)
    {
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "App Select Failed");
    }

    return ret;
}

size_t http_get_request(char *hostname, char *path, char *query, char *session_cookie)
{
    // char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    char *local_response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    // // concatenate the http query
    // char version[14] = "firmwareVersion=";
    // strcat(version, query);

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .query = query,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer, // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        .user_agent = "SmartGlassesOS/1.0.0",
        .buffer_size_tx = 2048 // fix HTTP_HEADER: Buffer length is small to fit all the headers error. https://www.reddit.com/r/esp32/comments/krwajq/esp32_http_post_fails_when_using_a_long_header/
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);

    if (session_cookie != NULL)
    {
        esp_http_client_set_header(client, "Cookie", session_cookie);
    }

    // GET
    esp_err_t err = esp_http_client_perform(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", http_status, esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));

        // TODO : handle E (22079) HTTP_CLIENT: HTTP GET request failed: ESP_ERR_HTTP_CONNECT
        // failed due to timeout

        http_status == HttpStatus_Forbidden ? http_status = HttpStatus_Forbidden : http_status == 600;
    }

    printf("GET local_response_buffer:\n");
    for (int i = 0; i <= strlen(local_response_buffer); i++)
    {
        printf("%c", local_response_buffer[i]);
    }
    printf("\n");

    free(local_response_buffer);
    local_response_buffer = NULL;

    esp_http_client_cleanup(client);

    return http_status;
}

size_t http_post_request(char *hostname, char *path, char *post_data, char *session_cookie)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    // char *local_response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    // memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .disable_auto_redirect = true,
        .user_agent = "SmartGlassesOS/1.0.0",
        .buffer_size_tx = 2048 // fix HTTP_HEADER: Buffer length is small to fit all the headers error. https://www.reddit.com/r/esp32/comments/krwajq/esp32_http_post_fails_when_using_a_long_header/
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    if (session_cookie != NULL)
    {
        esp_http_client_set_header(client, "Cookie", session_cookie);
    }

    if (post_data != NULL)
    {
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
    }

    esp_err_t err = esp_http_client_perform(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {
        size_t length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", http_status, length);

        // handle /signup POST data
        if (http_status == HttpStatus_Ok && strcmp(path, "/session-smartglasses-login") == 0)
        {
            // size_t length = esp_http_client_get_content_length(client);

            nvs_handle_t nvs_handle;

            err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
            }

            // nvs read id_token
            err = nvs_set_str(nvs_handle, "session_cookie", local_response_buffer);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "add_blob_to_nvs() nvs_set_blob() err: %s", esp_err_to_name(err));
            }

            nvs_commit(nvs_handle);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "add_blob_to_nvs() nvs_commit() err: %s", esp_err_to_name(err));
            }

            nvs_close(nvs_handle);

            // if the nvs write succeeded, add data to nvs_data struct
            if (err == ESP_OK)
            {
                // during session cookie refresh, check the data of the nvs_data.session_cookie struct
                if (nvs_data.session_cookie == NULL)
                {
                    nvs_data.session_cookie = malloc(length);
                }
                else
                {
                    free(nvs_data.session_cookie);
                    nvs_data.session_cookie = NULL;
                    nvs_data.session_cookie = malloc(length);
                }

                nvs_data.session_cookie_len = length;
                strncpy(nvs_data.session_cookie, local_response_buffer, length);

                printf("/session-smartglasses-login POST 200 nvs_data.session_cookie:\n");
                for (int i = 0; i < nvs_data.session_cookie_len; i++)
                {
                    printf("%c", nvs_data.session_cookie[i]);
                }
                printf("\n");
            }
        }

        // handle POST /signup/signout_glasses data
        if (http_status == HttpStatus_Ok && strcmp(path, "/signup/signout_glasses") == 0)
        {
            printf("/signup/signout_glasses POST 200 local_response_buffer:\n");
            for (int i = 0; i < length; i++)
            {
                printf("%c", local_response_buffer[i]);
            }
            printf("\n");
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));

        // TODO : handle E (22079) HTTP_CLIENT: HTTP GET request failed: ESP_ERR_HTTP_CONNECT
        // failed due to timeout

        // TODO : make sure this ternery works correctly
        http_status == HttpStatus_Unauthorized ? http_status = HttpStatus_Unauthorized : http_status == 600;
    }

    free(local_response_buffer);
    local_response_buffer = NULL;

    esp_http_client_cleanup(client);

    return http_status;
}

static bool sendImage(esp_http_client_handle_t client)
{
    esp_err_t err;

    err = toggle_camera_pwdn(0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
    }

// /*
// *
// *
// *
// *
// *
// sustain wht led
#if TXRX_TO_PIC12_EN
    err = set_tx_level(0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "set_tx_level() err: %s", esp_err_to_name(err));
    }
#endif
    // *
    // *
    // *
    // *
    // *
    // */

    vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();

    // /*
    // *
    // *
    // *
    // *
    // *
    // wake icm-20948 from sleep
    // err = icm_write_PWR_MGMT_1(1);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "icm_write_PWR_MGMT_1() err: %s", esp_err_to_name(err));
    // }

    // // wait until the chip has data ready
    // uint8_t i = 0;
    // uint8_t int_status_1 = 0;
    // do
    // {
    //     int_status_1 = icm_read_INT_STATUS_1();
    //     ESP_LOGI(TAG, "int_status_1: %d", int_status_1);
    //     i++;
    //     vTaskDelay(10 / portTICK_PERIOD_MS);

    // } while (int_status_1 != 1 || i >= 255);
    // *
    // *
    // *
    // *
    // *
    // */

    // capture a few images to warm up the sensor
    uint8_t capture_count = 0;
    while (1)
    {
        esp_camera_fb_return(pic);
        pic = esp_camera_fb_get();

        // /*
        // *
        // *
        // *
        // *
        // read from IMU
        // for (uint8_t i = 0; i <= 20; i++)
        // {
        //     err = icm_read_ACCEL_Y();
        //     if (err != ESP_OK)
        //     {
        //         ESP_LOGE(TAG, "icm_read_ACCEL_Y() err: %s", esp_err_to_name(err));
        //     }
        //     vTaskDelay(5 / portTICK_PERIOD_MS);
        // }
        // *
        // *
        // *
        // *
        // */

        printf("capture_count: %d\n", capture_count);
        if (capture_count++ == 4)
        {
            break;
        }
    }

    // char *img_buf = malloc(pic->len * sizeof(char));
    img_buf = malloc(pic->len * sizeof(char));
    int img_len = pic->len;
    memcpy(img_buf, pic->buf, pic->len);

    esp_camera_fb_return(pic);

    // /*
    // *
    // *
    // *
    // *
    // // put icm-20948 to sleep
    // err = icm_write_PWR_MGMT_1(0x41);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "icm_write_PWR_MGMT_1() err: %s", esp_err_to_name(err));
    // }
// *
// *
// *
// *
// */

// disable all light
#if TXRX_TO_PIC12_EN
    err = set_rx_level(0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "set_tx_level() err: %s", esp_err_to_name(err));
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);

    // sustain wht led briefly
    err = set_rx_level(1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "set_tx_level() err: %s", esp_err_to_name(err));
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);

    // disable sustain wht led

    err = set_tx_level(1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "set_tx_level() err: %s", esp_err_to_name(err));
    }
#endif
    // *
    // *
    // *
    // *
    // *
    // */

    err = toggle_camera_pwdn(1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "toggle_camera_pwdn() err: %s", esp_err_to_name(err));
    }

    // use pic->buf to access the image
    ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", img_len);

    //=============================================================================================

    time_t fileTransferStart = 0;
    time(&fileTransferStart);

    // request header
    char requestHead[200] = "";
    strcat(requestHead, "--");
    strcat(requestHead, HTTP_BOUNDARY);
    strcat(requestHead, "\r\n");
    strcat(requestHead, "Content-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n");
    ESP_LOGI(TAG, "requestHead: %s", requestHead);

    // request tail
    char tail[50] = "";
    strcat(tail, "\r\n--");
    strcat(tail, HTTP_BOUNDARY);
    strcat(tail, "--\r\n");
    ESP_LOGI(TAG, "tail: %s", tail);

    // Set Content-Length
    int contentLength = strlen(requestHead) + img_len + strlen(tail);
    ESP_LOGI(TAG, "length: %d", contentLength);
    char lengthStr[10];
    sprintf(lengthStr, "%i", contentLength);

    err = esp_http_client_set_header(client, "Content-Length", lengthStr);

    //=============================================================================================

    err = esp_http_client_open(client, contentLength);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    ESP_LOGI(TAG, "client connection open");

    ESP_LOGI(TAG, "requestHead:\t%d", esp_http_client_write(client, requestHead, strlen(requestHead)));
    // Send file parts
    char buffer[HTTP_BUFFER_SIZE];
    unsigned fileProgress = 0;
    while (fileProgress < img_len)
    {
        if ((fileProgress + 1024) > img_len)
        {
            uint16_t remaining_bytes = img_len % 1024;

            int write_ret = esp_http_client_write(client, &img_buf[remaining_bytes], remaining_bytes);
            ESP_LOGI(TAG, "write file:\t%d/%d", write_ret, remaining_bytes);
            if (write_ret < 0)
            {
                return false;
            }

            fileProgress += write_ret;

            break;
        }

        int write_ret = esp_http_client_write(client, &img_buf[fileProgress], 1024);
        ESP_LOGI(TAG, "write file:\t%d/%d", write_ret, 1024);
        if (write_ret < 0)
        {
            return false;
        }

        fileProgress += write_ret;
        time_t now;
        time(&now);
        if ((float)(now - fileTransferStart) / 1024 > 0)
            ESP_LOGI(TAG, "%u/%u bytes sent total %.02f KiB/s", fileProgress, img_len, fileProgress / (float)(now - fileTransferStart) / 1024);
    }

    // finish Multipart request
    ESP_LOGI(TAG, "tail:\t%d", esp_http_client_write(client, tail, strlen(tail)));

    // esp_camera_fb_return(pic);
    free(img_buf);
    img_buf = NULL;

    // Get response
    ESP_LOGI(TAG, "fetch_headers:\t%d", esp_http_client_fetch_headers(client));
    ESP_LOGI(TAG, "chunked:\t%d", esp_http_client_is_chunked_response(client));

    int responseLength = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "responseLength:\t%d", responseLength);

    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "status:\t%d", status);

    if (responseLength)
    {
        if (esp_http_client_read(client, buffer, responseLength) == responseLength)
            ESP_LOGI(TAG, "Response: %.*s", responseLength, buffer);
    }

    err = esp_http_client_close(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to close HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    return status == 200 || status == 409;
}

static bool sendFile(esp_http_client_handle_t client)
{
    esp_err_t err;

    static const int max_audio_buffer_allocation = 51200;
    char *temp_audio_buf = malloc(max_audio_buffer_allocation * sizeof(char));
    uint8_t audio_volume = 20;

    int samples_read = 0;

    err = i2s_start(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_start() err %s", esp_err_to_name(err));
    }

    // wait for i2s to intialize (removes pops)
    vTaskDelay(900 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "capture_audio() started");

    while (samples_read < max_audio_buffer_allocation)
    {
        int16_t sample = 0;
        size_t bytes_read;

        i2s_read(I2S_NUM_1, &sample, sizeof(sample), &bytes_read, 10);

        if (bytes_read == 2)
        {
            sample *= audio_volume;

            // uint8_t bit rate conversion from 16 bit
            temp_audio_buf[samples_read + 0] = sample;
            temp_audio_buf[samples_read + 1] = sample >> 8;

            samples_read += bytes_read;
        }
        // vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "capture_audio() finished");

    printf("samples_read: %d\n", samples_read);

    ESP_LOGI(TAG, "i2s_write() starting...");

    size_t bytes_written = 0;
    int samples_written = 0;
    while (samples_written < samples_read)
    {
        err = i2s_write(I2S_NUM_1, &temp_audio_buf[samples_written], sizeof(int8_t), &bytes_written, 10);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "i2s_write() err %s", esp_err_to_name(err));
            break;
        }
        samples_written += bytes_written;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // err = i2s_zero_dma_buffer(I2S_NUM_1);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGI(TAG, "i2s_stop() err %s", esp_err_to_name(err));
    // }

    err = i2s_stop(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_stop() err %s", esp_err_to_name(err));
    }

    time_t fileTransferStart = 0;
    time(&fileTransferStart);

    //=============================================================================================

    // request header
    char requestHead[200] = "";
    strcat(requestHead, "--");
    strcat(requestHead, HTTP_BOUNDARY);
    strcat(requestHead, "\r\n");
    strcat(requestHead, "Content-Disposition: form-data; name=\"wavAudio\"; filename=\"wave.wav\"\r\nContent-Type: audio/l16; bits=16; rate=16000; endianess=little-endian\r\n\r\n");
    ESP_LOGI(TAG, "requestHead: %s", requestHead);

    // request tail
    char tail[50] = "";
    strcat(tail, "\r\n--");
    strcat(tail, HTTP_BOUNDARY);
    strcat(tail, "--\r\n");
    ESP_LOGI(TAG, "tail: %s", tail);

    // Set Content-Length
    int contentLength = strlen(requestHead) + samples_read + strlen(tail);
    ESP_LOGI(TAG, "length: %d", contentLength);
    char lengthStr[10];
    sprintf(lengthStr, "%i", contentLength);

    err = esp_http_client_set_header(client, "Content-Length", lengthStr);

    //=============================================================================================

    err = esp_http_client_open(client, contentLength);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    ESP_LOGI(TAG, "client connection open");

    ESP_LOGI(TAG, "requestHead:\t%d", esp_http_client_write(client, requestHead, strlen(requestHead)));
    // Send file parts
    char buffer[HTTP_BUFFER_SIZE];
    unsigned fileProgress = 0;
    while (fileProgress < samples_read)
    {
        int write_ret = esp_http_client_write(client, &temp_audio_buf[fileProgress], 1024);
        ESP_LOGI(TAG, "write file:\t%d/%d", write_ret, 1024);
        if (write_ret < 0)
        {
            return false;
        }

        fileProgress += write_ret;
        // ESP_LOGI(TAG, "%.*s", length, buffer);
        time_t now;
        time(&now);
        if ((float)(now - fileTransferStart) / 1024 > 0)
            ESP_LOGI(TAG, "%u/%u bytes sent total %.02f KiB/s", fileProgress, samples_read, fileProgress / (float)(now - fileTransferStart) / 1024);
    }

    // finish Multipart request
    ESP_LOGI(TAG, "tail:\t%d", esp_http_client_write(client, tail, strlen(tail)));

    // Get response
    ESP_LOGI(TAG, "fetch_headers:\t%d", esp_http_client_fetch_headers(client));
    ESP_LOGI(TAG, "chunked:\t%d", esp_http_client_is_chunked_response(client));

    int responseLength = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "responseLength:\t%d", responseLength);

    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "status:\t%d", status);

    if (responseLength)
    {
        if (esp_http_client_read(client, buffer, responseLength) == responseLength)
            ESP_LOGI(TAG, "Response: %.*s", responseLength, buffer);
    }

    err = esp_http_client_close(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to close HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    err = i2s_start(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_start() err %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "i2s_write() starting...");

    bytes_written = 0;
    samples_written = 0;
    while (samples_written < samples_read)
    {
        err = i2s_write(I2S_NUM_1, &temp_audio_buf[samples_written], sizeof(int8_t), &bytes_written, 10);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "i2s_write() err %s", esp_err_to_name(err));
            break;
        }
        samples_written += bytes_written;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "i2s_write() samples_written: %d", samples_written);

    err = i2s_stop(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_stop() err %s", esp_err_to_name(err));
    }

    return status == 200 || status == 409;
}

void https_send_file(uint8_t *buf, size_t buf_len)
{
    char url[200] = "http://nosystems-env.eba-mpgixwp9.us-east-1.elasticbeanstalk.com/camera";
    // char url[200] = "http://nosystems-env.eba-mpgixwp9.us-east-1.elasticbeanstalk.com/voice";

    ESP_LOGI(TAG, "url = %s", url);
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .cert_pem = howsmyssl_com_root_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(TAG, "client initialized");

    err = esp_http_client_set_method(client, HTTP_METHOD_POST);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set method: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    // Set Multipart header
    char contentTypeStr[50] = "multipart/form-data; boundary=";
    strcat(contentTypeStr, HTTP_BOUNDARY);

    err = esp_http_client_set_header(client, "Content-Type", contentTypeStr);
    err = esp_http_client_set_header(client, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    err = esp_http_client_set_header(client, "Accept-Encoding", "gzip,deflate");
    err = esp_http_client_set_header(client, "Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7");
    err = esp_http_client_set_header(client, "User-Agent", "Test");
    err = esp_http_client_set_header(client, "Keep-Alive", "300");
    err = esp_http_client_set_header(client, "Connection", "keep-alive");
    err = esp_http_client_set_header(client, "Accept-Language", "en-us");

    // sendFile(client);
    sendImage(client);

    err = esp_http_client_cleanup(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to clean HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    // vTaskDelete(NULL);
}

/*
static void http_test_task(void *pvParameters)
{
    http_rest_with_url();
    http_rest_with_hostname_path();
#if CONFIG_ESP_HTTP_CLIENT_ENABLE_BASIC_AUTH
    http_auth_basic();
    http_auth_basic_redirect();
#endif
#if CONFIG_ESP_HTTP_CLIENT_ENABLE_DIGEST_AUTH
    http_auth_digest();
#endif
    http_relative_redirect();
    http_absolute_redirect();
    https_with_url();
    https_with_hostname_path();
    http_redirect_to_https();
    http_download_chunk();
    http_perform_as_stream_reader();
    https_async();
    https_with_invalid_url();
    http_native_request();
    http_partial_download();

    ESP_LOGI(TAG, "Finish http example");
    vTaskDelete(NULL);
}
*/