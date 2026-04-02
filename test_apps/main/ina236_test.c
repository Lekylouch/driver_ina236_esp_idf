/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Modified by Lekylouch, 2026-04-02
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "ina236.h"


static const char *TAG = "DEMO_INA236";


#define I2C_ADDR_INA    0x40
#define ALERT_GPIO_PIN  GPIO_NUM_12 
#define S1              GPIO_NUM_27 
#define S2              GPIO_NUM_26 


i2c_bus_handle_t i2c_bus;
ina236_handle_t my_ina;


void IRAM_ATTR alert_callback(void *arg) {
   
}

void setup()
{

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed =100000;
    i2c_bus = i2c_bus_create(I2C_NUM_0, &conf);


    ina236_config_t ina_cfg;
    ina_cfg.bus         = i2c_bus;     // ***** 
    ina_cfg.dev_addr    = 0x40;
    ina_cfg.r_shunt     = 0.005f;
    ina_cfg.current_max = 16.0f;
    ina_cfg.alert_en    = true;
    ina_cfg.alert_pin   = ALERT_GPIO_PIN;
    ina_cfg.alert_cb    = alert_callback;


    esp_err_t ret = ina236_create(&my_ina, &ina_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erreur initialisation INA236");
        return;
    }

    ina236_config_alert_t alert_cfg;
    alert_cfg.len       = false;             // Latch desactivé
    alert_cfg.apol      = false;             // Polarité normale
    alert_cfg.mode      = INA236_ALERT_BOL;  // Bus Over Limit
    alert_cfg.threshold = 15.0f;             // 15 Volts


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
    gpio_reset_pin(ALERT_GPIO_PIN);
    gpio_set_direction(ALERT_GPIO_PIN, GPIO_MODE_OUTPUT);
    setup();
    float v_bus = 0, current = 0, power = 0;

   while (1)
   {
    ina236_get_voltage(my_ina, &v_bus);
    ina236_get_current(my_ina, &current);
    ina236_get_power(my_ina, &power);

    printf("\n--- MESURES INA236 ---\n");
    printf("VBUS    : %.3f V\n", v_bus);
    printf("Current : %.3f A\n", current);
    printf("Power   : %.3f W\n", power);
    printf("Alert   : %d\n", gpio_get_level(ALERT_GPIO_PIN));
    
    vTaskDelay(pdMS_TO_TICKS(2000));
}

}