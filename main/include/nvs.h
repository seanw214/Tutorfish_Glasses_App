#ifndef NVS_H__
#define NVS_H__

esp_err_t read_nvs_email_pass(void);
esp_err_t read_nvs_session_cookie(void);
esp_err_t write_nvs_session_cookie(char *session_cookie);
esp_err_t write_nvs_email_pass(char *user_email, char *user_pass);
esp_err_t erase_nvs_key(char *nvs_key);
esp_err_t write_nvs_value(char *key, char *value);
//esp_err_t read_nvs_value(char *key, char nvs_data_buf, size_t nvs_data_len);
esp_err_t write_nvs_int(char *key, uint8_t value);
esp_err_t read_nvs_int(char *key, uint8_t *value);
void print_nvs_credentials(void);

#endif // NVS_H__