#pragma once
// =============================================================================
//  motors.h — open/close the 9 pig doors. Doors are numbered 0-8 in code
//  (the laptop shows them as 1-9). Pin numbers are in pins.h.
// =============================================================================

void motors_init(void);              // stop every motor
void motor_open(int door);           // start opening (call motor_stop after MOTOR_MOVE_MS)
void motor_close(int door);          // start closing
void motor_stop(int door);
const char *motor_name(int door);    // e.g. "M3"
