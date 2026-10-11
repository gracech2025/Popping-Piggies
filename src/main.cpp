#include "hub75.h"
#include "lvgl.h"

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

    // Create parent containers for the two panels and top/bottom text
    // These positions and sizes are estimates until we can test on the actual hardware!
    lv_obj_t *left_panel = lv_obj_create(lv_screen_active());
    lv_obj_set_size(left_panel, 50, 25);
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 30, 0);

    lv_obj_t *right_panel = lv_obj_create(lv_screen_active());
    lv_obj_set_size(right_panel, 50, 25);
    lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, -30, 0);

    lv_obj_t *top_text = lv_obj_create(lv_screen_active());
    lv_obj_set_size(top_text, 100, 8);
    lv_obj_align(top_text, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *bottom_text = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bottom_text, 100, 6);
    lv_obj_align(bottom_text, LV_ALIGN_BOTTOM_MID, 0, -4);

    driver.flip_buffer();  // Show the changes on the display
}



// Helper function to convert RGB888 to RGB565
uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g6 = (g >> 2) & 0x3F;
    uint16_t b5 = (b >> 3) & 0x1F;
    return (r5 << 11) | (g6 << 5) | b5;
}