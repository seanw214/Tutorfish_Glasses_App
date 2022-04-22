#ifndef AUDIO_IO_H__
#define AUDIO_IO_H__

esp_err_t init_i2s(void);
esp_err_t _i2s_stop(void);
esp_err_t playback_audio_file(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable);

esp_err_t malloc_home_sfx();
esp_err_t malloc_returning_home_wav_sfx();
esp_err_t malloc_home_instructions_00_wav();

esp_err_t malloc_attempt_wifi_conn_00_wav();
esp_err_t malloc_start_wifi_conn_00_wav(void);
esp_err_t malloc_starting_phone_call_mode_00_wav(void);
esp_err_t malloc_start_phone_call_mode_00_wav(void);
esp_err_t malloc_open_app_store_00_wav(void);
esp_err_t malloc_open_camera_00_wav(void);
esp_err_t malloc_open_settings_00_wav(void);
esp_err_t malloc_open_voice_memo_00_wav(void);
esp_err_t malloc_wifi_disconn_00_wav(void);

typedef struct
{
    bool hfp_i2s_playing;
    bool hfp_i2s_stop;
    bool system_i2s_playing;
    bool system_i2s_stop;
    int16_t *home_sfx_audio_buf;
    int home_sfx_audio_len;
    int16_t *returning_home_wav_audio_buf;
    int returning_home_wav_audio_len;
    int16_t *home_instructions_00_wav_audio_buf;
    int home_instructions_00_wav_audio_len;
    int16_t *attempt_wifi_conn_00_wav_audio_buf;
    int attempt_wifi_conn_00_wav_audio_len;
    int16_t *start_wifi_conn_00_wav_audio_buf;
    int start_wifi_conn_00_wav_audio_len;
    int16_t *starting_phone_call_mode_00_wav_audio_buf;
    int starting_phone_call_mode_00_wav_audio_len;
    int16_t *start_phone_call_mode_00_wav_audio_buf;
    int start_phone_call_mode_00_wav_audio_len;
    int16_t *open_app_store_00_wav_audio_buf;
    int open_app_store_00_wav_audio_len;
    int16_t *open_camera_00_wav_audio_buf;
    int open_camera_00_wav_audio_len;
    int16_t *open_settings_00_wav_audio_buf;
    int open_settings_00_wav_audio_len;
    int16_t *open_voice_memo_00_wav_audio_buf;
    int open_voice_memo_00_wav_audio_len;
    int16_t *wifi_disconnect_00_wav_audio_buf;
    int wifi_disconnect_00_wav_audio_len;
} audio_buf_t;

extern audio_buf_t audio_buf;

#endif //AUDIO_IO_H__