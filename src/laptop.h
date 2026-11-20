#pragma once
// =============================================================================
//  laptop.h — reads typed commands from the laptop over USB (the board's
//  "UART" port) and passes them to the game task.
//
//  Commands (not case-sensitive):
//    START <university>   start a game, e.g. START UBC
//    STOP                 end the current game
//    HIT                  TEST: pretend a ball went in
//    DOOR <1-9>           TEST: open and close one door
//    HELP                 show this list
// =============================================================================

void laptop_task(void *arg);
