#pragma once
// =============================================================================
//  pins.h — EVERY pin number in the project lives in this one file.
//
//  !! PLACEHOLDERS !!
//  The schematic's ESP32 symbol does not match the real DevKitC board (see the
//  note on the schematic), so the ESP32 GPIO numbers below are best guesses.
//  The hardware lead will assign the real numbers — when they arrive, only
//  this file needs to change.
// =============================================================================

#include <stdint.h>

// -----------------------------------------------------------------------------
//  ESP32 GPIO pins
// -----------------------------------------------------------------------------

// Pin expander 1 (MCP23017, I2C address 0x20) — on its own I2C bus
#define EXP1_I2C_PORT   0
#define EXP1_SCL_GPIO   1
#define EXP1_SDA_GPIO   2
#define EXP1_ADDR       0x20

// Pin expander 2 (MCP23017, I2C address 0x21, because its A0 pin is tied high)
// NOTE: GPIO19/20 are also the ESP32-S3's native USB pins. Use the board's
// "UART" USB port (not the "USB" one) for the laptop while these are used here.
#define EXP2_I2C_PORT   1
#define EXP2_SCL_GPIO   20
#define EXP2_SDA_GPIO   19
#define EXP2_ADDR       0x21

// Speaker: ESP32 -> PCM5102A DAC (I2S) -> TPA3110 amplifier -> speakers
#define AUDIO_BCK_GPIO  38   // bit clock          (DAC pin "BCK")
#define AUDIO_LCK_GPIO  48   // left/right clock   (DAC pin "LCK", a.k.a. WS)
#define AUDIO_DIN_GPIO  47   // audio data         (DAC pin "DIN")

// HUB75 LED display pins: added when the LED display branch is merged.

// -----------------------------------------------------------------------------
//  Pin expander pins
//  Each expander has 16 pins, numbered 0-15 here:
//    0-7  = GPA0-GPA7
//    8-15 = GPB0-GPB7
// -----------------------------------------------------------------------------

#define EXP1  0   // which expander (used in the tables below)
#define EXP2  1
#define NUM_EXPANDERS 2

// ---- Doors (motors) ----
// Each door motor is driven by 2 pins on a DRV8833 motor driver (IN1 and IN2):
//   IN1=1, IN2=0 -> open     IN1=0, IN2=1 -> close     both 0 -> stop
// If a door moves the wrong way, swap its IN1 and IN2 numbers below.
typedef struct {
    const char *name;   // motor label on the schematic
    uint8_t expander;   // EXP1 or EXP2
    uint8_t in1;        // expander pin 0-15
    uint8_t in2;        // expander pin 0-15
} door_pins_t;

#define NUM_DOORS 9

// Taken from the schematic (pin expander side). There is no M9 on the schematic.
// WARNING (hardware): M10's driver is controlled through IN3/IN4, but M10 is
// wired to OUT1/OUT2, so M10 won't move until that wiring is fixed.
#define DOOR_PINS_TABLE {          \
    /* name   expander  IN1  IN2 */ \
    { "M1",   EXP1,      5,   6 },  /* GPA5, GPA6 */ \
    { "M2",   EXP1,      7,   8 },  /* GPA7, GPB0 */ \
    { "M3",   EXP1,     11,  12 },  /* GPB3, GPB4 */ \
    { "M4",   EXP1,      9,  10 },  /* GPB1, GPB2 */ \
    { "M5",   EXP2,      2,   3 },  /* GPA2, GPA3 */ \
    { "M6",   EXP2,      0,   1 },  /* GPA0, GPA1 */ \
    { "M7",   EXP2,      6,   7 },  /* GPA6, GPA7 */ \
    { "M8",   EXP2,      4,   5 },  /* GPA4, GPA5 */ \
    { "M10",  EXP1,     13,  14 },  /* GPB5, GPB6 */ \
}

// ---- Break beams ----
// 5 IR break-beam receivers on expander 1, GPA0-GPA4 (BB_RX1-BB_RX5).
// A receiver pin reads 1 normally and 0 when the beam is broken.
// Only one door is open at a time, so ANY broken beam counts as a hit.
#define NUM_BEAMS      5
#define BEAM_EXPANDER  EXP1
#define BEAM_PIN_MASK  0x001F   // bits 0-4 = GPA0-GPA4

// ---- LED strips (not used by the code yet) ----
//   Expander 1: pin 15 (GPB7) = LS9
//   Expander 2: pins 8-15 (GPB0-GPB7) = LS8, LS10, LS11, LS12, LS13, LS14, LS16, LS15

// ---- Which expander pins are inputs ----
// 1 = input, 0 = output. Everything that isn't a beam is an output (starts LOW = off).
#define EXP1_INPUT_MASK  BEAM_PIN_MASK
#define EXP2_INPUT_MASK  0x0000
