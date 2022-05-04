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
    uint8_t jpeg_quality_exponent;
    uint8_t attempting_pic_capture; // 0 = false; 1 = true;
} nvs_data_t;

extern nvs_data_t nvs_data;

#endif // NVS_DATA_STRUCT_H__