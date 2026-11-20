#pragma once
// =============================================================================
//  expander.h — talks to the two MCP23017 pin expanders over I2C.
//  Motors, beams and LED strips are all wired to these chips, not directly to
//  the ESP32, so everything goes through these functions.
//
//  Only the game task should call these (one task owning the I2C buses means
//  two tasks can never talk over each other).
// =============================================================================

#include <stdbool.h>
#include <stdint.h>

// Set up both I2C buses and both expanders. If an expander isn't connected,
// it logs a warning and keeps going, so the game can still be tested from the
// laptop with no hardware attached.
void expanders_init(void);

// Set one output pin (0-15) on an expander (EXP1 or EXP2) high or low.
void expander_write_pin(int expander, int pin, bool high);

// Read all 16 pins of an expander at once (bit 0 = pin 0 ... bit 15 = pin 15).
// Returns 0xFFFF (all high = "no beam broken") if the expander isn't connected.
uint16_t expander_read_all(int expander);
