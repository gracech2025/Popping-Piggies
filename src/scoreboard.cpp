#include "hub75.h"
#include "lvgl.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ---------------------------------
// LED BOARD SETUP
// ---------------------------------

extern "C" void app_main()
{
 Hub75Config config{};

    // Each individual panel is 64 pixels wide x 32 pixels tall
    config.panel_width = 64;
    config.panel_height = 32;

    // We have 2 panels beside eachother
    config.layout_rows = 1;
    config.layout_cols = 2;
    config.layout = Hub75PanelLayout::HORIZONTAL;

    // Our panel manufacturer confirmed the driver is FM6124
    config.shift_driver = Hub75ShiftDriver::FM6124;

    //Temporary ESP32-S3 GPIO mapping
    config.pins.r1 = 1;
    config.pins.g1 = 2;
    config.pins.b1 = 3;

    config.pins.r2 = 4;
    config.pins.g2 = 5;
    config.pins.b2 = 6;

    config.pins.a = 7;
    config.pins.b = 8;
    config.pins.c = 9;
    config.pins.d = 10;
    config.pins.e = -1;

    config.pins.lat = 12;
    config.pins.oe = 13;
    config.pins.clk = 14;

     // Create and start the display driver
    Hub75Driver driver(config);
    driver.begin();

    driver.clear();  // Clear the back buffer

    driver.flip_buffer();  // Show the changes on the display

    //lv_init(); //initialize LVGL

    while (true) {
        // Main loop: handle LVGL tasks and refresh the display
        lv_timer_handler();  // Handle LVGL tasks
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay to allow other tasks to run
    }
}

// ---------------------------------
// STRUCTS AND ENUMS, DECLARATIONS, AND HELPER FUNCTIONS
// ---------------------------------

// Enum for the different screens
typedef enum {
    START_SCREEN,
    COUNTDOWN_SCREEN,
    GAME_SCREEN,
    END_SCREEN,
} ScreenState;

// Struct for score information
typedef struct {
    int topScore;
    int currentScore;
    int streak;
} ScoreInfo;

// Helper function to convert RGB888 to RGB565
uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g6 = (g >> 2) & 0x3F;
    uint16_t b5 = (b >> 3) & 0x1F;
    return (r5 << 11) | (g6 << 5) | b5;
}

// Declare global variables
static lv_obj_t *startGameText;
static lv_obj_t *topScoreText;
static int topScore = 0;
ScreenState currentState = START_SCREEN;

// Timing variables
static const int STATE_DURATION [] = {0, 3, 30, 0}; // Duration for each state in seconds - MAYBE CHANGE THE 30S
static lv_timer_t *stateTimer = nullptr;
static void stateTimerCB(lv_timer_t *timer);
static int timeRemaining = 0;

//global variables for screens and text objects
static lv_obj_t *startScreen;
static lv_obj_t *countdownScreen;
static lv_obj_t *gameScreen;
static lv_obj_t *endScreen;

static lv_obj_t *countdownText;
static lv_obj_t *getReadyText;
static lv_obj_t *finalScoreText;
static lv_obj_t *timesUpText;
static ScoreInfo scoreInfo = {0, 0, 0};

//function declarations for screen displays
void startScreenDisplay(Hub75Driver &driver, int &topScore);
void countdownScreenDisplay(Hub75Driver &driver);
void gameScreenDisplay(Hub75Driver &driver, ScoreInfo &scoreInfo);
void endScreenDisplay(Hub75Driver &driver, ScoreInfo &scoreInfo);
void changeScreen(ScreenState &currentState, Hub75Driver &driver);
void handleEvent(ScreenState &currentState, Hub75Driver &driver);
void createAllScreens(Hub75Driver &driver);

// ---------------------------------
// SCREEN/STATES SETUP
// ---------------------------------

// Is there anything else that should be set when screens are made? If there are a lot of things, maybe we should make a separate function to initialize screen creation
// --- Create all screens once ---
void createAllScreens(Hub75Driver &driver) {
    // Create the timer once at the beginning
    stateTimer = lv_timer_create(stateTimerCB, 1000, &driver);
    lv_timer_pause(stateTimer); // Start paused; will be resumed when a state with a duration is entered

    // Start screen: should always show bitmap icons, top score: "x" and "start game" text
    // Refreshes to start screen when the game ends and "x" is updated if necessary
    // We need to get the actual bitmap icons
    //lv_obj_t *startScreen = lv_obj_create(NULL);
    startScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(startScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

        //startGameText = lv_obj_create(startScreen);
        //removed set size because 
        //lv_obj_set_size(startGameText, 75, 8); 
        startGameText = lv_label_create(startScreen);
        lv_obj_set_pos(startGameText, 30, 5);
        lv_label_set_text(startGameText, "Start Game");

        //topScoreText = lv_obj_create(startScreen);
        //lv_obj_set_size(topScoreText, 70, 6);
        topScoreText = lv_label_create(startScreen);
        lv_obj_set_pos(topScoreText, 30, 20);
        lv_label_set_text(topScoreText, "Top Score: 0");

        // Two bitmap icons ~20x20 pixels on left and right of the screen

    // Countdown screen: should show countdown timer, "get ready" text, and bitmap icons
    // Refreshes to countdown screen when the game starts
    countdownScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(countdownScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

        countdownText = lv_label_create(countdownScreen);
        //lv_obj_set_size(countdownText, 50, 25);
        lv_label_set_text(countdownText, "3");
        lv_obj_align(countdownText, LV_ALIGN_CENTER, 0, 0);

        // create container for "get ready" text
        getReadyText = lv_label_create(countdownScreen);
        //lv_obj_set_size(getReadyText, 50, 6);
        lv_label_set_text(getReadyText, "Get Ready!");
        lv_obj_align(getReadyText, LV_ALIGN_BOTTOM_MID, 0, -4);

    // Game screen: should show current score, top score, and display streak text
    // Is it based on countdown or number of objects thrown? Will do countdown for now
    gameScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(gameScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);
    
        // Should we make these labels like everything else?
        lv_obj_t *left_panel = lv_obj_create(gameScreen);
        lv_obj_set_size(left_panel, 50, 25);
        lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 30, 0);

        lv_obj_t *right_panel = lv_obj_create(gameScreen);
        lv_obj_set_size(right_panel, 50, 25);
        lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, -30, 0);

        lv_obj_t *bottom_text = lv_obj_create(gameScreen);
        lv_obj_set_size(bottom_text, 100, 6);
        lv_obj_align(bottom_text, LV_ALIGN_BOTTOM_MID, 0, -4);

    // End screen: should show final score, top score, and "play again" text
    // Refreshes to end screen when the game ends
    endScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(endScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

        finalScoreText = lv_label_create(endScreen);
        //lv_obj_set_size(finalScoreText, 70, 8);
        lv_label_set_text(finalScoreText, "Final Score: 0");
        lv_obj_align(finalScoreText, LV_ALIGN_CENTER, 0, 0);

        timesUpText = lv_label_create(endScreen);
        //lv_obj_set_size(timesUpText, 50, 6);
        lv_label_set_text(timesUpText, "Time's Up!");
        lv_obj_align(timesUpText, LV_ALIGN_TOP_MID, 0, 4);
}

// --- Next State Logic ---
void changeScreen(ScreenState &currentState, Hub75Driver &driver) {
    switch (currentState) {
        case START_SCREEN:
            currentState = COUNTDOWN_SCREEN;
            //loading countdown screen
            lv_screen_load(countdownScreen);
            countdownScreenDisplay(driver);
            break;

        case COUNTDOWN_SCREEN:
            currentState = GAME_SCREEN;
            //loading gameplay screen
            lv_screen_load(gameScreen);
            gameScreenDisplay(driver, scoreInfo);
            break;

        case GAME_SCREEN:
            currentState = END_SCREEN;
            //loading end screen
            lv_screen_load(endScreen);
            endScreenDisplay(driver, scoreInfo);
            break;

        case END_SCREEN:
            currentState = START_SCREEN;
            //return to start screen
            lv_screen_load(startScreen);
            startScreenDisplay(driver, topScore);
            break;
    }

    // Reset the timer for the new state
    timeRemaining = STATE_DURATION[currentState];
    if (timeRemaining > 0) {
        lv_timer_reset(stateTimer);
        lv_timer_resume(stateTimer);
    } else {
        lv_timer_pause(stateTimer); // Pause the timer if the current state has no duration
    }
}

// --- Event handling to determine when to change screens ---
void handleEvent(ScreenState &currentState, Hub75Driver &driver) {
    
    // If there is outside input, we can check for it here and call changeScreen() when appropriate
}

// ---------------------------------
// SCREEN DISPLAYS
// ---------------------------------

// --- Display the start screen ---
void startScreenDisplay(Hub75Driver &driver, int &topScore) {
    // Stop any ongoing animations or timers
    if (startGameText == nullptr || topScoreText == nullptr) {
        
        return;
    }
    lv_label_set_text(startGameText, "Start Game");
    lv_label_set_text_fmt(topScoreText, "Top Score: %d", topScore);

    //driver.flip_buffer();  // Show the changes on the display
}

// --- Timer callback ---
void stateTimerCB(lv_timer_t *timer) {
    
    Hub75Driver *driver = (Hub75Driver *)lv_timer_get_user_data(timer);
    
    if (timeRemaining > 0) {
        timeRemaining--;
        lv_label_set_text_fmt(countdownText, "%d", timeRemaining);
    } else {
        // Time's up for the current state, move to the next state
        changeScreen(currentState, *driver);
    }
}

// --- Display the countdown screen ---
void countdownScreenDisplay(Hub75Driver &driver) {
    // Stop any ongoing animations or timers

    // Display countdown timer and "get ready" text
    //driver.flip_buffer();  // Show the changes on the display
    if (countdownText != nullptr) {
        lv_label_set_text(countdownText, "3"); // Connect to countdown timer
    }
    if (getReadyText != nullptr) {
        lv_label_set_text(getReadyText, "Get Ready!");
    }
}

// --- Display the game screen ---
void gameScreenDisplay(Hub75Driver &driver, ScoreInfo &scoreInfo) {
    // Stop any ongoing animations or timers

    // School name/start inputted from a laptop at the game
    // Score and combos are sent from other branch

    // Display current score, top score, and streak text
    //driver.flip_buffer();  // Show the changes on the display
}

// --- Display the end screen ---
void endScreenDisplay(Hub75Driver &driver, ScoreInfo &scoreInfo) {
    // Stop any ongoing animations or timers

    // Display final score, top score, and "play again" text
    //driver.flip_buffer();  // Show the changes on the display
    if (finalScoreText != nullptr) {
        lv_label_set_text_fmt(finalScoreText, "Final Score: %d", scoreInfo.currentScore);
    }

    if (timesUpText != nullptr) {
        lv_label_set_text(timesUpText, "Time's Up!");
    }
}