#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    uint16_t audio_format;    // 1 for uncompressed PCM
    uint16_t num_channels;    // 1 for Mono, 2 for Stereo
    uint32_t sample_rate;     // 44100 Hz
    uint32_t byte_rate;       // Bytes of audio data per second, across 2 channels
    uint16_t block_align;     // Bytes per sample across all channels
    uint16_t bits_per_sample; // 16
    const uint8_t *data;      // Point to first PCM data byte
    uint32_t data_size;       // Number of PCM bytes
} wav_info_t;

/**
 * @brief Parse the WAV file header and get audio formatting parameters.
 * 
 * Checks for valid file headers ("RIFF", "WAVE", "fmt ") and if the file is in
 * uncompressed PCM format then assigns a pointer to the beginning of the audio
 * samples.
 * 
 * @param[in]  raw_file Pointer to the start of the WAV file in flash memory.
 * @param[out] out_info Pointer to a wav_info_t struct where data will be stored.
 * 
 * @retval true  File is a valid PCM WAV and parsing data to out_info was successful.
 * @retval false Invalid file headers or not a .wav file.
 */
bool parse_wav(const uint8_t *raw_file, wav_info_t *out_info);

/**
 * @brief Duplicate mono audio to both LR channels to create stereo audio and
 *        transmit to I2S peripherals.
 * 
 * We are assuming the audio file in question is mono, not stereo. Reads the samples
 * in flash memory and duplicates them into a buffer array. Use this after successfully
 * calling parse_wav().
 * 
 * @param[in] in_info   Pointer to a filled wav_info_t struct, including the data    
 *                      pointer and metadata.
 * @param[in] tx_handle The initialized I2S transmission channel handle.
 */
void play_wav(wav_info_t *in_info, i2s_chan_handle_t tx_handle);