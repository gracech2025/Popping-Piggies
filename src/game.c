// The game: waits for START, runs 15 doors, scores hits, saves the top score.
// Grew out of the motor_actuation branch's "open a random pig, wait for a hit"
// loop in its app_main().
#include "game.h"
#include "config.h"
#include "pins.h"
#include "motors.h"
#include "beams.h"
#include "audio.h"
#include "display.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"

static const char *TAG = "game";

static QueueHandle_t cmd_queue;

// Top score is saved in flash (NVS) so it survives power-off.
#define NVS_NAMESPACE  "piggies"
static uint32_t top_score = 0;
static char top_school[SCHOOL_NAME_MAX] = "";

typedef enum { DOOR_HIT, DOOR_MISS, DOOR_STOPPED } door_result_t;

// ---------------------------------------------------------------------------
//  Small helpers
// ---------------------------------------------------------------------------

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;   // time since boot, in milliseconds
}

static void load_top_score(void)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return;  // nothing saved yet
    nvs_get_u32(nvs, "top_score", &top_score);
    size_t len = sizeof(top_school);
    nvs_get_str(nvs, "top_school", top_school, &len);
    nvs_close(nvs);
}

static void save_top_score(void)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        ESP_LOGW(TAG, "Couldn't save top score");
        return;
    }
    nvs_set_u32(nvs, "top_score", top_score);
    nvs_set_str(nvs, "top_school", top_school);
    nvs_commit(nvs);
    nvs_close(nvs);
}

static uint32_t points_for_time(int64_t ms)
{
    if (ms < TIER_FAST_MS)   return POINTS_FAST;
    if (ms < TIER_MEDIUM_MS) return POINTS_MEDIUM;
    return POINTS_SLOW;
}

// Randomly mix up a list (Fisher-Yates shuffle).
static void shuffle(uint8_t *list, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = esp_random() % (i + 1);
        uint8_t tmp = list[i];
        list[i] = list[j];
        list[j] = tmp;
    }
}

static bool same_door_twice_in_a_row(const uint8_t *order, int n)
{
    for (int i = 1; i < n; i++) {
        if (order[i] == order[i - 1]) return true;
    }
    return false;
}

// Pick which door opens on each of the DOORS_PER_GAME turns.
// Every door is used once before any door is used again, and the same door
// never opens twice in a row.
static void make_door_order(uint8_t order[DOORS_PER_GAME])
{
    uint8_t pool[NUM_DOORS];
    for (int i = 0; i < DOORS_PER_GAME; i++) {
        if (i % NUM_DOORS == 0) {               // start a new round of all doors
            for (int d = 0; d < NUM_DOORS; d++) pool[d] = d;
            shuffle(pool, NUM_DOORS);
        }
        order[i] = pool[i % NUM_DOORS];
    }
    for (int tries = 0; tries < 100 && same_door_twice_in_a_row(order, DOORS_PER_GAME); tries++) {
        shuffle(order, DOORS_PER_GAME);
    }
}

// Wait `ms` milliseconds, but keep listening for laptop commands meanwhile.
// Returns true if STOP was typed (the game should end).
static bool wait_or_stop(int ms)
{
    int64_t end = now_ms() + ms;
    for (;;) {
        int64_t left = end - now_ms();
        if (left <= 0) return false;

        TickType_t ticks = pdMS_TO_TICKS(left);
        if (ticks == 0) ticks = 1;

        game_cmd_t cmd;
        if (xQueueReceive(cmd_queue, &cmd, ticks) == pdTRUE) {
            if (cmd.type == CMD_STOP) return true;
            ESP_LOGW(TAG, "Game in progress: only STOP works right now");
        }
    }
}

// ---------------------------------------------------------------------------
//  One door: open it, wait for a ball (or time out), close it.
// ---------------------------------------------------------------------------

static door_result_t play_one_door(int door, int64_t *reaction_ms)
{
    // A beam that's already blocked (e.g. a stuck ball) doesn't count until it clears.
    uint16_t already_blocked = beams_read();

    int64_t start = now_ms();      // scoring time starts at the open signal
    motor_open(door);
    bool motor_running = true;
    door_result_t result = DOOR_MISS;

    for (;;) {
        int64_t elapsed = now_ms() - start;

        if (motor_running && elapsed >= MOTOR_MOVE_MS) {
            motor_stop(door);      // door is fully open
            motor_running = false;
        }
        if (elapsed >= DOOR_OPEN_MS) {
            break;                 // too slow: miss
        }

        // Check for laptop commands. Waiting here for up to BEAM_POLL_MS is also
        // what makes this loop check the beams every 10 ms.
        game_cmd_t cmd;
        if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(BEAM_POLL_MS)) == pdTRUE) {
            if (cmd.type == CMD_STOP) { result = DOOR_STOPPED; break; }
            if (cmd.type == CMD_HIT)  { result = DOOR_HIT;     break; }
            ESP_LOGW(TAG, "Game in progress: only STOP or HIT work right now");
        }

        uint16_t broken = beams_read();
        if (broken & ~already_blocked) {   // a beam that was clear just broke
            result = DOOR_HIT;
            break;
        }
        already_blocked &= broken;         // beams that cleared can count again
    }

    *reaction_ms = now_ms() - start;

    motor_close(door);
    vTaskDelay(pdMS_TO_TICKS(MOTOR_MOVE_MS));
    motor_stop(door);
    return result;
}

// ---------------------------------------------------------------------------
//  A whole game
// ---------------------------------------------------------------------------

static void game_stopped(void)
{
    ESP_LOGI(TAG, "Game stopped");
    display_show_idle(top_score, top_school);
}

static void run_game(const char *school)
{
    ESP_LOGI(TAG, "New game for %s", school);

    for (int n = COUNTDOWN_SECONDS; n > 0; n--) {
        display_show_countdown(n);
        audio_play(SOUND_COUNTDOWN);
        if (wait_or_stop(1000)) { game_stopped(); return; }
    }
    audio_play(SOUND_GAME_START);

    uint8_t order[DOORS_PER_GAME];
    make_door_order(order);

    uint32_t score = 0;
    int hits = 0;
    display_show_game(school, score, DOORS_PER_GAME);

    for (int turn = 0; turn < DOORS_PER_GAME; turn++) {
        int door = order[turn];
        ESP_LOGI(TAG, "Turn %d/%d: opening door %d (%s)",
                 turn + 1, DOORS_PER_GAME, door + 1, motor_name(door));

        int64_t ms = 0;
        door_result_t result = play_one_door(door, &ms);

        if (result == DOOR_STOPPED) { game_stopped(); return; }

        if (result == DOOR_HIT) {
            uint32_t points = points_for_time(ms);
            score += points;
            hits++;
            audio_play(SOUND_HIT);
            ESP_LOGI(TAG, "HIT after %lld ms: +%lu points", (long long)ms, (unsigned long)points);
        } else {
            audio_play(SOUND_MISS);
            ESP_LOGI(TAG, "Miss");
        }
        display_show_game(school, score, DOORS_PER_GAME - turn - 1);

        if (turn < DOORS_PER_GAME - 1) {
            int gap = GAP_MIN_MS + (int)(esp_random() % (GAP_MAX_MS - GAP_MIN_MS + 1));
            if (wait_or_stop(gap)) { game_stopped(); return; }
        }
    }

    bool new_record = score > top_score;
    if (new_record) {
        top_score = score;
        strlcpy(top_school, school, sizeof(top_school));
        save_top_score();
        audio_play(SOUND_NEW_HIGH_SCORE);
    } else {
        audio_play(SOUND_GAME_OVER);
    }
    // The end screen stays up until the next START.
    display_show_end(school, score, hits, top_score, new_record);
}

// TEST: open one door, show the beams for 2 s, close it.
static void test_door(int door)
{
    ESP_LOGI(TAG, "Testing door %d (%s)", door + 1, motor_name(door));
    motor_open(door);
    vTaskDelay(pdMS_TO_TICKS(MOTOR_MOVE_MS));
    motor_stop(door);

    for (int i = 0; i < 4; i++) {
        ESP_LOGI(TAG, "Broken beams (bit 0 = BB_RX1): 0x%02X", (unsigned)beams_read());
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    motor_close(door);
    vTaskDelay(pdMS_TO_TICKS(MOTOR_MOVE_MS));
    motor_stop(door);
    ESP_LOGI(TAG, "Door test done");
}

// ---------------------------------------------------------------------------
//  Public functions
// ---------------------------------------------------------------------------

void game_init(void)
{
    cmd_queue = xQueueCreate(8, sizeof(game_cmd_t));
}

bool game_send_command(const game_cmd_t *cmd)
{
    return xQueueSend(cmd_queue, cmd, 0) == pdTRUE;
}

void game_task(void *arg)
{
    load_top_score();
    display_show_idle(top_score, top_school);

    for (;;) {
        game_cmd_t cmd;
        xQueueReceive(cmd_queue, &cmd, portMAX_DELAY);   // sleep until a command arrives

        switch (cmd.type) {
        case CMD_START:
            run_game(cmd.school);
            break;
        case CMD_DOOR_TEST:
            test_door(cmd.door);
            break;
        case CMD_STOP:
        case CMD_HIT:
            ESP_LOGW(TAG, "No game running. Type: START <university>");
            break;
        }
    }
}
