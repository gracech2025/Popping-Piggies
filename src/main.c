#include <stdio.h>
#include "wav_player.h"

void app_main(void) {
    wav_info_t wav_data;
    i2s_chan_handle_t tx_handle;

    // Must follow naming convention: _binary_[file_name]_wav_start
    extern const uint8_t _binary_test_wav_start[];

    // Safety check
    if (parse_wav(_binary_test_wav_start, &wav_data) == false) {
        return;
    }

    i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&channel_config, &tx_handle, NULL);

    // Configure hardware
    i2s_std_config_t std_config = {
        // Clock settings
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(wav_data.sample_rate),

        // GPIO Pins
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .din = I2S_GPIO_UNUSED,
            // Change the pins here as needed
            .bclk = GPIO_NUM_1,
            .ws = GPIO_NUM_2,
            .dout = GPIO_NUM_3
        },

        // Stereo format
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)
    };

    i2s_channel_init_std_mode(tx_handle, &std_config);
    i2s_channel_enable(tx_handle);
    play_wav(&wav_data, tx_handle);

    // Free memory
    i2s_channel_disable(tx_handle);
    i2s_del_channel(tx_handle);
}