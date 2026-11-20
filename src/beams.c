// Break-beam sensors.
// From the motor_actuation branch: the get_mcp_pin(...) == 0 check in its loop.
#include "beams.h"
#include "pins.h"
#include "expander.h"

uint16_t beams_read(void)
{
    uint16_t pins = expander_read_all(BEAM_EXPANDER);
    // Receivers pull their pin LOW when the beam is broken, so flip the bits,
    // then keep only the beam pins.
    return (uint16_t)(~pins) & BEAM_PIN_MASK;
}
