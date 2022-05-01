#ifndef NVS_DATA_STRUCT_H__
#define NVS_DATA_STRUCT_H__

typedef struct
{
    char *user_email;
    size_t user_email_len;
    char *user_pass;
    size_t user_pass_len;
    char *ssid;
    size_t ssid_len;
    char *pass;
    size_t pass_len;
    char *session_cookie;
    size_t session_cookie_len;
} nvs_data_t;

extern nvs_data_t nvs_data;

#endif // NVS_DATA_STRUCT_H__