#pragma once
// =============================================================================
//  game.h — the game itself (runs in its own FreeRTOS task).
//  Other tasks (like the laptop task) control it by sending commands into its
//  queue — they never call motor/beam code directly.
// =============================================================================

#include <stdbool.h>
#include "config.h"

typedef enum {
    CMD_START,      // start a game for `school`
    CMD_STOP,       // end the current game early
    CMD_HIT,        // TEST: pretend a ball broke a beam
    CMD_DOOR_TEST,  // TEST: open and close one door (`door` = 0-8)
} game_cmd_type_t;

typedef struct {
    game_cmd_type_t type;
    int door;
    char school[SCHOOL_NAME_MAX];
} game_cmd_t;

void game_init(void);                           // create the command queue
bool game_send_command(const game_cmd_t *cmd);  // false if the queue is full
void game_task(void *arg);                      // the task function
