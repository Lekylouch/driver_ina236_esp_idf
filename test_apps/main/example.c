/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * * Modified by Lekylouch, 2026-04-03
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h" // Required for ESP-IDF v5+ I2C API
#include "ina236.h"

static const char *TAG = "DEMO_INA236";

#define I2C_ADDR_INA    0x40
#define ALERT_GPIO_PIN  GPIO_NUM_12 
#define S1              GPIO_NUM_27 
#define S2              GPIO_NUM_26 

// Use the new v5 I2C handle type
i2c_master_bus_handle_t i2c_bus;
ina236_handle_t my_ina;

void IRAM_ATTR alert_callback(void *arg) {
    // Handle alert interrupt here
}

void setup()
{
    // --- New I2C Master Bus Initialization (ESP-IDF v5+) ---
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 22,
        .sda_io_num = 21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus creation failed");
        return;
    }

    // --- INA236 Configuration ---
    ina236_config_t ina_cfg = {
        .bus         = i2c_bus,
        .dev_addr    = I2C_ADDR_INA,
        .r_shunt     = 0.005f,
        .current_max = 16.0f,
        .alert_en    = true,
        .alert_pin   = ALERT_GPIO_PIN,
        .alert_cb    = alert_callback
    };

    ret = ina236_create(&my_ina, &ina_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "INA236 initialization failed");
        return;
    }

    ina236_config_alert_t alert_cfg = {
        .len       = false,             // Latch disabled
        .apol      = false,             // Normal polarity
        .mode      = INA236_ALERT_BOL,  // Bus Over Limit
        .threshold = 15.0f              // 15 Volts
    };

    ina236_set_alert(my_ina, &alert_cfg);

    ESP_LOGI(TAG, "Setup INA236 OK");
}

void app_main() 
{
    gpio_reset_pin(S1); 
    gpio_set_direction(S1, GPIO_MODE_OUTPUT);
    gpio_set_level(S1, 1);
    
    gpio_reset_pin(S2);
    gpio_set_direction(S2, GPIO_MODE_OUTPUT);
    gpio_set_level(S2, 1); 

    // Note: No need to configure ALERT_GPIO_PIN here.
    // The driver (ina236_create) already configures it as an input with interrupts.

    setup();
    
    float v_bus = 0, current = 0, power = 0, v_shunt = 0;

    while (1)
    {
        // Read all measurements
        ina236_get_voltage(my_ina, &v_bus);
        ina236_get_voltage_shunt(my_ina, &v_shunt);
        ina236_get_current(my_ina, &current);
        ina236_get_power(my_ina, &power);

        // Print results (v_shunt uses 6 decimals because values are often in millivolts)
        printf("\n--- MESURES INA236 ---\n");
        printf("VBUS    : %.3f V\n", v_bus);
        printf("VSHUNT  : %.6f V\n", v_shunt); 
        printf("Current : %.3f A\n", current);
        printf("Power   : %.3f W\n", power);
        printf("Alert   : %d\n", gpio_get_level(ALERT_GPIO_PIN));
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}