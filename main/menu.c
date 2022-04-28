#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "touch.h"
#include "audio_io.h"
#include "state_machine.h"

static const char *TAG = "menu.c";

esp_err_t browse_menu(void)
{
    esp_err_t err;

    if (state_machine == TUTORFISH_HOME)
    {
        switch (menu_selection_idx)
        {
        case 0:

            err = playback_audio_file(audio_buf.tutor_fish_settings_00_wav_audio_buf, audio_buf.tutor_fish_settings_00_wav_len, 0.2f, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
                return err;
            }

            ESP_LOGI(TAG, "browse_menu(): Tutorfish Settings");
            break;
        case 1:

            err = playback_audio_file(audio_buf.submit_a_question_00_wav_audio_buf, audio_buf.submit_a_question_00_wav_len, 0.2f, true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "playback_audio_file() err: %s", esp_err_to_name(err));
                return err;
            }

            ESP_LOGI(TAG, "browse_menu(): Submit Question");
            break;
        default:
            break;
        }
    }

    return ESP_OK;
}

esp_err_t menu_selection(void)
{
    esp_err_t err;

    if (state_machine == TUTORFISH_HOME)
    {
        switch (menu_selection_idx)
        {
        case 0:
            ESP_LOGI(TAG, "menu_selection(): Tutorfish Settings");
            state_machine = TUTORFISH_SETTINGS;
            break;
        case 1:
            ESP_LOGI(TAG, "menu_selection(): Submit Question");
            state_machine = TUTORFISH_SUBMIT_QUESTION;
            break;
        default:
            break;
        }
    }

    return ESP_OK;
}