#ifndef BLUE_LED_H__
#define BLUE_LED_H__

esp_err_t init_blue_led(void);
esp_err_t start_blue_led_task(void);
void delete_blue_led_task(void);
esp_err_t set_blue_led(uint8_t level);
int get_blue_led_level(void);

extern bool blue_led_task_init;

#define BLUE_LED_ON (1)
#define BLUE_LED_OFF (0)

#endif //BLUE_LED_H__