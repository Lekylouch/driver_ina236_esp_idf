# INA236 ESP-IDF Driver

ESP-IDF component for the INA236 high-side current/power monitor.

This driver supports:
- Bus voltage reading
- Shunt voltage reading
- Current reading
- Power reading
- Configurable alert modes with optional GPIO interrupt callback

This version is adapted for ESP-IDF using the **new I2C master driver API** (`driver/i2c_master.h`). The INA236 device is attached to an already-created I2C master bus handle. 

> Modified by **Sylvestre Ibara** (2026) for ESP32 / ESP-IDF usage.  
> Original copyright notices are preserved in source files.

---

## Overview

The INA236 is a digital current and power monitor from Texas Instruments. It measures:
- Bus voltage
- Shunt voltage
- Current
- Power

The driver:
- Attaches the INA236 to an existing ESP-IDF I2C master bus
- Configures the calibration register from `r_shunt` and `current_max`
- Checks the device ID during initialization
- Provides measurement APIs in floating-point units
- Supports alert configuration through the INA236 alert register
- Optionally connects the ALERT pin to a GPIO ISR callback

---

## Features

- ESP-IDF component compatible
- Uses `driver/i2c_master.h`
- Default INA236 I2C address: `0x40`
- Configurable shunt resistor value
- Configurable expected max current for calibration
- Optional ALERT GPIO interrupt support
- Public API for:
  - `ina236_get_voltage()`
  - `ina236_get_voltage_shunt()`
  - `ina236_get_current()`
  - `ina236_get_power()`
  - `ina236_set_alert()`

---

## Repository Structure

```text
driver_ina236_esp_idf/
├── ina236.c
├── include/
│   └── ina236.h
├── priv_include/
│   └── ina_236_reg.h
├── test_apps/
├── CMakeLists.txt
└── README.md
```

---

## Requirements

- ESP-IDF with support for the new master I2C API
- An I2C bus created by the application before calling `ina236_create()`

This driver does **not** create the I2C bus for you.  
You must first initialize the I2C master bus in your application, then pass the resulting `i2c_master_bus_handle_t` to the driver.

---

## Hardware Connection

Example wiring:

| INA236 Pin | ESP32 Pin | Description |
|------------|-----------|-------------|
| SDA        | GPIO21    | I2C data    |
| SCL        | GPIO22    | I2C clock   |
| ALERT      | GPIO12    | Optional alert interrupt |
| VCC        | 3.3V      | Power supply |
| GND        | GND       | Ground |

> Use appropriate I2C pull-up resistors on SDA and SCL if required by your hardware design.  
> The driver only configures the optional ALERT GPIO input when alert mode is enabled.

---

## Installation

### Option 1 — Add locally in `components/`

Copy this repository into your project `components` directory:

```text
your_project/
├── main/
├── components/
│   └── ina236/
│       ├── ina236.c
│       ├── include/
│       │   └── ina236.h
│       ├── priv_include/
│       │   └── ina_236_reg.h
│       ├── CMakeLists.txt
│       └── README.md
└── CMakeLists.txt
```

Then include it in your source file:

```c
#include "ina236.h"
```

---

### Option 2 — Add with `idf_component.yml`

You can also declare the component as a managed dependency.

Example:

```yaml
dependencies:
  ina236:
    git: https://github.com/Lekylouch/driver_ina236_esp_idf.git
    version: main
```

Depending on your ESP-IDF/component manager setup, you may also use a namespace-based dependency if you publish the component to the ESP-IDF component registry.

If you use the Git form above, place it in your project's `idf_component.yml`.

Example project layout:

```text
your_project/
├── main/
├── idf_component.yml
└── CMakeLists.txt
```

Example `idf_component.yml`:

```yaml
dependencies:
  ina236:
    git: https://github.com/Lekylouch/driver_ina236_esp_idf.git
    version: main
```

---

## Public API

The public API is defined in `include/ina236.h`.

### Types

```c
typedef struct ina236_t *ina236_handle_t;

typedef void (*ina236_alert_cb_t)(void *arg);
```

### Alert modes

```c
typedef enum {
    INA236_ALERT_SOL = 0, // Shunt Over Limit
    INA236_ALERT_SUL,     // Shunt Under Limit
    INA236_ALERT_BOL,     // Bus Over Limit
    INA236_ALERT_BUL,     // Bus Under Limit
    INA236_ALERT_POL,     // Power Over Limit
    INA236_ALERT_CNVR     // Conversion Ready
} ina236_alert_mode_t;
```

### Device configuration

```c
typedef struct {
    i2c_master_bus_handle_t bus;
    uint8_t dev_addr;
    float r_shunt;
    float current_max;
    bool alert_en;
    uint8_t alert_pin;
    ina236_alert_cb_t alert_cb;
} ina236_config_t;
```

### Alert configuration

```c
typedef struct {
    bool len;
    bool apol;
    ina236_alert_mode_t mode;
    float threshold;
} ina236_config_alert_t;
```

### Functions

```c
esp_err_t ina236_create(ina236_handle_t *handle, const ina236_config_t *config);
esp_err_t ina236_delete(ina236_handle_t handle);

esp_err_t ina236_get_voltage(ina236_handle_t handle, float *volt);
esp_err_t ina236_get_voltage_shunt(ina236_handle_t handle, float *vshunt);
esp_err_t ina236_get_current(ina236_handle_t handle, float *curr);
esp_err_t ina236_get_power(ina236_handle_t handle, float *power);

esp_err_t ina236_set_alert(ina236_handle_t handle, const ina236_config_alert_t *conf);
```

---

## Quick Start

### 1. Create the I2C master bus

The application must create the I2C bus first.

Example:

```c
#include "driver/i2c_master.h"

i2c_master_bus_handle_t bus_handle;

i2c_master_bus_config_t bus_cfg = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
```

---

### 2. Create the INA236 device

```c
#include "ina236.h"

#define INA236_ADDR 0x40
#define INA236_ALERT_GPIO GPIO_NUM_12

static ina236_handle_t ina = NULL;

static void IRAM_ATTR ina_alert_callback(void *arg)
{
    // Optional alert callback
}

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    ina236_config_t cfg = {
        .bus = bus_handle,
        .dev_addr = INA236_ADDR,
        .r_shunt = 0.005f,
        .current_max = 16.0f,
        .alert_en = true,
        .alert_pin = INA236_ALERT_GPIO,
        .alert_cb = ina_alert_callback,
    };

    ESP_ERROR_CHECK(ina236_create(&ina, &cfg));
}
```

---

### 3. Read measurements

```c
float vbus = 0.0f;
float vshunt = 0.0f;
float current = 0.0f;
float power = 0.0f;

ESP_ERROR_CHECK(ina236_get_voltage(ina, &vbus));
ESP_ERROR_CHECK(ina236_get_voltage_shunt(ina, &vshunt));
ESP_ERROR_CHECK(ina236_get_current(ina, &current));
ESP_ERROR_CHECK(ina236_get_power(ina, &power));
```

---

### 4. Configure alerts

```c
ina236_config_alert_t alert_cfg = {
    .len = false,
    .apol = false,
    .mode = INA236_ALERT_BOL,
    .threshold = 15.0f,
};

ESP_ERROR_CHECK(ina236_set_alert(ina, &alert_cfg));
```

---

## Alert Threshold Units

The threshold unit depends on the selected alert mode:

- `INA236_ALERT_SOL`: shunt voltage threshold
- `INA236_ALERT_SUL`: shunt voltage threshold
- `INA236_ALERT_BOL`: bus voltage threshold
- `INA236_ALERT_BUL`: bus voltage threshold
- `INA236_ALERT_POL`: power threshold
- `INA236_ALERT_CNVR`: no threshold register is written

When using `ina236_set_alert()`, pass `threshold` in the physical unit expected by the selected mode.

---

## Notes

- `ina236_create()` computes the current LSB from `current_max / 32768.0f`
- The calibration register is derived from `r_shunt` and `current_max`
- The driver checks the INA236 device ID during initialization
- If the device ID does not match, initialization fails
- If alert mode is enabled and a callback is provided, the ALERT pin is configured as a GPIO interrupt input

---

## Cleanup

When finished:

```c
ESP_ERROR_CHECK(ina236_delete(ina));
```

If your application no longer needs the bus, remove or destroy the I2C bus separately in your application code.

---

## License

This project is licensed under the Apache License 2.0.

Original copyright notices are preserved in source files.  
Modified files are marked accordingly.

---

## Credits

- Original base driver: Espressif Systems
- Adaptation and ESP32 integration: Sylvestre Ibara
