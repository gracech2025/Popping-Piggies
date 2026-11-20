// Laptop task: turns typed lines into game commands.
#include "laptop.h"
#include "game.h"
#include "config.h"
#include "pins.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "driver/uart.h"
#include "sdkconfig.h"

#define LAPTOP_UART    CONFIG_ESP_CONSOLE_UART_NUM   // same port the logs print on
#define MAX_LINE_LEN   64

static void print_help(void)
{
    printf("\nCommands:\n"
           "  START <university>   start a game (e.g. START UBC)\n"
           "  STOP                 end the current game\n"
           "  HIT                  test: pretend a ball went in\n"
           "  DOOR <1-%d>           test: open and close one door\n"
           "  HELP                 show this list\n\n", NUM_DOORS);
}

// Remove spaces from the start and end of a string.
static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

// True if `line` starts with the word `word` (followed by a space or the end).
static bool starts_with_word(const char *line, const char *word)
{
    size_t n = strlen(word);
    return strncasecmp(line, word, n) == 0 &&
           (line[n] == '\0' || isspace((unsigned char)line[n]));
}

static void handle_line(char *line)
{
    line = trim(line);
    if (*line == '\0') return;

    game_cmd_t cmd = { 0 };

    if (starts_with_word(line, "START")) {
        cmd.type = CMD_START;
        const char *name = trim(line + 5);
        if (*name == '\0') name = DEFAULT_SCHOOL;
        strlcpy(cmd.school, name, sizeof(cmd.school));
    } else if (strcasecmp(line, "STOP") == 0) {
        cmd.type = CMD_STOP;
    } else if (strcasecmp(line, "HIT") == 0) {
        cmd.type = CMD_HIT;
    } else if (starts_with_word(line, "DOOR")) {
        int n = atoi(line + 4);
        if (n < 1 || n > NUM_DOORS) {
            printf("Door must be 1-%d\n", NUM_DOORS);
            return;
        }
        cmd.type = CMD_DOOR_TEST;
        cmd.door = n - 1;
    } else if (strcasecmp(line, "HELP") == 0) {
        print_help();
        return;
    } else {
        printf("Unknown command: %s (type HELP)\n", line);
        return;
    }

    if (!game_send_command(&cmd)) {
        printf("Game is busy, try again\n");
    }
}

void laptop_task(void *arg)
{
    // Install the UART driver so we can read what's typed.
    ESP_ERROR_CHECK(uart_driver_install(LAPTOP_UART, 256, 0, 0, NULL, 0));
    print_help();

    char line[MAX_LINE_LEN];
    size_t len = 0;

    for (;;) {
        uint8_t c;
        if (uart_read_bytes(LAPTOP_UART, &c, 1, portMAX_DELAY) != 1) continue;

        if (c == '\r' || c == '\n') {               // Enter: run the line
            if (len > 0) {
                uart_write_bytes(LAPTOP_UART, "\r\n", 2);
                line[len] = '\0';
                handle_line(line);
                len = 0;
            }
        } else if ((c == '\b' || c == 0x7F) && len > 0) {   // Backspace
            len--;
            uart_write_bytes(LAPTOP_UART, "\b \b", 3);
        } else if (isprint(c) && len < MAX_LINE_LEN - 1) {
            line[len++] = (char)c;
            uart_write_bytes(LAPTOP_UART, (const char *)&c, 1);   // echo so you see what you type
        }
    }
}
