// =============================================================================
//  Popping Piggies — main.c
//
//  app_main() runs once at power-up. It sets up the hardware, then starts the
//  FreeRTOS tasks that run the game. Each task is like a separate little
//  program running at the same time:
//
//    laptop task  --commands-->  game task  --sounds-->  audio task
//    (reads what you type)       (doors, beams, score)   (plays WAV files)
//
//  The game task is the only one that touches the pin expanders (motors and
//  beams). Other tasks talk to it through a queue (a FreeRTOS "mailbox").
//  The LED display will be added later as one more task.
//
//  Combines the speaker branch (now in audio.c) and the motor_actuation
//  branch (now in expander.c, motors.c, beams.c and game.c).
// =============================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "expander.h"
#include "motors.h"
#include "audio.h"
#include "game.h"
#include "laptop.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Popping Piggies starting");

    // Flash storage (used to remember the top score).
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Hardware
    expanders_init();   // I2C + pin expanders (warns and continues if not connected)
    motors_init();      // make sure every door motor is stopped

    // Tasks
    audio_init();       // starts the audio task
    game_init();        // creates the game's command queue
    //          function     name      stack(bytes) arg   priority  handle
    xTaskCreate(game_task,   "game",   4096,        NULL, 5,        NULL);
    xTaskCreate(laptop_task, "laptop", 4096,        NULL, 4,        NULL);

    // app_main can return now; the tasks keep running.
}
