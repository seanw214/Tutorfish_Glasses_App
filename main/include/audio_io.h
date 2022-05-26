#ifndef AUDIO_IO_H__
#define AUDIO_IO_H__

esp_err_t init_i2s(void);
esp_err_t _i2s_stop(void);
esp_err_t playback_audio_file(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable);
esp_err_t play_submit_question_instructions(void);
esp_err_t playback_error_message(void);

esp_err_t malloc_returning_home_wav(void);
void free_returning_home_wav(void);

esp_err_t malloc_attempt_wifi_conn_00_wav(void);
void free_attempt_wifi_conn_00(void);

esp_err_t malloc_wifi_disconn_00_wav(void);

esp_err_t malloc_home_instructions_00_wav(void);
void free_home_instructions_00(void);

esp_err_t malloc_exit_this_app_01_wav(void);
void free_exit_this_app_01(void);

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

esp_err_t malloc_taking_a_picture321_02_wav(void);
void free_taking_a_picture321_02_wav(void);

esp_err_t malloc_uploading_the_picture_please_wait_00_wav(void);
void free_uploading_the_picture_please_wait_00_wav(void);

esp_err_t malloc_error_message_00_wav(void);
void free_error_message_00_wav(void);

esp_err_t malloc_to_hear_the_answer_again_00_wav(void);
void free_to_hear_the_answer_again_00_wav(void);

esp_err_t malloc_tutors_found_answer_00_wav(void);
void free_tutors_found_answer_00_wav(void);

esp_err_t malloc_tutors_look_for_answer_00_wav(void);
void free_tutors_look_for_answer_00_wav(void);

esp_err_t malloc_wait_app_loads_00_wav(void);
void free_wait_app_loads_00_wav(void);


typedef struct
{

    // bool hfp_i2s_playing;
    // bool hfp_i2s_stop;
    bool system_i2s_playing;
    bool system_i2s_stop;

    // downloaded tts audio
    char *tts_audio_buf;
    int tts_audio_len;

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

    int8_t *taking_a_picture321_02_wav_audio_buf;
    int taking_a_picture321_02_wav_len;

    int8_t *uploading_the_picture_please_wait_00_wav_audio_buf;
    int uploading_the_picture_please_wait_00_wav_len;

    int8_t *to_hear_the_answer_again_00_wav_audio_buf;
    int to_hear_the_answer_again_00_wav_len;

    int8_t *tutors_found_answer_00_wav_audio_buf;
    int tutors_found_answer_00_wav_len;

    int8_t *tutors_look_for_answer_00_wav_audio_buf;
    int tutors_look_for_answer_00_wav_len;

    // system audio
    int16_t *returning_home_wav_audio_buf;
    int returning_home_wav_audio_len;

    int16_t *exit_this_app_01_wav_audio_buf;
    int exit_this_app_01_wav_audio_len;

    int16_t *home_instructions_00_wav_audio_buf;
    int home_instructions_00_wav_audio_len;

    int16_t *attempt_wifi_conn_00_wav_audio_buf;
    int attempt_wifi_conn_00_wav_audio_len;

    int16_t *wifi_disconnect_00_wav_audio_buf;
    int wifi_disconnect_00_wav_audio_len;

    int16_t *error_message_00_wav_audio_buf;
    int error_message_00_wav_audio_len;

    int16_t *wait_app_loads_00_wav_audio_buf;
    int wait_app_loads_00_wav_audio_len;
    
} audio_buf_t;

extern audio_buf_t audio_buf;
extern bool repeat_tts_playback;
extern float audio_volume;

#endif //AUDIO_IO_H__