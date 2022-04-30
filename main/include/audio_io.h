#ifndef AUDIO_IO_H__
#define AUDIO_IO_H__

esp_err_t init_i2s(void);
esp_err_t _i2s_stop(void);
esp_err_t playback_audio_file(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable);
esp_err_t playback_audio_file_2(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable);
esp_err_t play_submit_question_instructions(void);

esp_err_t malloc_returning_home_wav(void);
void free_returning_home_wav(void);

esp_err_t malloc_attempt_wifi_conn_00_wav(void);
void free_attempt_wifi_conn_00(void);

esp_err_t malloc_wifi_disconn_00_wav(void);

esp_err_t malloc_home_instructions_00_wav(void);
void free_home_instructions_00(void);

esp_err_t malloc_exit_this_app_00_wav(void);
void free_exit_this_app_00(void);

esp_err_t malloc_welcome_to_tutor_fish_01(void);
void free_welcome_to_tutor_fish_01(void);

esp_err_t malloc_submit_a_question_00_wav(void);
void free_submit_a_question_00(void);

esp_err_t malloc_tutor_fish_settings_00_wav(void);
void free_tutor_fish_settings_00(void);

esp_err_t malloc_look_at_your_question_01_wav(void);
void free_look_at_your_question_01(void);

esp_err_t malloc_the_camera_take_a_pic_01_wav(void);
void free_the_camera_take_a_pic_01(void);

esp_err_t malloc_to_conserve_battery_01_wav(void);
void free_to_conserve_battery_01(void);

void malloc_p_wav(void);

typedef struct
{
    // bool hfp_i2s_playing;
    // bool hfp_i2s_stop;
    bool system_i2s_playing;
    bool system_i2s_stop;

    // test audio
    int8_t *p_wav_audio_buf;
    int p_wav_len;

    // tutorfish audio
    int8_t *welcome_01_wav_audio_buf;
    int welcome_01_audio_len;

    int8_t *submit_a_question_00_wav_audio_buf;
    int submit_a_question_00_wav_len;

    int8_t *tutor_fish_settings_00_wav_audio_buf;
    int tutor_fish_settings_00_wav_len;

    int8_t *look_at_your_question_01_wav_audio_buf;
    int look_at_your_question_01_wav_len;

    int8_t *the_camera_take_a_pic_01_wav_audio_buf;
    int the_camera_take_a_pic_01_wav_len;

    int8_t *to_conserve_battery_01_wav_audio_buf;
    int to_conserve_battery_01_wav_len;

    // system audio
    int16_t *returning_home_wav_audio_buf;
    int returning_home_wav_audio_len;

    int16_t *exit_this_app_00_wav_audio_buf;
    int exit_this_app_00_wav_audio_len;

    int16_t *home_instructions_00_wav_audio_buf;
    int home_instructions_00_wav_audio_len;

    int16_t *attempt_wifi_conn_00_wav_audio_buf;
    int attempt_wifi_conn_00_wav_audio_len;

    int16_t *wifi_disconnect_00_wav_audio_buf;
    int wifi_disconnect_00_wav_audio_len;
} audio_buf_t;

extern audio_buf_t audio_buf;
extern bool repeat_tts_playback;

#endif //AUDIO_IO_H__