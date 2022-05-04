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
    err = nvs_get_blob(nvs_handle, "session_cookie", NULL, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "session_cookie nvs_get_blob() err: %s", esp_err_to_name(err));
    }
    else
    {
        char *session_cookie = malloc(length);

        err = nvs_get_blob(nvs_handle, "session_cookie", session_cookie, &length);
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
    
    err = nvs_set_blob(nvs_handle, "session_cookie", session_cookie, strlen(session_cookie));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_session_cookie() nvs_set_blob(session_cookie) err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_session_cookie() nvs_commit() err: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    return err;
}

esp_err_t write_nvs_value(char *key, char *value)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_blob(nvs_handle, key, value, strlen(value));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_value() nvs_set_blob(value) err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_value() nvs_commit() err: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    return err;
}

/*
esp_err_t read_nvs_value(char *key, char nvs_data_buf, size_t nvs_data_len)
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
    err = nvs_get_blob(nvs_handle, key, NULL, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "session_cookie nvs_get_blob() err: %s", esp_err_to_name(err));
    }
    else
    {
        char *value = malloc(length);

        err = nvs_get_blob(nvs_handle, key, value, &length);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "session_cookie nvs_get_str err: %s", esp_err_to_name(err));
        }

        // error
        *nvs_data_buf = malloc(length);
        nvs_data_len = length;
        strncpy(&nvs_data_buf, value, length);
        free(value);
    }

    nvs_close(nvs_handle);

    return err;
}
*/

esp_err_t write_nvs_int(char *key, uint8_t value)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_u8(nvs_handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_value() nvs_set_u8(value) err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_value() nvs_commit() err: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    return err;
}

esp_err_t read_nvs_int(char *key, uint8_t *value)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_get_u8(nvs_handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_int() nvs_get_u8(value) err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "read_nvs_int() nvs_commit() err: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    return err;
}


esp_err_t write_nvs_email_pass(char *user_email, char *user_pass)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open err: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_str(nvs_handle, "user_email", user_email);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_email_pass() nvs_set_str(user_email) err: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "user_pass", user_pass);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_email_pass() nvs_set_str(user_pass) err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "write_nvs_email_pass() nvs_commit() err: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    return err;

}

esp_err_t erase_nvs_key(char *nvs_key)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "erase_nvs_key() nvs_open() err: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_erase_key(nvs_handle, nvs_key);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "erase_nvs_key() nvs_erase_key() err: %s", esp_err_to_name(err));
    }

    nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "erase_nvs_key() nvs_commit() err: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

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
    // if (nvs_data.jpeg_quality_exponent != NULL)
    // {
    //     printf("nvs_data.jpeg_quality_exponent: %d", nvs_data.jpeg_quality_exponent);
    //     printf("\n");
    // }
    // else
    // {
    //     printf("nvs_data.jpeg_quality NULL\n");
    // }
    // if (nvs_data.attempting_pic_capture != NULL)
    // {
    //     printf("nvs_data.attempting_pic_capture: %d", nvs_data.attempting_pic_capture);
    //     printf("\n");
    // }
    // else
    // {
    //     printf("nvs_data.attempting_pic_capture NULL\n");
    // }
}



