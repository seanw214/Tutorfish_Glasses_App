#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "nvs_data_struct.h"

static const char *TAG = "nvs";

esp_err_t read_nvs_email_pass(void)
{
    esp_err_t err;

    // read from NVS
    nvs_handle_t nvs_handle;
    size_t length = 0;

    err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }

    // nvs read user_email
    err = nvs_get_str(nvs_handle, "user_email", NULL, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "user_email nvs_get_blob err: %s", esp_err_to_name(err));
    }

    char *user_email = malloc(length);

    err = nvs_get_str(nvs_handle, "user_email", user_email, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "user_email nvs_get_blob err: %s", esp_err_to_name(err));
    }

    nvs_data.user_email = malloc(length);
    nvs_data.user_email_len = length;
    strncpy(nvs_data.user_email, user_email, length);
    free(user_email);

    // nvs read user_password
    err = nvs_get_str(nvs_handle, "user_pass", NULL, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "user_pass nvs_get_blob err: %s", esp_err_to_name(err));
    }

    char *user_pass = malloc(length);

    err = nvs_get_str(nvs_handle, "user_pass", user_pass, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "user_pass nvs_get_blob err: %s", esp_err_to_name(err));
    }

    nvs_data.user_pass = malloc(length);
    nvs_data.user_pass_len = length;
    strncpy(nvs_data.user_pass, user_pass, length);
    free(user_pass);

    nvs_close(nvs_handle);

    return err;
}

esp_err_t read_nvs_session_cookie(void)
{
    esp_err_t err;

    // read from NVS
    nvs_handle_t nvs_handle;
    size_t length = 0;

    err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }

    // malloc the users current session cookie
    err = nvs_get_str(nvs_handle, "session_cookie", NULL, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "session_cookie nvs_get_str() err: %s", esp_err_to_name(err));
    }
    else
    {
        char *session_cookie = malloc(length);

        err = nvs_get_str(nvs_handle, "session_cookie", session_cookie, &length);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "session_cookie nvs_get_str err: %s", esp_err_to_name(err));
        }

        nvs_data.session_cookie = malloc(length);
        nvs_data.session_cookie_len = length;
        strncpy(nvs_data.session_cookie, session_cookie, length);
        free(session_cookie);
    }

    nvs_close(nvs_handle);

    return err;
}

esp_err_t write_nvs_session_cookie(char *session_cookie)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_str(nvs_handle, "session_cookie", session_cookie);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_session_cookie() nvs_set_str(session_cookie) err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_session_cookie() nvs_commit() err: %s", esp_err_to_name(err));
    }

    return err;

}

void print_nvs_credentials(void)
{
    if (nvs_data.user_email != NULL)
    {
        //printf("nvs_data.user_email_len: %d\n", nvs_data.user_email_len);
        for (int i = 0; i < nvs_data.user_email_len; i++)
        {
            printf("%c", nvs_data.user_email[i]);
        }
        printf("\n");
    }
    else
    {
        printf("nvs_data.user_email NULL\n");
    }
    if (nvs_data.user_pass != NULL)
    {
        //printf("nvs_data.user_pass_len: %d\n", nvs_data.user_pass_len);
        for (int i = 0; i < nvs_data.user_pass_len; i++)
        {
            printf("%c", nvs_data.user_pass[i]);
        }
        printf("\n");
    }
    else
    {
        printf("nvs_data.user_pass NULL\n");
    }
    if (nvs_data.session_cookie != NULL)
    {
        //printf("nvs_data.session_cookie: %d\n", nvs_data.session_cookie_len);
        for (int i = 0; i < nvs_data.session_cookie_len; i++)
        {
            printf("%c", nvs_data.session_cookie[i]);
        }
        printf("\n");
    }
    else
    {
        printf("nvs_data.session_cookie NULL\n");
    }
}



