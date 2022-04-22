#include <string.h>
#include "driver/i2s.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "audio_io.c";

#define I2S_BCK_IO (2)
#define I2S_WS_IO (19) // MTMS (GPIO14) when using pcb -> GPIO19 breakout
#define I2S_DO_IO (5)
#define I2S_DI_IO (36)

static QueueHandle_t i2s_event_queue;
static int queued_audio_file_len = 0;

static int16_t *temp_buf = NULL;
static bool queued_audio_file_stoppable;

audio_buf_t audio_buf;

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

esp_err_t malloc_open_voice_memo_00_wav(void)
{
    const char *audio_file_path = "/audio/open_voice_memo_00.wav";

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
    audio_buf.open_voice_memo_00_wav_audio_len = ftell(f);
    audio_buf.open_voice_memo_00_wav_audio_buf = malloc(audio_buf.open_voice_memo_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.open_voice_memo_00_wav_audio_buf, sizeof(uint8_t), audio_buf.open_voice_memo_00_wav_audio_len, f);
    if (read_ret != audio_buf.open_voice_memo_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.open_voice_memo_00_wav_audio_len);
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

esp_err_t malloc_open_settings_00_wav(void)
{
    const char *audio_file_path = "/audio/open_settings_00.wav";

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
    audio_buf.open_settings_00_wav_audio_len = ftell(f);
    audio_buf.open_settings_00_wav_audio_buf = malloc(audio_buf.open_settings_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.open_settings_00_wav_audio_buf, sizeof(uint8_t), audio_buf.open_settings_00_wav_audio_len, f);
    if (read_ret != audio_buf.open_settings_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.open_settings_00_wav_audio_len);
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

esp_err_t malloc_open_camera_00_wav(void)
{
    const char *audio_file_path = "/audio/open_camera_00.wav";

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
    audio_buf.open_camera_00_wav_audio_len = ftell(f);
    audio_buf.open_camera_00_wav_audio_buf = malloc(audio_buf.open_camera_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.open_camera_00_wav_audio_buf, sizeof(uint8_t), audio_buf.open_camera_00_wav_audio_len, f);
    if (read_ret != audio_buf.open_camera_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.open_camera_00_wav_audio_len);
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

esp_err_t malloc_open_app_store_00_wav(void)
{
    const char *audio_file_path = "/audio/open_app_store_00.wav";

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
    audio_buf.open_app_store_00_wav_audio_len = ftell(f);
    audio_buf.open_app_store_00_wav_audio_buf = malloc(audio_buf.open_app_store_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.open_app_store_00_wav_audio_buf, sizeof(uint8_t), audio_buf.open_app_store_00_wav_audio_len, f);
    if (read_ret != audio_buf.open_app_store_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.open_app_store_00_wav_audio_len);
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

esp_err_t malloc_start_phone_call_mode_00_wav(void)
{
    const char *audio_file_path = "/audio/start_phone_call_mode_00.wav";

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
    audio_buf.start_phone_call_mode_00_wav_audio_len = ftell(f);
    audio_buf.start_phone_call_mode_00_wav_audio_buf = malloc(audio_buf.start_phone_call_mode_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.start_phone_call_mode_00_wav_audio_buf, sizeof(uint8_t), audio_buf.start_phone_call_mode_00_wav_audio_len, f);
    if (read_ret != audio_buf.start_phone_call_mode_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.start_phone_call_mode_00_wav_audio_len);
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

esp_err_t malloc_starting_phone_call_mode_00_wav(void)
{
    const char *audio_file_path = "/audio/starting_phone_call_mode_00.wav";

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
    audio_buf.starting_phone_call_mode_00_wav_audio_len = ftell(f);
    audio_buf.starting_phone_call_mode_00_wav_audio_buf = malloc(audio_buf.starting_phone_call_mode_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.starting_phone_call_mode_00_wav_audio_buf, sizeof(uint8_t), audio_buf.starting_phone_call_mode_00_wav_audio_len, f);
    if (read_ret != audio_buf.starting_phone_call_mode_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.starting_phone_call_mode_00_wav_audio_len);
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

esp_err_t malloc_attempt_wifi_conn_00_wav()
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

esp_err_t malloc_start_wifi_conn_00_wav(void)
{
    const char *audio_file_path = "/audio/start_wifi_conn_00.wav";

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
    audio_buf.start_wifi_conn_00_wav_audio_len = ftell(f);
    audio_buf.start_wifi_conn_00_wav_audio_buf = malloc(audio_buf.start_wifi_conn_00_wav_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.start_wifi_conn_00_wav_audio_buf, sizeof(uint8_t), audio_buf.start_wifi_conn_00_wav_audio_len, f);
    if (read_ret != audio_buf.start_wifi_conn_00_wav_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.start_wifi_conn_00_wav_audio_len);
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

esp_err_t malloc_home_instructions_00_wav()
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

esp_err_t malloc_returning_home_wav_sfx()
{
    const char *audio_file_path = "/audio/returning_home.wav";

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

esp_err_t malloc_home_sfx()
{
    const char *audio_file_path = "/audio/home_sfx_00.wav";

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
    audio_buf.home_sfx_audio_len = ftell(f);
    audio_buf.home_sfx_audio_buf = malloc(audio_buf.home_sfx_audio_len * sizeof(int16_t));

    // Moving pointer to beginning
    f_err = fseek(f, 0, SEEK_SET);
    if (f_err != 0)
    {
        ESP_LOGE(TAG, "fseek() SEEK_SET error");
        return ESP_FAIL;
    }

    int read_ret = fread(audio_buf.home_sfx_audio_buf, sizeof(uint8_t), audio_buf.home_sfx_audio_len, f);
    if (read_ret != audio_buf.home_sfx_audio_len)
    {
        ESP_LOGE(TAG, "fread() error. read %d bytes out of %d", read_ret, audio_buf.home_sfx_audio_len);
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

esp_err_t playback_audio_file(int16_t *audio_file_buf, int audio_file_len, float audio_volume, bool audio_playback_stoppable)
{
    esp_err_t err;

    // wait until the previous audio file has completed
    while (audio_buf.system_i2s_playing)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    temp_buf = malloc(audio_file_len * sizeof(int16_t));
    queued_audio_file_len = audio_file_len;
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
        temp_buf[i] = audio_file_buf[i] * audio_volume;
    }

    err = i2s_start(I2S_NUM_1);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "i2s_start() err %s", esp_err_to_name(err));
        return err;
    }

    audio_buf.system_i2s_playing = true;
    audio_buf.system_i2s_stop = false;

    //vTaskDelay(500 / portTICK_PERIOD_MS);

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

                else if (audio_buf.hfp_i2s_playing)
                {
                    if (audio_buf.hfp_i2s_stop)
                    {
                        i2s_zero_dma_buffer(I2S_NUM_1);
                        i2s_stop(I2S_NUM_1);

                        audio_buf.hfp_i2s_stop = false;
                        audio_buf.hfp_i2s_playing = false;
                    }
                }
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
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, //I2S_COMM_FORMAT_STAND_MSB <- big-endian
        .intr_alloc_flags = ESP_INTR_FLAG_INTRDISABLED,    //ESP_INTR_FLAG_INTRDISABLED //ESP_INTR_FLAG_LEVEL1
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
    audio_buf.hfp_i2s_playing = false;
    audio_buf.hfp_i2s_stop = true;

    task_err = xTaskCreatePinnedToCore(
        i2sTask,
        "i2sTask",
        10000, //configMINIMAL_STACK_SIZE
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