#pragma once
// =============================================================================
//  config.h — game rules and timings. Change numbers here to tune the game.
//  (Agreed in the meeting with the lead; all are provisional.)
// =============================================================================

// How many doors open in one game. Every door opens at least once.
#define DOORS_PER_GAME      15

// Scoring: time from the door-open signal until a beam breaks.
#define TIER_FAST_MS        2000    // under 2 s ...
#define POINTS_FAST         100     // ... = 100 points
#define TIER_MEDIUM_MS      4000    // 2-4 s ...
#define POINTS_MEDIUM       50      // ... = 50 points
#define POINTS_SLOW         30      // after 4 s, until the door closes = 30 points

// How long a door stays open before it closes and counts as a miss.
// PLACEHOLDER: not decided yet. Must be longer than TIER_MEDIUM_MS for the
// 30-point tier to ever happen.
#define DOOR_OPEN_MS        6000

// How long the motor is powered to fully open or close a door.
// PLACEHOLDER: depends on the mechanism; tune once a door is built.
#define MOTOR_MOVE_MS       500

// Random pause between one door closing and the next one opening.
#define GAP_MIN_MS          1000
#define GAP_MAX_MS          2500

// "3, 2, 1" countdown before the first door opens.
#define COUNTDOWN_SECONDS   3

// How often the break beams are checked while a door is open.
#define BEAM_POLL_MS        10

// University name typed on the laptop ("START UBC").
#define SCHOOL_NAME_MAX     32
#define DEFAULT_SCHOOL      "UBC"   // used if START is typed with no name
