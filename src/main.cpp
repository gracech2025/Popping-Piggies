#include "hub75.h"
#include "lvgl.h"

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

// ---------------------------------
// SCREEN/STATES SETUP
// ---------------------------------

// Is there anything else that should be set when screens are made? If there are a lot of things, maybe we should make a separate function to initialize screen creation
// --- Create all screens once ---
void createAllScreens(Hub75Driver &driver) {
    
    // Start screen: should always show bitmap icons, top score: "x" and "start game" text
    // Refreshes to start screen when the game ends and "x" is updated if necessary
    // We need to get the actual bitmap icons
    lv_obj_t *startScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(startScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

        lv_obj_t *startGameText = lv_obj_create(startScreen);
        lv_obj_set_size(startGameText, 75, 8);
        lv_obj_set_pos(startGameText, 30, 5);

        lv_obj_t *topScoreText = lv_obj_create(startScreen);
        lv_obj_set_size(topScoreText, 70, 6);
        lv_obj_set_pos(topScoreText, 30, 20);

        // Two bitmap icons ~20x20 pixels on left and right of the screen

    // Countdown screen: should show countdown timer, "get ready" text, and bitmap icons
    // Refreshes to countdown screen when the game starts
    lv_obj_t *countdownScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(countdownScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

        lv_obj_t *countdownText = lv_obj_create(countdownScreen);
        lv_obj_set_size(countdownText, 50, 25);
        lv_obj_align(countdownText, LV_ALIGN_CENTER, 0, 0);

        // create container for "get ready" text
        lv_obj_t *getReadyText = lv_obj_create(countdownScreen);
        lv_obj_set_size(getReadyText, 50, 6);
        lv_obj_align(getReadyText, LV_ALIGN_BOTTOM_MID, 0, -4);

    // Game screen: should show current score, top score, and display streak text
    // Is it based on countdown or number of objects thrown? Will do countdown for now
    lv_obj_t *gameScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(gameScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);
    
        // Create parent containers for the two panels and top/bottom text
        // These positions and sizes are estimates until we can test on the actual hardware!
        // I got rid of the top text for now
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
    lv_obj_t *endScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(endScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

        lv_obj_t *finalScoreText = lv_obj_create(endScreen);
        lv_obj_set_size(finalScoreText, 70, 8);
        lv_obj_align(finalScoreText, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *timesUpText = lv_obj_create(endScreen);
        lv_obj_set_size(timesUpText, 50, 6);
        lv_obj_align(timesUpText, LV_ALIGN_TOP_MID, 0, 4);
}

// --- Next State Logic ---
void changeScreen(ScreenState &currentState, Hub75Driver &driver) {
    switch (currentState) {
        case START_SCREEN:
            currentState = COUNTDOWN_SCREEN;
            break;
        case COUNTDOWN_SCREEN:
            currentState = GAME_SCREEN;
            break;
        case GAME_SCREEN:
            currentState = END_SCREEN;
            break;
        case END_SCREEN:
            currentState = START_SCREEN;
            break;
    }
}

// --- Event handling to determine when to change screens ---
void handleEvent(ScreenState &currentState, Hub75Driver &driver) {
    // Countdown starts on user input
    // Game starts after countdown ends
    // Game ends after countdown ends
    // End screen goes back to start screen on user input
}

// ---------------------------------
// SCREEN DISPLAYS
// ---------------------------------

// --- Display the start screen ---
void startScreenDisplay(Hub75Driver &driver, int &topScore) {
    // Stop any ongoing animations or timers
    
    lv_label_set_text(startGameText, "Start Game");
    lv_label_set_text_fmt(topScoreText, "Top Score: %d", topScore);

    driver.flip_buffer();  // Show the changes on the display
}

// --- Countdown logic ---

// --- Display the countdown screen ---
void countdownScreenDisplay(Hub75Driver &driver) {
    // Stop any ongoing animations or timers

    // Display countdown timer and "get ready" text
    driver.flip_buffer();  // Show the changes on the display
}

// --- Display the game screen ---
void gameScreenDisplay(Hub75Driver &driver, ScoreInfo &scoreInfo) {
    // Stop any ongoing animations or timers

    // Display current score, top score, and streak text
    driver.flip_buffer();  // Show the changes on the display
}

// --- Display the end screen ---
void endScreenDisplay(Hub75Driver &driver, ScoreInfo &scoreInfo) {
    // Stop any ongoing animations or timers

    // Display final score, top score, and "play again" text
    driver.flip_buffer();  // Show the changes on the display
}