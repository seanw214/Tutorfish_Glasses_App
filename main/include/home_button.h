#ifndef HOME_BUTTON_H__
#define HOME_BUTTON_H__

#define GPIO_HOME_BUTTON (37)

extern bool gpio_wakeup_enabled;
esp_err_t init_home_button(void);

#endif //HOME_BUTTON_H__