// TEMPORARY stand-in for the LED scoreboard: prints to the laptop instead.
// Replaced by the real HUB75/LVGL code when feature/led-display is merged.
#include "display.h"
#include "config.h"
#include <stdio.h>

void display_show_idle(uint32_t top_score, const char *top_school)
{
    if (top_score > 0) {
        printf("[DISPLAY] Ready! Top score: %lu (%s)\n", (unsigned long)top_score, top_school);
    } else {
        printf("[DISPLAY] Ready! No top score yet.\n");
    }
}

void display_show_countdown(int seconds_left)
{
    printf("[DISPLAY] %d...\n", seconds_left);
}

void display_show_game(const char *school, uint32_t score, int doors_left)
{
    printf("[DISPLAY] %s | score %lu | %d of %d doors left\n",
           school, (unsigned long)score, doors_left, DOORS_PER_GAME);
}

void display_show_end(const char *school, uint32_t score, int hits,
                      uint32_t top_score, bool new_record)
{
    printf("[DISPLAY] GAME OVER! %s scored %lu (%d/%d hits)%s Top score: %lu\n",
           school, (unsigned long)score, hits, DOORS_PER_GAME,
           new_record ? " NEW HIGH SCORE!" : "", (unsigned long)top_score);
}
