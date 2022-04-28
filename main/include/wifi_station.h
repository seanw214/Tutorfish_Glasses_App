#ifndef WIFI_STATION_H__
#define WIFI_STATION_H__

esp_err_t wifi_init_sta(void);
esp_err_t _esp_wifi_init(void);
esp_err_t _wifi_deinit(void);
esp_err_t start_wifi(void);
esp_err_t stop_wifi(void);

#endif // WIFI_STATION_H__