# INA236 ESP32 Driver — ESP-IDF Component

[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](license.txt)

> **Modified version** of the original INA236 ESP-IDF component driver.  
> Modifications by **Sylvestre Ibara** (2026) — adapted and tested on ESP32 with PlatformIO.  
> Original copyright notices preserved in source files. See [license.txt](license.txt) for details.

---

## Overview

The INA236 is a 16-bit digital current monitor with an I2C interface, compatible with
1.2 V, 1.8 V, 3.3 V, and 5.0 V bus voltages. It measures current, bus voltage and power
through an external shunt resistor.

This ESP-IDF component provides:
- Voltage, current and power reading via I2C
- Configurable shunt resistor and current range
- Programmable alert (over-voltage, over-current, etc.) with GPIO callback

---

## Repository Structure

```
driver_ina236_esp_idf/
├── ina236.c                  # Driver source
├── include/
│   └── ina236.h              # Driver header (API)
├── test_apps/
│   └── main/
│       └── ina236_test.c     # Full tested example (see Quick Start below)
├── CMakeLists.txt
├── idf_component.yml
└── license.txt
```

---

## Hardware Setup

| INA236 Pin | ESP32 GPIO | Description        |
|------------|------------|--------------------|
| SDA        | GPIO21     | I2C Data           |
| SCL        | GPIO22     | I2C Clock          |
| ALERT      | GPIO12     | Alert interrupt    |
| VCC        | 3.3V       | Power supply       |
| GND        | GND        | Ground             |

> No external pull-up resistors needed — internal pull-up resistors are enabled by the driver.

---

## Installation

Copy this folder into your project's `components/` directory:

```
your_project/
└── components/
    └── ina236/          ← this repo
        ├── ina236.c
        ├── include/
        │   └── ina236.h
        └── CMakeLists.txt
```

Then in your `main.c`:

```c
#include "ina236.h"
```

---

## Quick Start

A full working example is available in [`test_apps/main/ina236_test.c`](test_apps/main/ina236_test.c).

Here is a minimal usage:

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "ina236.h"

#define I2C_ADDR_INA    0x40
#define ALERT_GPIO_PIN  GPIO_NUM_12

i2c_bus_handle_t i2c_bus;
ina236_handle_t my_ina;

void IRAM_ATTR alert_callback(void *arg) {
    // Handle alert interrupt here
}

void app_main() {
    // Step 1: Init I2C bus
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    i2c_bus = i2c_bus_create(I2C_NUM_0, &conf);

    // Step 2: Init INA236
    ina236_config_t ina_cfg;
    ina_cfg.bus         = i2c_bus;
    ina_cfg.dev_addr    = I2C_ADDR_INA;
    ina_cfg.r_shunt     = 0.005f;   // 5 mOhm shunt resistor
    ina_cfg.current_max = 16.0f;    // 16 A max
    ina_cfg.alert_en    = true;
    ina_cfg.alert_pin   = ALERT_GPIO_PIN;
    ina_cfg.alert_cb    = alert_callback;
    ina236_create(&my_ina, &ina_cfg);

    // Step 3: Configure alert (Bus Over Limit at 15V)
    ina236_config_alert_t alert_cfg;
    alert_cfg.len       = false;
    alert_cfg.apol      = false;
    alert_cfg.mode      = INA236_ALERT_BOL;
    alert_cfg.threshold = 15.0f;
    ina236_set_alert(my_ina, &alert_cfg);

    // Step 4: Read measurements
    float voltage = 0, current = 0, power = 0;
    while (1) {
        ina236_get_voltage(my_ina, &voltage);
        ina236_get_current(my_ina, &current);
        ina236_get_power(my_ina, &power);

        printf("VBUS    : %.3f V\n", voltage);
        printf("Current : %.3f A\n", current);
        printf("Power   : %.3f W\n", power);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

---

## API Reference

```c
// Create and initialize the INA236 handle
esp_err_t ina236_create(ina236_handle_t *handle, const ina236_config_t *cfg);

// Configure programmable alert
esp_err_t ina236_set_alert(ina236_handle_t handle, const ina236_config_alert_t *cfg);

// Read measurements
esp_err_t ina236_get_voltage(ina236_handle_t handle, float *voltage_v);
esp_err_t ina236_get_current(ina236_handle_t handle, float *current_a);
esp_err_t ina236_get_power(ina236_handle_t handle, float *power_w);

// Delete handle and free resources
esp_err_t ina236_delete(ina236_handle_t handle);
```

---

## License

This project is licensed under the **Apache License 2.0** — see [license.txt](license.txt) for full details.

**As required by the Apache License 2.0 (Section 4):**
- Original copyright notices are preserved in source files
- Modified files are clearly marked with the author’s name and modification date
- This is a derivative work — see source file headers for original attribution

**You are free to:** use, copy, modify and distribute this driver, including in commercial projects.

**You must:** keep original copyright notices, include a copy of the Apache 2.0 License, and clearly state your modifications.

---

## Credits

- Original driver: ESP-IDF INA236 component (Apache 2.0)
- Modifications & testing: Sylvestre Ibara — ESP32 adaptation (2026)
