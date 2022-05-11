#ifndef RED_WHT_LED_H__
#define RED_WHT_LED_H__

esp_err_t init_red_wht_led(void);
esp_err_t set_rx_level(uint8_t level);
esp_err_t set_tx_level(uint8_t level);

#endif //RED_WHT_LED_H__