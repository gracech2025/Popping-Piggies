// Audio task: waits for sound requests and plays them through I2S.
// From the speaker branch: the I2S setup in its app_main(), plus
// parse_wav()/play_wav() from wav_player.c (unchanged).
#include "audio.h"
#include "pins.h"
#include "wav_player.h"

#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "audio";

// Sound files are built into the firmware (see EMBED_FILES in src/CMakeLists.txt).
// The name always follows the pattern _binary_<file>_<ext>_start.
// Files must be 16-bit, mono, uncompressed (PCM) WAV.
extern const uint8_t _binary_test_wav_start[];

// Which file each sound uses. Only test.wav exists so far; when the real
// sounds are added, embed them in CMakeLists.txt and point to them here.
static const uint8_t *const sound_files[SOUND_COUNT] = {
    [SOUND_COUNTDOWN]      = _binary_test_wav_start,
    [SOUND_GAME_START]     = _binary_test_wav_start,
    [SOUND_HIT]            = _binary_test_wav_start,
    [SOUND_MISS]           = _binary_test_wav_start,
    [SOUND_GAME_OVER]      = _binary_test_wav_start,
    [SOUND_NEW_HIGH_SCORE] = _binary_test_wav_start,
};

static QueueHandle_t audio_queue;

static void audio_task(void *arg)
{
    // Set up I2S once, then keep it running between sounds.
    i2s_chan_handle_t tx;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;   // send silence (not the last sound) when idle
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx, NULL));

    uint32_t current_rate = 44100;
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AUDIO_BCK_GPIO,
            .ws   = AUDIO_LCK_GPIO,
            .dout = AUDIO_DIN_GPIO,
            .din  = I2S_GPIO_UNUSED,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx));

    for (;;) {
        sound_t sound;
        xQueueReceive(audio_queue, &sound, portMAX_DELAY);   // sleep until asked

        wav_info_t wav;
        if (sound >= SOUND_COUNT || !parse_wav(sound_files[sound], &wav)) {
            ESP_LOGW(TAG, "Sound %d is not a valid WAV file", (int)sound);
            continue;
        }
        if (wav.num_channels != 1 || wav.bits_per_sample != 16) {
            ESP_LOGW(TAG, "Sound %d must be 16-bit mono", (int)sound);
            continue;
        }

        // Different files can have different sample rates: retune the clock if needed.
        if (wav.sample_rate != current_rate) {
            i2s_channel_disable(tx);
            i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(wav.sample_rate);
            i2s_channel_reconfig_std_clock(tx, &clk_cfg);
            i2s_channel_enable(tx);
            current_rate = wav.sample_rate;
        }

        play_wav(&wav, tx);   // blocks this task (not the game) until the sound ends
    }
}

void audio_init(void)
{
    audio_queue = xQueueCreate(4, sizeof(sound_t));
    xTaskCreate(audio_task, "audio", 4096, NULL, 5, NULL);
}

void audio_play(sound_t sound)
{
    xQueueSend(audio_queue, &sound, 0);   // 0 = don't wait if the queue is full
}
