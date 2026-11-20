#pragma once
// =============================================================================
//  display.h — what the scoreboard should show.
//
//  TEMPORARY: for now these just print to the laptop. When the LED display
//  branch is merged, display.c will send these to the HUB75 panel instead,
//  and the game code won't need to change.
// =============================================================================

#include <stdbool.h>
#include <stdint.h>

void display_show_idle(uint32_t top_score, const char *top_school);
void display_show_countdown(int seconds_left);
void display_show_game(const char *school, uint32_t score, int doors_left);
void display_show_end(const char *school, uint32_t score, int hits,
                      uint32_t top_score, bool new_record);
