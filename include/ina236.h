/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Modified by Lekylouch, 2026-04-02
 */


#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "i2c_bus.h"

#ifdef __cplusplus

extern "C" {
#endif

#define INA236_I2C_ADDRESS_DEFAULT   (0x40)


typedef struct ina236_t *ina236_handle_t;
typedef void (*ina236_alert_cb_t)(void *arg);

typedef enum {
    INA236_ALERT_SOL = 0, // Shunt Over Limit
    INA236_ALERT_SUL,     // Shunt Under Limit
    INA236_ALERT_BOL,     // Bus Over Limit
    INA236_ALERT_BUL,     // Bus Under Limit
    INA236_ALERT_POL,     // Power Over Limit
    INA236_ALERT_CNVR      // Conversion Ready
} ina236_alert_mode_t;


typedef struct {
    i2c_bus_handle_t bus;
    uint8_t dev_addr;
    float r_shunt;        // Valeur en Ohms (ex: 0.1)
    float current_max;    // Courant max attendu en Ampères
    bool alert_en;
    uint8_t alert_pin;
    ina236_alert_cb_t alert_cb;
} ina236_config_t;

typedef struct {
    bool len;             // Latch enable
    bool apol;            // Alert polarity
    ina236_alert_mode_t mode;
    float threshold;      // Seuil en V, A ou W selon le mode
} ina236_config_alert_t;



esp_err_t ina236_create(ina236_handle_t *handle, const ina236_config_t *config);
esp_err_t ina236_delete(ina236_handle_t handle);
esp_err_t ina236_get_voltage(ina236_handle_t handle, float *volt);
esp_err_t ina236_get_current(ina236_handle_t handle, float *curr);
esp_err_t ina236_get_power(ina236_handle_t handle, float *power);
esp_err_t ina236_get_voltage_shunt(ina236_handle_t handle, float *vshunt);
esp_err_t ina236_set_alert(ina236_handle_t handle, const ina236_config_alert_t *conf);

#ifdef __cplusplus
}
#endif
