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
//#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"

#include "driver/i2s.h"
#include "esp_camera.h"
#include "esp_ota.h"
#include "nvs_data_struct.h"
#include "audio_io.h"

#include "camera.h" // <- remove after test camera PWDN toggle before/after using camera

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
static char *session_buf = NULL;
static char *response_buffer = NULL;

char local_response_buffer_[MAX_HTTP_OUTPUT_BUFFER] = {0};

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

int http_download_file(char *hostname, char *path, char *query, bool cookie)
{
    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .event_handler = _http_event_handler,
        .disable_auto_redirect = true,
        .user_agent = "SmartGlassesOS/1.0.0",
        .buffer_size_tx = 2048 // fix HTTP_HEADER: Buffer length is small to fit all the headers error. https://www.reddit.com/r/esp32/comments/krwajq/esp32_http_post_fails_when_using_a_long_header/
    };

    // check if the GET request has a query
    char query_[1024] = "";
    if (cookie)
    {
        strncat(query_, nvs_data.session_cookie, nvs_data.session_cookie_len);
        config.query = query_;
    }

    if (query != NULL)
    {
        if (cookie)
        {
            const char *amperstand = "&";
            strcat(query_, amperstand);
        }

        if (strcmp(query, "ttsKey") == 0)
        {
            const char *ttsKey = "ttsKey=";
            strcat(query_, ttsKey);
            strcat(query_, nvs_data.question_ttsKey);
        }
        config.query = query_;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET Request
    esp_err_t err = esp_http_client_set_method(client, HTTP_METHOD_GET);

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));

        err = esp_http_client_cleanup(client);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_http_client_cleanup() err: %s", esp_err_to_name(err));
            return err;
        }

        return ESP_FAIL;
    }

    // GET
    err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        err = esp_http_client_cleanup(client);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_http_client_cleanup() err: %s", esp_err_to_name(err));
            return err;
        }

        return ESP_FAIL;
    }

    int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "esp_http_client_get_content_length(): %d", content_length);

    if (content_length < 0)
    {
        ESP_LOGE(TAG, "esp_http_client_get_content_length is 0");
    }
    else
    {
        // check the path of the request and handle accordingly
        if (strcmp(path, "/student-download-tts") == 0)
        {
            // allocate audio buffer
            // NOTE : added 10000 additional bytes to prevent early playback stop
            audio_buf.tts_audio_len = content_length;
            if (audio_buf.tts_audio_buf == NULL)
            {
                audio_buf.tts_audio_buf = malloc(audio_buf.tts_audio_len);
            }
            else
            {
                free(audio_buf.tts_audio_buf);
                audio_buf.tts_audio_buf = NULL;
                audio_buf.tts_audio_buf = malloc(audio_buf.tts_audio_len);
            }

            int data_read = esp_http_client_read_response(client, audio_buf.tts_audio_buf, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read < 0)
            {
                ESP_LOGE(TAG, "Failed to read response");
                free(audio_buf.tts_audio_buf);
                audio_buf.tts_audio_buf = NULL;
            }
            /*
            if (data_read >= 0)
            {
                //ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
                //ESP_LOG_BUFFER_HEX(TAG, audio_buf.tts_audio_buf, 8);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to read response");
                free(audio_buf.tts_audio_buf);
                audio_buf.tts_audio_buf = NULL;
            }
            */
        }
    }

    int http_status = esp_http_client_get_status_code(client);

    err = esp_http_client_close(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_close() err: %s", esp_err_to_name(err));
    }

    err = esp_http_client_cleanup(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_cleanup() err: %s", esp_err_to_name(err));
        return err;
    }

    return err == ESP_OK && http_status == 200 ? err = ESP_OK : http_status;
}

/*
size_t http_get_request(char *hostname, char *path, char *query, bool cookie)
{

    // char *local_response_buffer_ = malloc(MAX_HTTP_OUTPUT_BUFFER);
    // memset(local_response_buffer_, 0, MAX_HTTP_OUTPUT_BUFFER);

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer_, // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        .user_agent = "SmartGlassesOS/1.0.0",
        .buffer_size_tx = 2048 // fix HTTP_HEADER: Buffer length is small to fit all the headers error. https://www.reddit.com/r/esp32/comments/krwajq/esp32_http_post_fails_when_using_a_long_header/
    };

    // check if the GET request has a query
    char query_[1024] = "";
    if (cookie)
    {
        strncat(query_, nvs_data.session_cookie, nvs_data.session_cookie_len);
        config.query = query_;
    }

    if (query != NULL)
    {
        if (strcmp(query, "documentId") == 0)
        {
            const char *documentId = "&documentId=";
            // const char *documentId = "&documentId=ov8mx2F3UNAlYtV5JRon";
            strcat(query_, documentId);
            strncat(query_, nvs_data.documentId, nvs_data.documentId_len);
        }

        config.query = query_;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err_ = esp_http_client_set_method(client, HTTP_METHOD_GET);
    if (err_ != ESP_OK)
    {
        ESP_LOGE(TAG, "http_get_request() esp_http_client_set_method() err_ : %s", esp_err_to_name(err_));
    }

    // GET
    esp_err_t err = esp_http_client_perform(client);
    size_t length = esp_http_client_get_content_length(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", http_status, length);

        // if the http request was successful, add new session cookie to nvs
        if (http_status == HttpStatus_Ok && strcmp(path, "/student-question-status") == 0)
        {
            ESP_LOGI(TAG, "local_response_buffer_: %s", local_response_buffer_);

            // BUG FIX: local_reponse_buffer retains previous request data
            if (strstr(local_response_buffer_, "unanswered"))
            {
                length = strlen("unanswered");
            }
            else if (strstr(local_response_buffer_, "pending"))
            {
                length = strlen("pending");
            }
            else if (strstr(local_response_buffer_, "answered"))
            {
                length = strlen("answered");
            }
            else if (strstr(local_response_buffer_, "issue"))
            {
                length = strlen("issue");
            }
            else if (strstr(local_response_buffer_, "expired"))
            {
                length = strlen("expired");
            }
            else if (strstr(local_response_buffer_, "canceled"))
            {
                length = strlen("canceled");
            }

            printf("length: %d\n", length);

            nvs_data.question_status_len = length;
            if (nvs_data.question_status == NULL)
            {
                nvs_data.question_status = malloc(nvs_data.question_status_len);
            }
            else
            {
                free(nvs_data.question_status);
                nvs_data.question_status = NULL;
                nvs_data.question_status = malloc(nvs_data.question_status_len);

            }
            memset(nvs_data.question_status, 0, nvs_data.question_status_len);
            strncpy(nvs_data.question_status, local_response_buffer_, nvs_data.question_status_len);
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));

        // TODO : handle E (22079) HTTP_CLIENT: HTTP GET request failed: ESP_ERR_HTTP_CONNECT
        // failed due to timeout

        http_status == HttpStatus_Forbidden ? http_status = HttpStatus_Forbidden : http_status == 600;
    }

    free(session_buf);
    session_buf = NULL;

    esp_http_client_cleanup(client);

    return http_status;
}
*/

size_t http_get_request(char *hostname, char *path, char *query, bool cookie)
{
    response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .event_handler = _http_event_handler,
        .user_data = response_buffer, // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        .user_agent = "SmartGlassesOS/1.0.0",
        .buffer_size_tx = 2048, // fix HTTP_HEADER: Buffer length is small to fit all the headers error. https://www.reddit.com/r/esp32/comments/krwajq/esp32_http_post_fails_when_using_a_long_header/
        .buffer_size = 2048
    };

    // check if the GET request has a query
    char query_[1024] = "";
    if (cookie)
    {
        strncat(query_, nvs_data.session_cookie, nvs_data.session_cookie_len);
        config.query = query_;
    }

    if (query != NULL)
    {
        if (cookie)
        {
            const char *amperstand = "&";
            strcat(query_, amperstand);
        }

        if (strcmp(query, "documentId") == 0)
        {
            const char *documentId = "documentId=";
            // const char *documentId = "documentId=w2EKdfqufgjNO0IU8SZS"; // NOTE: remove when NOT testing
            strcat(query_, documentId);
            strncat(query_, nvs_data.documentId, nvs_data.documentId_len);
        }

        config.query = query_;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err_ = esp_http_client_set_method(client, HTTP_METHOD_GET);
    if (err_ != ESP_OK)
    {
        ESP_LOGE(TAG, "http_get_request() esp_http_client_set_method() err_ : %s", esp_err_to_name(err_));
    }

    // GET
    esp_err_t err = esp_http_client_perform(client);
    size_t length = esp_http_client_get_content_length(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", http_status, length);

        // check the path of the request and handle accordingly
        if (http_status == HttpStatus_Ok && strcmp(path, "/student-question-status") == 0)
        {
            ESP_LOGI(TAG, "response_buffer: %s", response_buffer);
            memset(nvs_data.question_status, 0, 255);
            strncpy(nvs_data.question_status, response_buffer, length);
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));

        // TODO : handle E (22079) HTTP_CLIENT: HTTP GET request failed: ESP_ERR_HTTP_CONNECT
        // failed due to timeout

        http_status == HttpStatus_Forbidden ? http_status = HttpStatus_Forbidden : http_status == 600;
    }

    free(response_buffer);
    response_buffer = NULL;

    esp_http_client_cleanup(client);

    return http_status;
}

size_t http_post_request(char *hostname, char *path, char *post_data, char *session_cookie)
{

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer_,
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
    size_t length = esp_http_client_get_content_length(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {

        ESP_LOGI(TAG, "local_response_buffer_: %s", local_response_buffer_);

        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", http_status, length);

        // if the http request was successful, add new session cookie to nvs
        if (http_status == HttpStatus_Ok && strcmp(path, "/session-smartglasses-login") == 0)
        {
            // BUG FIX: local_reponse_buffer retains previous request data
            if (strstr(local_response_buffer_, "Bad Request"))
            {
                const int bad_request_len = strlen("Bad Request");
                int ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (int i = 0; i < length + bad_request_len; i++)
                {
                    if (i >= bad_request_len)
                    {
                        session_buf[ii++] = local_response_buffer_[i];
                    }
                }
            }
            else if (strstr(local_response_buffer_, "Unauthorized"))
            {
                const int unauthorized_len = strlen("Unauthorized");
                int ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (int i = 0; i < length + unauthorized_len; i++)
                {
                    if (i >= unauthorized_len)
                    {
                        session_buf[ii++] = local_response_buffer_[i];
                    }
                }
            }
            else
            {
                session_buf = malloc(length);
                memset(session_buf, 0, length);

                char session[900] = "session=eyJhbGciOiJSUzI1NiIsImtpZCI6InRCME0yQSJ9.eyJpc3MiOiJodHRwczovL3Nlc3Npb24uZmlyZWJhc2UuZ29vZ2xlLmNvbS9hLXBsdXMtZmlyZWJhc2UiLCJhdWQiOiJhLXBsdXMtZmlyZWJhc2UiLCJhdXRoX3RpbWUiOjE2NTMwNzI4NjYsInVzZXJfaWQiOiIxeTlJMHMwSGhFUmwxcGFzSjFTQnIzT0N4aGUyIiwic3ViIjoiMXk5STBzMEhoRVJsMXBhc0oxU0JyM09DeGhlMiIsImlhdCI6MTY1MzA3Mjg2NiwiZXhwIjoxNjU0MjgyNDY2LCJlbWFpbCI6InN0dWRlbnRAZW1haWwuY29tIiwiZW1haWxfdmVyaWZpZWQiOmZhbHNlLCJmaXJlYmFzZSI6eyJpZGVudGl0aWVzIjp7ImVtYWlsIjpbInN0dWRlbnRAZW1haWwuY29tIl19LCJzaWduX2luX3Byb3ZpZGVyIjoicGFzc3dvcmQifX0.mBSgtykmjgX6lSRJ-pSKSS3CErlIFS9UHSoi5-3fC198rwPRYwvbncZ-sq8AK0ldghr_Lv231Dz8S7FfHgwiDmL7BrCkH4lMJW2-TQR1fGGBRiZ6eIFamMXoCPvfE817CmM78LxVETtpGe9L19m429TESSnn3ngfHY4ObO0LEhqvXtz1EppBNP2FxcSfLRUIC_BXdArqn7KQU2jhC2Kj9oNrAHFm4kUfYVkMQiYY249ACK3InYroYL24gEZZOxE3kx02OYQ9A-D3GjwR0FGhy66SX7_pO4bo9rbsT7peCnQHx8Dc5oIsc1U4cCuBCJlSB8esHK06KwV8XI1efgxZ-w;";
                strcpy(session_buf, local_response_buffer_);

                ESP_LOGI(TAG, "session_buf: %s", session_buf);

                nvs_handle_t nvs_handle;

                err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
                }

                // nvs read id_token
                err = nvs_set_blob(nvs_handle, "session_cookie", session_buf, length);
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
                    strncpy(nvs_data.session_cookie, session_buf, length);
                }
            }

            free(session_buf);
            session_buf = NULL;
        }

        // handle POST /signup/signout_glasses data
        if (http_status == HttpStatus_Ok && strcmp(path, "/signup/signout_glasses") == 0)
        {
            printf("/signup/signout_glasses POST 200 local_response_buffer_:\n");
            for (int i = 0; i < length; i++)
            {
                printf("%c", local_response_buffer_[i]);
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

    // free(local_response_buffer_);
    // local_response_buffer_ = NULL;

    esp_http_client_cleanup(client);

    return http_status;
}

static int sendImage(esp_http_client_handle_t client, camera_fb_t *pic)
{
    img_buf = malloc(pic->len * sizeof(char));
    int img_len = pic->len;
    memcpy(img_buf, pic->buf, pic->len);

    esp_camera_fb_return(pic);

    time_t fileTransferStart = 0;
    time(&fileTransferStart);

    // request header
    char requestHead[200] = "";
    strcat(requestHead, "--");
    strcat(requestHead, HTTP_BOUNDARY);
    strcat(requestHead, "\r\n");
    strcat(requestHead, "Content-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n");

    // TODO : add expiry to HTTP req

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

    esp_err_t err = esp_http_client_set_header(client, "Content-Length", lengthStr);

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
    // char buffer[HTTP_BUFFER_SIZE];
    bool playback_uploading_picture_message = false;
    unsigned fileProgress = 0;
    while (fileProgress < img_len)
    {
        // write the remaining bytes that is less than 1024 to the client
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
        {
            ESP_LOGI(TAG, "%u/%u bytes sent total %.02f KiB/s", fileProgress, img_len, fileProgress / (float)(now - fileTransferStart) / 1024);
        }

        // playback uploading message when the picture is halfway uploaded
        if (fileProgress >= img_len / 2 && !playback_uploading_picture_message)
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
                    err = playback_audio_file(audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf, audio_buf.uploading_the_picture_please_wait_00_wav_len, audio_volume, true);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "playback_audio_file(uploading_the_picture_please_wait_00_wav_audio_buf) err: %s", esp_err_to_name(err));
                    }

                    free_uploading_the_picture_please_wait_00_wav();
                }
            }

            playback_uploading_picture_message = true;
        }
    }

    // finish Multipart request
    ESP_LOGI(TAG, "tail:\t%d", esp_http_client_write(client, tail, strlen(tail)));

    // return pic img_buf
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
        // malloc documentId if the server uploaded ok
        if (status == 200)
        {
            nvs_data.documentId_len = responseLength;

            if (nvs_data.documentId == NULL)
            {
                nvs_data.documentId = malloc(responseLength);
                memset(nvs_data.documentId, 0, nvs_data.documentId_len);
            }

            if (esp_http_client_read(client, nvs_data.documentId, responseLength) == responseLength)
            {
                ESP_LOGI(TAG, "Response: %.*s", nvs_data.documentId_len, nvs_data.documentId);
            }
        }
    }

    err = esp_http_client_close(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to close HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    return status;
}

int https_send_pic(bool cookie, camera_fb_t *pic)
{
    char url[200] = "http://tutorfish-env.eba-tdamw63n.us-east-1.elasticbeanstalk.com/upload-image";

    ESP_LOGI(TAG, "url = %s", url);
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .user_agent = "SmartGlassesOS/1.0.0",
        .cert_pem = howsmyssl_com_root_cert_pem_start,
        .buffer_size_tx = 2048};
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
    err = esp_http_client_set_header(client, "User-Agent", "SmartGlassesOS/1.0.0");
    err = esp_http_client_set_header(client, "Keep-Alive", "300");
    err = esp_http_client_set_header(client, "Connection", "keep-alive");
    err = esp_http_client_set_header(client, "Accept-Language", "en-us");

    if (cookie)
    {
        if (nvs_data.session_cookie != NULL)
        {
            err = esp_http_client_set_header(client, "Cookie", nvs_data.session_cookie);
            ESP_LOGI(TAG, "adding session_cookie to client header");
        }
        else
        {
            // no cookie!
            return -1;
        }
    }

    int ret_status = sendImage(client, pic);

    err = esp_http_client_cleanup(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to clean HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }

    return ret_status;
}