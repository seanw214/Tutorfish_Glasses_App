#include <string.h>
#include "driver/i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio_io.h"

#include "welcome_to_tutor_fish_01.h"
#include "submit_a_question_00.h"
#include "tutor_fish_settings_00.h"
#include "look_at_your_question_01.h"
#include "the_camera_take_a_pic_01.h"
#include "to_conserve_battery_01.h"
#include "taking_a_picture321_02.h"
#include "uploading_the_picture_please_wait_00.h"
#include "to_hear_the_answer_again_00.h"
#include "tutors_found_answer_00.h"
#include "tutors_look_for_answer_00.h"

static const char *TAG = "audio_io.c";

#define I2S_BCK_IO (2)
#define I2S_WS_IO (19)
#define I2S_DO_IO (5)
#define I2S_DI_IO (36)

static QueueHandle_t i2s_event_queue;
static int queued_audio_file_len = 0;

static int16_t *temp_buf = NULL;
static bool queued_audio_file_stoppable;

audio_buf_t audio_buf;

esp_err_t malloc_confirm_sfx_04_wav(void)
{
    const char *audio_file_path = "/audio/confirm_sfx_04.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.confirm_sfx_04_wav_audio_len = ftell(f);
    audio_buf.confirm_sfx_04_wav_audio_buf = malloc(audio_buf.confirm_sfx_04_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.confirm_sfx_04_wav_audio_buf, sizeof(uint8_t), audio_buf.confirm_sfx_04_wav_audio_len, f);
    if (read_ret != audio_buf.confirm_sfx_04_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.confirm_sfx_04_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_confirm_sfx_04_wav(void)
{
    free(audio_buf.confirm_sfx_04_wav_audio_buf);
    audio_buf.confirm_sfx_04_wav_audio_buf = NULL;
}


esp_err_t malloc_wait_app_loads_00_wav(void)
{
    const char *audio_file_path = "/audio/wait_app_loads_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.wait_app_loads_00_wav_audio_len = ftell(f);
    audio_buf.wait_app_loads_00_wav_audio_buf = malloc(audio_buf.wait_app_loads_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.wait_app_loads_00_wav_audio_buf, sizeof(uint8_t), audio_buf.wait_app_loads_00_wav_audio_len, f);
    if (read_ret != audio_buf.wait_app_loads_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.wait_app_loads_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_wait_app_loads_00_wav(void)
{
    free(audio_buf.wait_app_loads_00_wav_audio_buf);
    audio_buf.wait_app_loads_00_wav_audio_buf = NULL;
}

esp_err_t malloc_wifi_disconn_00_wav(void)
{
    const char *audio_file_path = "/audio/wifi_disconn_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.wifi_disconnect_00_wav_audio_len = ftell(f);
    audio_buf.wifi_disconnect_00_wav_audio_buf = malloc(audio_buf.wifi_disconnect_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.wifi_disconnect_00_wav_audio_buf, sizeof(uint8_t), audio_buf.wifi_disconnect_00_wav_audio_len, f);
    if (read_ret != audio_buf.wifi_disconnect_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.wifi_disconnect_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t malloc_attempt_wifi_conn_00_wav(void)
{
    const char *audio_file_path = "/audio/attempt_wifi_conn_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.attempt_wifi_conn_00_wav_audio_len = ftell(f);
    audio_buf.attempt_wifi_conn_00_wav_audio_buf = malloc(audio_buf.attempt_wifi_conn_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.attempt_wifi_conn_00_wav_audio_buf, sizeof(uint8_t), audio_buf.attempt_wifi_conn_00_wav_audio_len, f);
    if (read_ret != audio_buf.attempt_wifi_conn_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.attempt_wifi_conn_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_attempt_wifi_conn_00(void)
{
    free(audio_buf.attempt_wifi_conn_00_wav_audio_buf);
    audio_buf.attempt_wifi_conn_00_wav_audio_buf = NULL;
}

esp_err_t malloc_returning_home_wav_sfx(void)
{
    const char *audio_file_path = "/audio/returning_home_07.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.returning_home_wav_audio_len = ftell(f);
    audio_buf.returning_home_wav_audio_buf = malloc(audio_buf.returning_home_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.returning_home_wav_audio_buf, sizeof(uint8_t), audio_buf.returning_home_wav_audio_len, f);
    if (read_ret != audio_buf.returning_home_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.returning_home_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_returning_home_wav_sfx(void)
{
    free(audio_buf.returning_home_wav_audio_buf);
    audio_buf.returning_home_wav_audio_buf = NULL;
}

esp_err_t malloc_home_instructions_00_wav(void)
{
    const char *audio_file_path = "/audio/home_instructions_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.home_instructions_00_wav_audio_len = ftell(f);
    audio_buf.home_instructions_00_wav_audio_buf = malloc(audio_buf.home_instructions_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.home_instructions_00_wav_audio_buf, sizeof(uint8_t), audio_buf.home_instructions_00_wav_audio_len, f);
    if (read_ret != audio_buf.home_instructions_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.home_instructions_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_home_instructions_00(void)
{
    free(audio_buf.home_instructions_00_wav_audio_buf);
    audio_buf.home_instructions_00_wav_audio_buf = NULL;
}

esp_err_t malloc_exit_this_app_01_wav(void)
{
    const char *audio_file_path = "/audio/exit_this_app_01.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.exit_this_app_01_wav_audio_len = ftell(f);
    audio_buf.exit_this_app_01_wav_audio_buf = malloc(audio_buf.exit_this_app_01_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.exit_this_app_01_wav_audio_buf, sizeof(uint8_t), audio_buf.exit_this_app_01_wav_audio_len, f);
    if (read_ret != audio_buf.exit_this_app_01_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.exit_this_app_01_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_exit_this_app_01(void)
{
    free(audio_buf.exit_this_app_01_wav_audio_buf);
    audio_buf.exit_this_app_01_wav_audio_buf = NULL;
}

esp_err_t malloc_error_message_00_wav(void)
{
    const char *audio_file_path = "/audio/error_message_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.error_message_00_wav_audio_len = ftell(f);
    audio_buf.error_message_00_wav_audio_buf = malloc(audio_buf.error_message_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.error_message_00_wav_audio_buf, sizeof(uint8_t), audio_buf.error_message_00_wav_audio_len, f);
    if (read_ret != audio_buf.error_message_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.error_message_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_error_message_00_wav(void)
{
    free(audio_buf.error_message_00_wav_audio_buf);
    audio_buf.error_message_00_wav_audio_buf = NULL;
}

esp_err_t malloc_welcome_to_tutor_fish_01(void)
{
    audio_buf.welcome_01_audio_len = welcome_to_tutor_fish_01_wav_len;

    audio_buf.welcome_01_wav_audio_buf = malloc(audio_buf.welcome_01_audio_len);

    for (int i = 0; i < audio_buf.welcome_01_audio_len; i++)
    {
        audio_buf.welcome_01_wav_audio_buf[i] = welcome_to_tutor_fish_01_wav[i];
    }

    return ESP_OK;
}

void free_welcome_to_tutor_fish_01(void)
{
    free(audio_buf.welcome_01_wav_audio_buf);
    audio_buf.welcome_01_wav_audio_buf = NULL;
}

esp_err_t malloc_submit_a_question_00_wav(void)
{
    audio_buf.submit_a_question_00_wav_len = submit_a_question_00_wav_len;

    audio_buf.submit_a_question_00_wav_audio_buf = malloc(audio_buf.submit_a_question_00_wav_len);

    for (int i = 0; i < audio_buf.submit_a_question_00_wav_len; i++)
    {
        audio_buf.submit_a_question_00_wav_audio_buf[i] = submit_a_question_00_wav[i];
    }

    return ESP_OK;
}

void free_submit_a_question_00(void)
{
    free(audio_buf.submit_a_question_00_wav_audio_buf);
    audio_buf.submit_a_question_00_wav_audio_buf = NULL;
}

esp_err_t malloc_tutor_fish_settings_00_wav(void)
{
    audio_buf.tutor_fish_settings_00_wav_len = tutor_fish_settings_00_wav_len;

    audio_buf.tutor_fish_settings_00_wav_audio_buf = malloc(audio_buf.tutor_fish_settings_00_wav_len);

    for (int i = 0; i < audio_buf.tutor_fish_settings_00_wav_len; i++)
    {
        audio_buf.tutor_fish_settings_00_wav_audio_buf[i] = tutor_fish_settings_00_wav[i];
    }

    return ESP_OK;
}

void free_tutor_fish_settings_00(void)
{
    free(audio_buf.tutor_fish_settings_00_wav_audio_buf);
    audio_buf.tutor_fish_settings_00_wav_audio_buf = NULL;
}

esp_err_t malloc_look_at_your_question_01_wav(void)
{
    audio_buf.look_at_your_question_01_wav_len = look_at_your_question_01_wav_len;

    audio_buf.look_at_your_question_01_wav_audio_buf = malloc(audio_buf.look_at_your_question_01_wav_len);

    for (int i = 0; i < audio_buf.look_at_your_question_01_wav_len; i++)
    {
        audio_buf.look_at_your_question_01_wav_audio_buf[i] = look_at_your_question_01_wav[i];
    }

    return ESP_OK;
}

void free_look_at_your_question_01(void)
{
    free(audio_buf.look_at_your_question_01_wav_audio_buf);
    audio_buf.look_at_your_question_01_wav_audio_buf = NULL;
}

esp_err_t malloc_the_camera_take_a_pic_01_wav(void)
{
    audio_buf.the_camera_take_a_pic_01_wav_len = the_camera_take_a_pic_01_wav_len;

    audio_buf.the_camera_take_a_pic_01_wav_audio_buf = malloc(audio_buf.the_camera_take_a_pic_01_wav_len);

    for (int i = 0; i < audio_buf.the_camera_take_a_pic_01_wav_len; i++)
    {
        audio_buf.the_camera_take_a_pic_01_wav_audio_buf[i] = the_camera_take_a_pic_01_wav[i];
    }

    return ESP_OK;
}

void free_the_camera_take_a_pic_01(void)
{
    free(audio_buf.the_camera_take_a_pic_01_wav_audio_buf);
    audio_buf.the_camera_take_a_pic_01_wav_audio_buf = NULL;
}

esp_err_t malloc_to_conserve_battery_01_wav(void)
{
    audio_buf.to_conserve_battery_01_wav_len = to_conserve_battery_01_wav_len;

    audio_buf.to_conserve_battery_01_wav_audio_buf = malloc(audio_buf.to_conserve_battery_01_wav_len);

    for (int i = 0; i < audio_buf.to_conserve_battery_01_wav_len; i++)
    {
        audio_buf.to_conserve_battery_01_wav_audio_buf[i] = to_conserve_battery_01_wav[i];
    }

    return ESP_OK;
}

void free_to_conserve_battery_01(void)
{
    free(audio_buf.to_conserve_battery_01_wav_audio_buf);
    audio_buf.to_conserve_battery_01_wav_audio_buf = NULL;
}

esp_err_t malloc_taking_a_picture321_02_wav(void)
{
    audio_buf.taking_a_picture321_02_wav_len = taking_a_picture321_02_wav_len;

    audio_buf.taking_a_picture321_02_wav_audio_buf = malloc(audio_buf.taking_a_picture321_02_wav_len);

    for (int i = 0; i < audio_buf.taking_a_picture321_02_wav_len; i++)
    {
        audio_buf.taking_a_picture321_02_wav_audio_buf[i] = taking_a_picture321_02_wav[i];
    }

    return ESP_OK;
}

void free_taking_a_picture321_02_wav(void)
{
    free(audio_buf.taking_a_picture321_02_wav_audio_buf);
    audio_buf.taking_a_picture321_02_wav_audio_buf = NULL;
}

esp_err_t malloc_uploading_the_picture_please_wait_00_wav(void)
{
    audio_buf.uploading_the_picture_please_wait_00_wav_len = uploading_the_picture_please_wait_00_wav_len;

    audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf = malloc(audio_buf.uploading_the_picture_please_wait_00_wav_len);

    for (int i = 0; i < audio_buf.uploading_the_picture_please_wait_00_wav_len; i++)
    {
        audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf[i] = uploading_the_picture_please_wait_00_wav[i];
    }

    return ESP_OK;
}

void free_uploading_the_picture_please_wait_00_wav(void)
{
    free(audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf);
    audio_buf.uploading_the_picture_please_wait_00_wav_audio_buf = NULL;
}

esp_err_t malloc_to_hear_the_answer_again_00_wav(void)
{
    audio_buf.to_hear_the_answer_again_00_wav_len = to_hear_the_answer_again_00_wav_len;

    audio_buf.to_hear_the_answer_again_00_wav_audio_buf = malloc(audio_buf.to_hear_the_answer_again_00_wav_len);

    for (int i = 0; i < audio_buf.to_hear_the_answer_again_00_wav_len; i++)
    {
        audio_buf.to_hear_the_answer_again_00_wav_audio_buf[i] = to_hear_the_answer_again_00_wav[i];
    }

    return ESP_OK;
}

void free_to_hear_the_answer_again_00_wav(void)
{
    free(audio_buf.to_hear_the_answer_again_00_wav_audio_buf);
    audio_buf.to_hear_the_answer_again_00_wav_audio_buf = NULL;
}

esp_err_t malloc_tutors_found_answer_00_wav(void)
{
    audio_buf.tutors_found_answer_00_wav_len = tutors_found_answer_00_wav_len;

    audio_buf.tutors_found_answer_00_wav_audio_buf = malloc(audio_buf.tutors_found_answer_00_wav_len);

    for (int i = 0; i < audio_buf.tutors_found_answer_00_wav_len; i++)
    {
        audio_buf.tutors_found_answer_00_wav_audio_buf[i] = tutors_found_answer_00_wav[i];
    }

    return ESP_OK;
}

void free_tutors_found_answer_00_wav(void)
{
    free(audio_buf.tutors_found_answer_00_wav_audio_buf);
    audio_buf.to_hear_the_answer_again_00_wav_audio_buf = NULL;
}

esp_err_t malloc_tutors_look_for_answer_00_wav(void)
{
    audio_buf.tutors_look_for_answer_00_wav_len = tutors_look_for_answer_00_wav_len;

    audio_buf.tutors_look_for_answer_00_wav_audio_buf = malloc(audio_buf.tutors_look_for_answer_00_wav_len);

    for (int i = 0; i < audio_buf.tutors_look_for_answer_00_wav_len; i++)
    {
        audio_buf.tutors_look_for_answer_00_wav_audio_buf[i] = tutors_look_for_answer_00_wav[i];
    }

    return ESP_OK;
}

void free_tutors_look_for_answer_00_wav(void)
{
    free(audio_buf.tutors_look_for_answer_00_wav_audio_buf);
    audio_buf.tutors_look_for_answer_00_wav_audio_buf = NULL;
}

esp_err_t malloc_ascii_5_00_wav(void)
{
    const char *audio_file_path = "/audio/ascii_5_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.ascii_5_00_wav_audio_len = ftell(f);
    audio_buf.ascii_5_00_wav_audio_buf = malloc(audio_buf.ascii_5_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.ascii_5_00_wav_audio_buf, sizeof(uint8_t), audio_buf.ascii_5_00_wav_audio_len, f);
    if (read_ret != audio_buf.ascii_5_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.ascii_5_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_ascii_5_00_wav(void)
{
    free(audio_buf.ascii_5_00_wav_audio_buf);
    audio_buf.ascii_5_00_wav_audio_buf = NULL;
}

///////

esp_err_t malloc_percent_completed_00_wav(void)
{
    const char *audio_file_path = "/audio/percent_completed_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf.percent_completed_00_wav_audio_len = ftell(f);
    audio_buf.percent_completed_00_wav_audio_buf = malloc(audio_buf.percent_completed_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.percent_completed_00_wav_audio_buf, sizeof(uint8_t), audio_buf.percent_completed_00_wav_audio_len, f);
    if (read_ret != audio_buf.percent_completed_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.percent_completed_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_percent_completed_00_wav(void)
{
    free(audio_buf.percent_completed_00_wav_audio_buf);
    audio_buf.percent_completed_00_wav_audio_buf = NULL;
}

esp_err_t malloc_10_00_wav(void)
{
    const char *audio_file_path = "/audio/10_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._10_00_wav_audio_len = ftell(f);
    audio_buf._10_00_wav_audio_buf = malloc(audio_buf._10_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._10_00_wav_audio_buf, sizeof(uint8_t), audio_buf._10_00_wav_audio_len, f);
    if (read_ret != audio_buf._10_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._10_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_10_00_wav(void)
{
    free(audio_buf._10_00_wav_audio_buf);
    audio_buf._10_00_wav_audio_buf = NULL;
}

esp_err_t malloc_20_00_wav(void)
{
    const char *audio_file_path = "/audio/20_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._20_00_wav_audio_len = ftell(f);
    audio_buf._20_00_wav_audio_buf = malloc(audio_buf._20_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._20_00_wav_audio_buf, sizeof(uint8_t), audio_buf._20_00_wav_audio_len, f);
    if (read_ret != audio_buf._20_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._20_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_20_00_wav(void)
{
    free(audio_buf._20_00_wav_audio_buf);
    audio_buf._20_00_wav_audio_buf = NULL;
}

esp_err_t malloc_30_00_wav(void)
{
    const char *audio_file_path = "/audio/30_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._30_00_wav_audio_len = ftell(f);
    audio_buf._30_00_wav_audio_buf = malloc(audio_buf._30_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._30_00_wav_audio_buf, sizeof(uint8_t), audio_buf._30_00_wav_audio_len, f);
    if (read_ret != audio_buf._30_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._30_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_30_00_wav(void)
{
    free(audio_buf._30_00_wav_audio_buf);
    audio_buf._30_00_wav_audio_buf = NULL;
}

esp_err_t malloc_40_00_wav(void)
{
    const char *audio_file_path = "/audio/40_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._40_00_wav_audio_len = ftell(f);
    audio_buf._40_00_wav_audio_buf = malloc(audio_buf._40_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._40_00_wav_audio_buf, sizeof(uint8_t), audio_buf._40_00_wav_audio_len, f);
    if (read_ret != audio_buf._40_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._40_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_40_00_wav(void)
{
    free(audio_buf._40_00_wav_audio_buf);
    audio_buf._40_00_wav_audio_buf = NULL;
}

esp_err_t malloc_50_00_wav(void)
{
    const char *audio_file_path = "/audio/50_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._50_00_wav_audio_len = ftell(f);
    audio_buf._50_00_wav_audio_buf = malloc(audio_buf._50_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._50_00_wav_audio_buf, sizeof(uint8_t), audio_buf._50_00_wav_audio_len, f);
    if (read_ret != audio_buf._50_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._50_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_50_00_wav(void)
{
    free(audio_buf._50_00_wav_audio_buf);
    audio_buf._50_00_wav_audio_buf = NULL;
}

esp_err_t malloc_60_00_wav(void)
{
    const char *audio_file_path = "/audio/60_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._60_00_wav_audio_len = ftell(f);
    audio_buf._60_00_wav_audio_buf = malloc(audio_buf._60_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._60_00_wav_audio_buf, sizeof(uint8_t), audio_buf._60_00_wav_audio_len, f);
    if (read_ret != audio_buf._60_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._60_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_60_00_wav(void)
{
    free(audio_buf._60_00_wav_audio_buf);
    audio_buf._60_00_wav_audio_buf = NULL;
}

esp_err_t malloc_70_00_wav(void)
{
    const char *audio_file_path = "/audio/70_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._70_00_wav_audio_len = ftell(f);
    audio_buf._70_00_wav_audio_buf = malloc(audio_buf._70_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._70_00_wav_audio_buf, sizeof(uint8_t), audio_buf._70_00_wav_audio_len, f);
    if (read_ret != audio_buf._70_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._70_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_70_00_wav(void)
{
    free(audio_buf._70_00_wav_audio_buf);
    audio_buf._70_00_wav_audio_buf = NULL;
}

esp_err_t malloc_80_00_wav(void)
{
    const char *audio_file_path = "/audio/80_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._80_00_wav_audio_len = ftell(f);
    audio_buf._80_00_wav_audio_buf = malloc(audio_buf._80_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._80_00_wav_audio_buf, sizeof(uint8_t), audio_buf._80_00_wav_audio_len, f);
    if (read_ret != audio_buf._80_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._80_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_80_00_wav(void)
{
    free(audio_buf._80_00_wav_audio_buf);
    audio_buf._80_00_wav_audio_buf = NULL;
}

esp_err_t malloc_90_00_wav(void)
{
    const char *audio_file_path = "/audio/90_00.wav";

    FILE *f = fopen(audio_file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file reading");
        return ESP_FAIL;
    }

    // Moving pointer to end
    int f_err = fseek(f, 0, SEEK_END);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_END error");
        return ESP_FAIL;
    }

    // get audio file length
    audio_buf._90_00_wav_audio_len = ftell(f);
    audio_buf._90_00_wav_audio_buf = malloc(audio_buf._90_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf._90_00_wav_audio_buf, sizeof(uint8_t), audio_buf._90_00_wav_audio_len, f);
    if (read_ret != audio_buf._90_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf._90_00_wav_audio_len);
        return ESP_FAIL;
    }

    f_err = fclose(f);
    if (f_err != 0)
    {
        ESP_LOGW(TAG, "fclose() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void free_90_00_wav(void)
{
    free(audio_buf._90_00_wav_audio_buf);
    audio_buf._90_00_wav_audio_buf = NULL;
}


esp_err_t play_submit_question_instructions(void)
{
    esp_err_t err;

    if (audio_buf.the_camera_take_a_pic_01_wav_audio_buf == NULL)
    {
        err = malloc_the_camera_take_a_pic_01_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_the_camera_take_a_pic_01_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.the_camera_take_a_pic_01_wav_audio_buf, audio_buf.the_camera_take_a_pic_01_wav_len, audio_volume, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(the_camera_take_a_pic_01_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_the_camera_take_a_pic_01();
        }
    }

    if (audio_buf.look_at_your_question_01_wav_audio_buf == NULL)
    {
        err = malloc_look_at_your_question_01_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_look_at_your_question_01_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.look_at_your_question_01_wav_audio_buf, audio_buf.look_at_your_question_01_wav_len, audio_volume, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(look_at_your_question_01_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_look_at_your_question_01();
        }
    }

    if (audio_buf.to_conserve_battery_01_wav_audio_buf == NULL)
    {
        err = malloc_to_conserve_battery_01_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_to_conserve_battery_01_wav() err: %s", esp_err_to_name(err));
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.to_conserve_battery_01_wav_audio_buf, audio_buf.to_conserve_battery_01_wav_len, audio_volume, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file(look_at_your_question_01_wav_audio_buf) err: %s", esp_err_to_name(err));
            }

            free_to_conserve_battery_01();
        }
    }

    return ESP_OK;
}

esp_err_t playback_audio_file(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable)
{
    esp_err_t err;
    int padding = 0;

    // wait until the previous audio file has completed
    while (audio_buf.system_i2s_playing)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    temp_buf = malloc(audio_file_len * sizeof(int16_t));

    // pad short audio files
    if (audio_file_len < 32024)
    {
        padding = 32024 - audio_file_len;
        queued_audio_file_len = audio_file_len + padding;
    }
    else
    {
        queued_audio_file_len = audio_file_len;
    }

    queued_audio_file_stoppable = audio_playback_stoppable;

    /*
    uint8_t audio bytes
    int ii = 0;
    for (int i = 0; i < audio_file_len; i += 2)
    {
        temp_buf[ii++] = (int16_t)(audio_file_buf[i + 1] << 8 | audio_file_buf[i]) * audio_volume;
    }
    */

    for (int i = 0; i <= audio_file_len / sizeof(int16_t); i++)
    {
        // removes the pop from the wav file bytes
        if (i <= 2000)
        {
            temp_buf[i] = 0x00;
        }
        else
        {
            temp_buf[i] = audio_file_buf[i] * audio_volume;
        }
    }

    err = i2s_start(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_start() err %s", esp_err_to_name(err));
        return err;
    }

    audio_buf.system_i2s_playing = true;
    audio_buf.system_i2s_stop = false;

    ESP_LOGI(TAG, "i2s_write() starting...");

    size_t bytes_written = 0;
    err = i2s_write(I2S_NUM_1, temp_buf, audio_file_len, &bytes_written, 10);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_write() err %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "i2s_write() complete");

    return err;
}

/*
esp_err_t playback_audio_file_2(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable)
{
    esp_err_t err;

    // wait until the previous audio file has completed
    while (audio_buf.system_i2s_playing)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // if the audio file length is less than 64048 it needs additional padding
    if (audio_file_len < 84048)
    {
        const int padding = 84048 - audio_file_len;
        const int audio_file_len_padded = audio_file_len + padding;

        temp_buf = malloc(audio_file_len_padded * sizeof(int16_t));
        queued_audio_file_len = audio_file_len_padded;
        queued_audio_file_stoppable = audio_playback_stoppable;

        for (int i = 0; i <= audio_file_len_padded / sizeof(int16_t); i++)
        {
            // removes the pop from the beginning of the wav file bytes
            if (i <= 1000)
            {
                temp_buf[i] = 0x00;
            }
            else
            {
                if (i <= audio_file_len)
                {
                    temp_buf[i] = audio_file_buf[i] * audio_volume;
                }
                else
                {
                    temp_buf[i] = 0;
                }
            }
        }

        audio_file_len += padding;
    }
    else
    {
        temp_buf = malloc(audio_file_len * sizeof(int16_t));
        queued_audio_file_len = audio_file_len;
        queued_audio_file_stoppable = audio_playback_stoppable;

        for (int i = 0; i <= audio_file_len / sizeof(int16_t); i++)
        {
            // removes the pop from the beginning of the wav file bytes
            if (i <= 1000)
            {
                temp_buf[i] = 0x00;
            }
            else
            {
                temp_buf[i] = audio_file_buf[i] * audio_volume;
            }
        }
    }

    err = i2s_start(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_start() err %s", esp_err_to_name(err));
        return err;
    }

    audio_buf.system_i2s_playing = true;
    audio_buf.system_i2s_stop = false;

    // vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "i2s_write() starting...");

    size_t bytes_written = 0;
    err = i2s_write(I2S_NUM_1, temp_buf, audio_file_len, &bytes_written, 10);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_write() err %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "i2s_write() complete");

    return err;
}
*/

esp_err_t playback_error_message(void)
{
    esp_err_t err = ESP_OK;

    // playback error message
    if (audio_buf.error_message_00_wav_audio_buf == NULL)
    {
        err = malloc_error_message_00_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_error_message_00_wav() err: %s", esp_err_to_name(err));
        }
    }
    if (err == ESP_OK)
    {
        err = playback_audio_file(audio_buf.error_message_00_wav_audio_buf, audio_buf.error_message_00_wav_audio_len, audio_volume, false);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "playback_audio_file(error_message_00_wav_audio_buf) err: %s", esp_err_to_name(err));
        }
    }

    free_error_message_00_wav();

    return err;
}

esp_err_t playback_confirm_sfx_04(bool skippable)
{
    esp_err_t err = ESP_OK;

    // playback error message
    if (audio_buf.confirm_sfx_04_wav_audio_buf == NULL)
    {
        err = malloc_confirm_sfx_04_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_confirm_sfx_04_wav() err: %s", esp_err_to_name(err));
        }
    }
    if (err == ESP_OK)
    {
        err = playback_audio_file(audio_buf.confirm_sfx_04_wav_audio_buf, audio_buf.confirm_sfx_04_wav_audio_len, audio_volume, skippable);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "playback_audio_file(confirm_sfx_04_wav_audio_buf) err: %s", esp_err_to_name(err));
        }
    }

    free_confirm_sfx_04_wav();

    return err;
}

esp_err_t playback_returning_home_wav(void)
{
    esp_err_t err = ESP_OK;

    if (audio_buf.returning_home_wav_audio_buf == NULL)
    {
        err = malloc_returning_home_wav_sfx();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_smart_glasses_home_01_wav() err: %s", esp_err_to_name(err));
        }
    }

    if (err == ESP_OK)
    {
        playback_audio_file(audio_buf.returning_home_wav_audio_buf, audio_buf.returning_home_wav_audio_len, audio_volume, false);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
        }
    }

    free_returning_home_wav_sfx();

    return err;
}

esp_err_t playback_completion_percentage(int file_size, float written_bytes, bool skippable)
{
    esp_err_t err = ESP_OK;

    if (written_bytes <= file_size * 0.1f)
    {
        if (audio_buf.ascii_5_00_wav_audio_buf == NULL)
        {
            err = malloc_ascii_5_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_ascii_5_00_wav() err: %s", esp_err_to_name(err));
            }
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf.ascii_5_00_wav_audio_buf, audio_buf.ascii_5_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_ascii_5_00_wav();
    }
    else if (written_bytes > file_size * 0.1f && written_bytes <= file_size * 0.2f)
    {
        if (audio_buf._10_00_wav_audio_buf == NULL)
        {
            err = malloc_10_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_10_00_wav() err: %s", esp_err_to_name(err));
            }
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._10_00_wav_audio_buf, audio_buf._10_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_10_00_wav();
    }
    else if (written_bytes > file_size * 0.2f && written_bytes <= file_size * 0.3f)
    {
        if (audio_buf._20_00_wav_audio_buf == NULL)
        {
            err = malloc_20_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_20_00_wav() err: %s", esp_err_to_name(err));
            }
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._20_00_wav_audio_buf, audio_buf._20_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_20_00_wav();
    }
    else if (written_bytes > file_size * 0.3f && written_bytes <= file_size * 0.4f)
    {
        if (audio_buf._30_00_wav_audio_buf == NULL)
        {
            err = malloc_30_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_30_00_wav() err: %s", esp_err_to_name(err));
            }
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._30_00_wav_audio_buf, audio_buf._30_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_30_00_wav();
    }
    else if (written_bytes > file_size * 0.4f && written_bytes <= file_size * 0.5f)
    {
        if (audio_buf._40_00_wav_audio_buf == NULL)
        {
            err = malloc_40_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_40_00_wav() err: %s", esp_err_to_name(err));
            }
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._40_00_wav_audio_buf, audio_buf._40_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_40_00_wav();
    }
    else if (written_bytes > file_size * 0.5f && written_bytes <= file_size * 0.6f)
    {
        if (audio_buf._50_00_wav_audio_buf == NULL)
        {
            err = malloc_50_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_50_00_wav() err: %s", esp_err_to_name(err));
            }
        }
        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._50_00_wav_audio_buf, audio_buf._50_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_50_00_wav();
    }
    else if (written_bytes > file_size * 0.6f && written_bytes <= file_size * 0.7f)
    {
        if (audio_buf._60_00_wav_audio_buf == NULL)
        {
            err = malloc_60_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_60_00_wav() err: %s", esp_err_to_name(err));
            }
        }
        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._60_00_wav_audio_buf, audio_buf._60_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_60_00_wav();
    }
    else if (written_bytes > file_size * 0.7f && written_bytes <= file_size * 0.8f)
    {
        if (audio_buf._70_00_wav_audio_buf == NULL)
        {
            err = malloc_70_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_70_00_wav() err: %s", esp_err_to_name(err));
            }
        }
        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._70_00_wav_audio_buf, audio_buf._70_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_70_00_wav();
    }
    else if (written_bytes > file_size * 0.8f && written_bytes <= file_size * 0.9f)
    {
        if (audio_buf._80_00_wav_audio_buf == NULL)
        {
            err = malloc_80_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_80_00_wav() err: %s", esp_err_to_name(err));
            }
        }

        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._80_00_wav_audio_buf, audio_buf._80_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_80_00_wav();
    }
    else if (written_bytes > file_size * 0.9f)
    {
        if (audio_buf._90_00_wav_audio_buf == NULL)
        {
            err = malloc_90_00_wav();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "malloc_90_00_wav() err: %s", esp_err_to_name(err));
            }
        }
        if (err == ESP_OK)
        {
            playback_audio_file(audio_buf._90_00_wav_audio_buf, audio_buf._90_00_wav_audio_len, audio_volume, skippable);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
            }
        }

        free_90_00_wav();
    }

    if (audio_buf.percent_completed_00_wav_audio_buf == NULL)
    {
        err = malloc_percent_completed_00_wav();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "malloc_percent_completed_00_wav() err: %s", esp_err_to_name(err));
        }
    }

    if (err == ESP_OK)
    {
        err = playback_audio_file(audio_buf.percent_completed_00_wav_audio_buf, audio_buf.percent_completed_00_wav_audio_len, audio_volume, skippable);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "playback_audio_file(percent_completed_00_wav_audio_buf) err: %s", esp_err_to_name(err));
        }
    }

    free_percent_completed_00_wav();

    return err;
}

esp_err_t _i2s_stop(void)
{
    return i2s_stop(I2S_NUM_1);
}

static void i2sTask(void *pvParameters)
{
    i2s_event_t event;
    int txEvents = 0;

    while (true)
    {
        // Wait indefinitely for a new message in the queue
        if (xQueueReceive(i2s_event_queue, &event, portMAX_DELAY) == pdTRUE)
        {

            /*
            if (event.type == I2S_EVENT_TX_DONE && glasses_state == GLASSES_HFP)
            {
                if (!audio_buf.hfp_i2s_ready)
                {
                    i2s_zero_dma_buffer(I2S_NUM_1);
                    i2s_stop(I2S_NUM_1);
                }
            }
            else if (event.type == I2S_EVENT_TX_DONE)
            {
                if (++txEvents >= (queued_audio_file_len / 2048) + 8)
                {
                    i2s_zero_dma_buffer(I2S_NUM_1);
                    i2s_stop(I2S_NUM_1);

                    free(temp_buf);
                    temp_buf = NULL;

                    txEvents = 0;
                }
                printf("txEvents: %d\n", txEvents);
            }
            */
            if (event.type == I2S_EVENT_TX_DONE)
            {
                if (audio_buf.system_i2s_playing)
                {
                    if (++txEvents >= (queued_audio_file_len / 2048))
                    {
                        i2s_zero_dma_buffer(I2S_NUM_1);
                        i2s_stop(I2S_NUM_1);

                        free(temp_buf);
                        temp_buf = NULL;

                        txEvents = 0;

                        audio_buf.system_i2s_playing = false;
                    }
                    printf("txEvents: %d\n", txEvents);

                    if (audio_buf.system_i2s_stop && queued_audio_file_stoppable)
                    {
                        i2s_zero_dma_buffer(I2S_NUM_1);
                        i2s_stop(I2S_NUM_1);

                        free(temp_buf);
                        temp_buf = NULL;

                        txEvents = 0;

                        audio_buf.system_i2s_playing = false;
                        audio_buf.system_i2s_stop = false;
                    }
                }

                // else if (audio_buf.hfp_i2s_playing)
                // {
                //     if (audio_buf.hfp_i2s_stop)
                //     {
                //         i2s_zero_dma_buffer(I2S_NUM_1);
                //         i2s_stop(I2S_NUM_1);

                //         audio_buf.hfp_i2s_stop = false;
                //         audio_buf.hfp_i2s_playing = false;
                //     }
                // }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

esp_err_t init_i2s(void)
{
    esp_err_t err;
    BaseType_t task_err;

    const i2s_config_t i2s_config_1 = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, // I2S_COMM_FORMAT_STAND_MSB <- big-endian
        .intr_alloc_flags = ESP_INTR_FLAG_INTRDISABLED,    // ESP_INTR_FLAG_INTRDISABLED //ESP_INTR_FLAG_LEVEL1
        .dma_buf_count = 8,                                // number of buffers
        .dma_buf_len = 1024                                /*,    //64
        .use_apll = false,
        .tx_desc_auto_clear = false*/
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO};

    // Configuring the I2S driver and pins.
    // This function must be called before any I2S driver read/write operations.
    err = i2s_driver_install(I2S_NUM_1, &i2s_config_1, 1, &i2s_event_queue);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_driver_install() err: %s", esp_err_to_name(err));
        return err;
    }
    err = i2s_set_pin(I2S_NUM_1, &pin_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_set_pin() err: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_set_clk(I2S_NUM_1, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_set_pin() err: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_set_sample_rates(I2S_NUM_1, 16000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_set_pin() err: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_stop(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_stop() err: %s", esp_err_to_name(err));
        return err;
    }

    audio_buf.system_i2s_playing = false;
    audio_buf.system_i2s_stop = false;
    // audio_buf.hfp_i2s_playing = false;
    // audio_buf.hfp_i2s_stop = true;

    task_err = xTaskCreatePinnedToCore(
        i2sTask,
        "i2sTask",
        10000, // configMINIMAL_STACK_SIZE
        NULL,
        5,
        NULL,
        1);
    if (task_err != pdPASS)
    {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore(i2sTask) err: %d", task_err);

        // attempt to create the home_button with xTaskCreate
        task_err = xTaskCreate(i2sTask, "i2sTask", 20000, NULL, 10, NULL);
        if (task_err != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate(i2sTask) err: %d", task_err);
            return ESP_FAIL;
        }
    }

    return err;
}