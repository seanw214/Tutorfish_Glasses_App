#ifndef TOUCH_H__
#define TOUCH_H__

esp_err_t init_touch(void);

extern uint8_t menu_selection_idx;
extern bool touch_pad_forward;
extern bool touch_pad_backward;

#endif //TOUCH_H__