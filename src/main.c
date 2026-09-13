#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "POPPING_PIGGIES";

// hardware pins
#define I2C_BUS0_SCL_PIN    1
#define I2C_BUS0_SDA_PIN    2

#define I2C_BUS1_SCL_PIN    19
#define I2C_BUS1_SDA_PIN    20

#define I2C_FREQ_HZ         400000

#define MCP1_ADDR           0x20
#define MCP2_ADDR           0x21

// MCP23017 registers
#define REG_IODIRA          0x00
#define REG_IODIRB          0x01
#define REG_GPPUA           0x0C
#define REG_GPPUB           0x0D
#define REG_GPIOA           0x12
#define REG_GPIOB           0x13

// hardware device handles
static i2c_master_dev_handle_t mcp1_handle;
static i2c_master_dev_handle_t mcp2_handle;

// 16-bit shadow state registers for outputs
static uint16_t mcp1_state = 0x0000;
static uint16_t mcp2_state = 0x0000;

typedef struct {
    i2c_master_dev_handle_t motor_dev;  // expander driving the motor
    uint8_t in1_pin;                     // IN1 pin (0-15)
    uint8_t in2_pin;                     // IN2 pin (0-15)
    i2c_master_dev_handle_t sensor_dev; // expander reading the break-beam
    uint8_t sensor_pin;                  // sensor pin (0-15)
} piggie_config_t;

// motor mapping
static piggie_config_t piggies[9];

// i2c write
static esp_err_t mcp_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val) {
    uint8_t tx_buf[2] = {reg, val};
    return i2c_master_transmit(dev, tx_buf, sizeof(tx_buf), -1);
}

// i2c read
static uint8_t mcp_read_reg(i2c_master_dev_handle_t dev, uint8_t reg) {
    uint8_t val = 0;
    i2c_master_transmit_receive(dev, &reg, 1, &val, 1, -1);
    return val;
}

// bitwise pin output setter
static void set_mcp_pin(i2c_master_dev_handle_t dev, uint16_t *shadow_state, uint8_t pin, uint8_t state) {
    if (state) {
        *shadow_state |= (1 << pin);
    } else {
        *shadow_state &= ~(1 << pin);
    }

    if (pin < 8) {
        mcp_write_reg(dev, REG_GPIOA, (uint8_t)(*shadow_state & 0xFF));
    } else {
        mcp_write_reg(dev, REG_GPIOB, (uint8_t)((*shadow_state >> 8) & 0xFF));
    }
}

// pin input reader
static uint8_t get_mcp_pin(i2c_master_dev_handle_t dev, uint8_t pin) {
    uint8_t reg = (pin < 8) ? REG_GPIOA : REG_GPIOB;
    uint8_t bit = (pin < 8) ? pin : (pin - 8);
    uint8_t val = mcp_read_reg(dev, reg);
    return (val >> bit) & 0x01;
}

// motor actuation
static void drive_motor(piggie_config_t pig, bool open) {
    uint16_t *state = (pig.motor_dev == mcp1_handle) ? &mcp1_state : &mcp2_state;
    if (open) {
        set_mcp_pin(pig.motor_dev, state, pig.in1_pin, 1);
        set_mcp_pin(pig.motor_dev, state, pig.in2_pin, 0);
    } else {
        set_mcp_pin(pig.motor_dev, state, pig.in1_pin, 0);
        set_mcp_pin(pig.motor_dev, state, pig.in2_pin, 1);
    }
}

static void stop_motor(piggie_config_t pig) {
    uint16_t *state = (pig.motor_dev == mcp1_handle) ? &mcp1_state : &mcp2_state;
    set_mcp_pin(pig.motor_dev, state, pig.in1_pin, 0);
    set_mcp_pin(pig.motor_dev, state, pig.in2_pin, 0);
}

void app_main(void) {
    // set config params for both buses
    i2c_master_bus_config_t bus_cfg_0 = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_BUS0_SCL_PIN,
        .sda_io_num = I2C_BUS0_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle_0;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg_0, &bus_handle_0));

    i2c_master_bus_config_t bus_cfg_1 = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_1,
        .scl_io_num = I2C_BUS1_SCL_PIN,
        .sda_io_num = I2C_BUS1_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle_1;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg_1, &bus_handle_1));

    // attach addresses
    i2c_device_config_t dev_cfg_1 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP1_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_0, &dev_cfg_1, &mcp1_handle));

    i2c_device_config_t dev_cfg_2 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP2_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_1, &dev_cfg_2, &mcp2_handle));

    // config MCP23017 expanders
    // expander 1: 0-4 outputs, 5-7 inputs
    mcp_write_reg(mcp1_handle, REG_IODIRA, 0x1F);  // 00011111
    mcp_write_reg(mcp1_handle, REG_GPPUA, 0x1F);
    mcp_write_reg(mcp1_handle, REG_IODIRB, 0x00);

    // expander 2: all outputs
    mcp_write_reg(mcp2_handle, REG_IODIRA, 0x00);
    mcp_write_reg(mcp2_handle, REG_IODIRB, 0x00);

    // double check ts is right
    piggies[0] = (piggie_config_t){mcp1_handle, 5, 6, mcp1_handle, 0};
    piggies[1] = (piggie_config_t){mcp1_handle, 7, 8, mcp1_handle, 1};
    piggies[2] = (piggie_config_t){mcp1_handle, 9, 10, mcp1_handle, 2};
    piggies[3] = (piggie_config_t){mcp1_handle, 11, 12, mcp1_handle, 3};
    piggies[4] = (piggie_config_t){mcp1_handle, 13, 14, mcp1_handle, 4};
    piggies[5] = (piggie_config_t){mcp2_handle, 0, 1, mcp2_handle, 2};
    piggies[6] = (piggie_config_t){mcp2_handle, 3, 4, mcp2_handle, 5};
    piggies[7] = (piggie_config_t){mcp2_handle, 6, 7, mcp2_handle, 8};
    piggies[8] = (piggie_config_t){mcp2_handle, 9, 10, mcp2_handle, 11};

    ESP_LOGI(TAG, "Game Engine Started");

    // main loop
    while (1) {
        int idx = esp_random() % 9;
        piggie_config_t active_pig = piggies[idx];

        ESP_LOGI(TAG, "Piggie #%d...", idx + 1);
        drive_motor(active_pig, true);

        TickType_t start_time = xTaskGetTickCount();
        bool hit = false;
        while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(3000)) {
            if (get_mcp_pin(active_pig.sensor_dev, active_pig.sensor_pin) == 0) {
                hit = true;
                ESP_LOGI(TAG, "Hit on Piggie #%d", idx + 1);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (!hit) {
            ESP_LOGI(TAG, "Piggie #%d timed out", idx + 1);
        }

        // retract motor
        drive_motor(active_pig, false);
        vTaskDelay(pdMS_TO_TICKS(500));
        stop_motor(active_pig);

        vTaskDelay(pdMS_TO_TICKS(1000 + (esp_random() % 1500)));
    }
}