#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

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

     // Test: red pixel at x = 10, y = 10
    driver.set_pixel(10, 10, 255, 0, 0);
}