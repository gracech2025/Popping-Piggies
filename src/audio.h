#pragma once
// =============================================================================
//  audio.h — sound effects. Sounds play in their own FreeRTOS task, so asking
//  for a sound returns immediately and the game never waits for audio.
// =============================================================================

typedef enum {
    SOUND_COUNTDOWN,
    SOUND_GAME_START,
    SOUND_HIT,
    SOUND_MISS,
    SOUND_GAME_OVER,
    SOUND_NEW_HIGH_SCORE,
    SOUND_COUNT          // keep last: number of sounds
} sound_t;

void audio_init(void);          // start the audio task
void audio_play(sound_t sound); // queue a sound (dropped if 4 are already waiting)
