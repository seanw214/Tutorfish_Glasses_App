#ifndef WIFI_BT_STATUS_H__
#define WIFI_BT_STATUS_H__

typedef struct
{
    bool hfp_init;
    bool hfp_conn;
    //bool voice_assistant_active;
    bool wifi_init;
    bool wifi_sta_init;
    bool wifi_conn;
    bool user_end_bt_conn;
    bool user_end_wifi_conn;
} wifi_bt_status_t;

extern wifi_bt_status_t wifi_bt_status;

#endif //WIFI_BT_STATUS_T__