#ifndef NVS_H__
#define NVS_H__

esp_err_t read_nvs_email_pass(void);
esp_err_t read_nvs_session_cookie(void);
esp_err_t write_nvs_session_cookie(char *session_cookie);
esp_err_t write_nvs_email_pass(char *user_email, char *user_pass);
esp_err_t erase_nvs_key(char *nvs_key);
void print_nvs_credentials(void);

#endif // NVS_H__