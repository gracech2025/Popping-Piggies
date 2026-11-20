#pragma once
// =============================================================================
//  beams.h — the IR break-beam sensors that detect a ball going in.
// =============================================================================

#include <stdint.h>

// Returns which beams are broken right now: one bit per expander pin
// (0 = nothing broken). Pins are set in pins.h (BEAM_PIN_MASK).
uint16_t beams_read(void);
