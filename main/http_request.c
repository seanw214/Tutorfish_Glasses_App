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
//#include "glasses_state_machine.h"
#include "esp_camera.h"
#include "esp_ota.h"
#include "nvs_data_struct.h"

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

char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

typedef enum
{
    SESSION
} queries_t;

queries_t queries;

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

size_t http_get_request(char *hostname, char *path, char *query, bool cookie)
{

    // char *local_response_buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    // memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    esp_http_client_config_t config = {
        .host = hostname,
        .path = path,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer, // Pass address of local buffer to get response
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
            ESP_LOGI(TAG, "local_response_buffer: %s", local_response_buffer);

            // BUG FIX: local_reponse_buffer retains previous request data
            if (strstr(local_response_buffer, "unanswered"))
            {
                length = strlen("unanswered");
            }
            else if (strstr(local_response_buffer, "pending"))
            {
                length = strlen("pending");
            }
            else if (strstr(local_response_buffer, "answered"))
            {
                length = strlen("answered");
            }
            else if (strstr(local_response_buffer, "issue"))
            {
                length = strlen("issue");
            }
            else if (strstr(local_response_buffer, "expired"))
            {
                length = strlen("expired");
            }
            else if (strstr(local_response_buffer, "canceled"))
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
            strncpy(nvs_data.question_status, local_response_buffer, nvs_data.question_status_len);
        }
    }
    /*
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", http_status, length);

        // if the http request was successful, add new session cookie to nvs
        if (http_status == HttpStatus_Ok && strcmp(path, "/student-question-status") == 0)
        {
            ESP_LOGI(TAG, "local_response_buffer: %s", local_response_buffer);

            // BUG FIX: local_reponse_buffer retains previous request data
            if (strstr(local_response_buffer, "unanswered"))
            {
                const uint8_t unanswered_len = strlen("unanswered");
                uint8_t ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (uint8_t i = 0; i < length + unanswered_len; i++)
                {
                    if (i >= unanswered_len)
                    {
                        nvs_data.question_status[ii++] = local_response_buffer[i];
                    }
                }
            }
            else if (strstr(local_response_buffer, "pending"))
            {
                const uint8_t pending_len = strlen("pending");
                uint8_t ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (uint8_t i = 0; i < length + pending_len; i++)
                {
                    if (i >= pending_len)
                    {
                        nvs_data.question_status[ii++] = local_response_buffer[i];
                    }
                }
            }
            else if (strstr(local_response_buffer, "answered"))
            {
                const uint8_t answered_len = strlen("answered");
                uint8_t ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (uint8_t i = 0; i < length + answered_len; i++)
                {
                    if (i >= answered_len)
                    {
                        nvs_data.question_status[ii++] = local_response_buffer[i];
                    }
                }
            }
            else if (strstr(local_response_buffer, "issue"))
            {
                const uint8_t issue_len = strlen("issue");
                uint8_t ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (uint8_t i = 0; i < length + issue_len; i++)
                {
                    if (i >= issue_len)
                    {
                        nvs_data.question_status[ii++] = local_response_buffer[i];
                    }
                }
            }
            else if (strstr(local_response_buffer, "expired"))
            {
                const uint8_t expired_len = strlen("expired");
                uint8_t ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (uint8_t i = 0; i < length + expired_len; i++)
                {
                    if (i >= expired_len)
                    {
                        nvs_data.question_status[ii++] = local_response_buffer[i];
                    }
                }
            }
            else if (strstr(local_response_buffer, "canceled"))
            {
                const uint8_t canceled_len = strlen("canceled");
                uint8_t ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (uint8_t i = 0; i < length + canceled_len; i++)
                {
                    if (i >= canceled_len)
                    {
                        nvs_data.question_status[ii++] = local_response_buffer[i];
                    }
                }
            }

            // make sure the session_buf has data
            if (session_buf != NULL)
            {
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
                memcpy(nvs_data.question_status, session_buf, length);
            }
        }
    }
    */
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

size_t http_post_request(char *hostname, char *path, char *post_data, char *session_cookie)
{

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
    size_t length = esp_http_client_get_content_length(client);
    size_t http_status = esp_http_client_get_status_code(client);

    if (err == ESP_OK)
    {

        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", http_status, length);

        // if the http request was successful, add new session cookie to nvs
        if (http_status == HttpStatus_Ok && strcmp(path, "/session-smartglasses-login") == 0)
        {
            // BUG FIX: local_reponse_buffer retains previous request data
            if (strstr(local_response_buffer, "Bad Request"))
            {
                const int bad_request_len = strlen("Bad Request");
                int ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (int i = 0; i < length + bad_request_len; i++)
                {
                    if (i >= bad_request_len)
                    {
                        session_buf[ii++] = local_response_buffer[i];
                    }
                }
            }
            else if (strstr(local_response_buffer, "Unauthorized"))
            {
                const int unauthorized_len = strlen("Unauthorized");
                int ii = 0;
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                for (int i = 0; i < length + unauthorized_len; i++)
                {
                    if (i >= unauthorized_len)
                    {
                        session_buf[ii++] = local_response_buffer[i];
                    }
                }
            }
            else
            {
                session_buf = malloc(length);
                memset(session_buf, 0, length);
                strcpy(session_buf, local_response_buffer);
            }

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

                // printf("/session-smartglasses-login POST 200 nvs_data.session_cookie:\n");
                // for (int i = 0; i < nvs_data.session_cookie_len; i++)
                // {
                //     printf("%c", nvs_data.session_cookie[i]);
                // }
                // printf("\n");
            }

            free(session_buf);
            session_buf = NULL;
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

    // free(post_local_response_buffer);
    // post_local_response_buffer = NULL;

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