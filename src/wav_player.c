#include "wav_player.h"

bool parse_wav(const uint8_t *raw_file, wav_info_t *out_info) {
    // Check for "RIFF" heading (bytes 0-4)
    if (
        raw_file[0] != 'R' ||
        raw_file[1] != 'I' || 
        raw_file[2] != 'F' ||
        raw_file[3] != 'F'
    ) {
        return false;
    } 
    // Check for "WAVE" heading (bytes 9-12)
    if (
        raw_file[8] != 'W' ||
        raw_file[9] != 'A' ||
        raw_file[10] != 'V' ||
        raw_file[11] != 'E'
    ) {
        return false;
    }
    // Check for "fmt " heading (bytes 13-16)
    if (
        raw_file[12] != 'f' ||
        raw_file[13] != 'm' ||
        raw_file[14] != 't' ||
        raw_file[15] != ' '
    ) {
        return false;
    }

    // Map file information to out_info
    memcpy(&out_info->audio_format, &raw_file[20], 2);
    memcpy(&out_info->bits_per_sample, &raw_file[34], 2);
    memcpy(&out_info->block_align, &raw_file[32], 2);
    memcpy(&out_info->byte_rate, &raw_file[28], 4);
    memcpy(&out_info->num_channels, &raw_file[22], 2);
    memcpy(&out_info->sample_rate, &raw_file[24], 4);

    // Check valid PCM format
    if (out_info->audio_format != 1) {
        return false;
    }

    // Audio data extraction
    // Check for "data" heading (bytes 37-40)
    if (
        raw_file[36] != 'd' ||
        raw_file[37] != 'a' ||
        raw_file[38] != 't' ||
        raw_file[39] != 'a'
    ) {
        return false;
    }

    // Map audio data
    memcpy(&out_info->data_size, &raw_file[40], 4);
    out_info->data = &raw_file[44];

    return true;
}

void play_wav(wav_info_t *in_info, i2s_chan_handle_t tx_handle) {
    int16_t buffer[1024];
    const int16_t *data = (int16_t *) in_info->data;

    int total_samples = in_info->data_size / 2;
    int processed_samples = 0;

    while (processed_samples < total_samples) {
        // Duplicate mono audio channel to create stereo audio
        int i = 0;
        for (i = 0; i < 1024; i += 2) {
            if (processed_samples == total_samples) break;

            // Samples alternate between L and R
            buffer[i] = data[processed_samples];     // Left channel
            buffer[i + 1] = data[processed_samples]; // Right channel
            processed_samples++;
        }

        i2s_channel_write(tx_handle, buffer, i * sizeof(int16_t), NULL, portMAX_DELAY);
    }
}