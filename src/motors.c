// Door motors (DRV8833 drivers, controlled through the pin expanders).
// From the motor_actuation branch: drive_motor() and stop_motor().
#include "motors.h"
#include "pins.h"
#include "expander.h"

static const door_pins_t doors[NUM_DOORS] = DOOR_PINS_TABLE;

static void set_pins(int door, bool in1, bool in2)
{
    if (door < 0 || door >= NUM_DOORS) return;
    expander_write_pin(doors[door].expander, doors[door].in1, in1);
    expander_write_pin(doors[door].expander, doors[door].in2, in2);
}

void motors_init(void)
{
    for (int d = 0; d < NUM_DOORS; d++) {
        motor_stop(d);
    }
}

void motor_open(int door)  { set_pins(door, true, false); }
void motor_close(int door) { set_pins(door, false, true); }
void motor_stop(int door)  { set_pins(door, false, false); }

const char *motor_name(int door)
{
    if (door < 0 || door >= NUM_DOORS) return "?";
    return doors[door].name;
}
