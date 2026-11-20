// MCP23017 pin expander driver.
// From the motor_actuation branch: mcp_write_reg(), mcp_read_reg(),
// set_mcp_pin(), get_mcp_pin() and the I2C setup in its app_main().
#include "expander.h"
#include "pins.h"

#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "expander";

#define I2C_FREQ_HZ      400000
#define I2C_TIMEOUT_MS   50

// MCP23017 registers (default "BANK=0" layout: each A register is followed by
// its B register, so writing 2 bytes starting at an A register sets A then B)
#define REG_IODIRA  0x00   // pin direction: 1 = input, 0 = output
#define REG_GPPUA   0x0C   // internal pull-up resistors: 1 = on
#define REG_GPIOA   0x12   // read the pins
#define REG_OLATA   0x14   // set the output pins

typedef struct {
    bool present;                 // false if the chip didn't answer at startup
    i2c_master_dev_handle_t dev;
    uint16_t outputs;             // remembered output levels (bit per pin)
    int read_errors;
} expander_t;

static expander_t expanders[NUM_EXPANDERS];

// Write a 16-bit value to an A/B register pair.
static esp_err_t write_pair(expander_t *e, uint8_t reg_a, uint16_t value)
{
    uint8_t buf[3] = { reg_a, value & 0xFF, value >> 8 };
    return i2c_master_transmit(e->dev, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

static void init_one(int id, int port, int scl, int sda, uint8_t addr, uint16_t input_mask)
{
    expander_t *e = &expanders[id];

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = port,
        .scl_io_num = scl,
        .sda_io_num = sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    // Is the chip actually there?
    if (i2c_master_probe(bus, addr, I2C_TIMEOUT_MS) != ESP_OK) {
        ESP_LOGW(TAG, "Pin expander %d (0x%02X) not found. Running without it "
                      "(motors/beams on it won't work).", id + 1, addr);
        e->present = false;
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &e->dev));

    // Outputs LOW first (motors stopped, LEDs off), THEN switch pins to outputs,
    // so nothing twitches at power-up.
    e->outputs = 0x0000;
    write_pair(e, REG_OLATA, e->outputs);
    write_pair(e, REG_IODIRA, input_mask);
    write_pair(e, REG_GPPUA, input_mask);

    e->present = true;
    ESP_LOGI(TAG, "Pin expander %d (0x%02X) ready", id + 1, addr);
}

void expanders_init(void)
{
    init_one(EXP1, EXP1_I2C_PORT, EXP1_SCL_GPIO, EXP1_SDA_GPIO, EXP1_ADDR, EXP1_INPUT_MASK);
    init_one(EXP2, EXP2_I2C_PORT, EXP2_SCL_GPIO, EXP2_SDA_GPIO, EXP2_ADDR, EXP2_INPUT_MASK);
}

void expander_write_pin(int expander, int pin, bool high)
{
    if (expander < 0 || expander >= NUM_EXPANDERS || pin < 0 || pin > 15) return;
    expander_t *e = &expanders[expander];
    if (!e->present) return;

    if (high) {
        e->outputs |= (1u << pin);
    } else {
        e->outputs &= ~(1u << pin);
    }
    if (write_pair(e, REG_OLATA, e->outputs) != ESP_OK) {
        ESP_LOGW(TAG, "Write to pin expander %d failed", expander + 1);
    }
}

uint16_t expander_read_all(int expander)
{
    if (expander < 0 || expander >= NUM_EXPANDERS) return 0xFFFF;
    expander_t *e = &expanders[expander];
    if (!e->present) return 0xFFFF;

    uint8_t reg = REG_GPIOA;
    uint8_t data[2] = { 0xFF, 0xFF };
    if (i2c_master_transmit_receive(e->dev, &reg, 1, data, 2, I2C_TIMEOUT_MS) != ESP_OK) {
        // Beams are read every 10 ms, so only log every 500th error.
        if (e->read_errors++ % 500 == 0) {
            ESP_LOGW(TAG, "Read from pin expander %d failed", expander + 1);
        }
        return 0xFFFF;
    }
    return (uint16_t)(data[0] | (data[1] << 8));
}
