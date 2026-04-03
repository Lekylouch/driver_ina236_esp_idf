/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * Modified by Lekylouch, 2026-04-03
 */

#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"   
#include "driver/i2c_master.h"
#include "ina236.h"
#include "ina_236_reg.h"

static const char *TAG = "INA236";

typedef struct ina236_t {
    i2c_master_dev_handle_t i2c_dev;
    float r_shunt;
    float current_lsb;
    uint8_t alert_pin;
    bool alert_en;
    ina236_alert_cb_t cb;
} ina236_t;

static esp_err_t ina236_write_reg(ina236_t *ina236, uint8_t reg, uint16_t data) {
    uint8_t buffer[3] = { reg, (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) }; 
    return i2c_master_transmit(ina236->i2c_dev, buffer, 3, -1);
}

static esp_err_t ina236_read_reg(ina236_t *ina236, uint8_t reg, uint16_t *data) {
    uint8_t buffer[2] = {0};
    esp_err_t ret = i2c_master_transmit_receive(ina236->i2c_dev, &reg, 1, buffer, 2, -1);
    if (ret == ESP_OK) {
        *data = (buffer[0] << 8) | buffer[1];
    }
    return ret;
}

esp_err_t ina236_create(ina236_handle_t *handle, const ina236_config_t *config) {
    ESP_RETURN_ON_FALSE(handle && config, ESP_ERR_INVALID_ARG, TAG, "Invalid args");

    ina236_t *ina236 = (ina236_t *)calloc(1, sizeof(ina236_t));
    ESP_RETURN_ON_FALSE(ina236, ESP_ERR_NO_MEM, TAG, "No mem");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->dev_addr,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(config->bus, &dev_cfg, &ina236->i2c_dev);
    if (err != ESP_OK) {
        free(ina236);
        return err;
    }

    ina236->r_shunt = config->r_shunt;
    ina236->alert_pin = config->alert_pin;
    ina236->alert_en = config->alert_en;
    ina236->cb = config->alert_cb;

    ina236->current_lsb = config->current_max / 32768.0f;
    uint16_t calib_val = (uint16_t)(0.00512f / (ina236->r_shunt * ina236->current_lsb));

    uint16_t id = 0;
    ina236_read_reg(ina236, INA236_REG_DEVID, &id);
    
    if (id != INA236_DEVICE_ID) {
        ESP_LOGE(TAG, "ID mismatch: 0x%04X (expected 0x%04X)", id, INA236_DEVICE_ID);
        i2c_master_bus_rm_device(ina236->i2c_dev);
        free(ina236);
        return ESP_ERR_NOT_FOUND;
    }

    ina236_write_reg(ina236, INA236_REG_CFG, 0x8000); 
    ina236_write_reg(ina236, INA236_REG_CALIBRATION, calib_val);

    if (ina236->alert_en && ina236->cb) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << ina236->alert_pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };
        gpio_config(&io_conf);
        
        esp_err_t isr_err = gpio_install_isr_service(0);
        if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "Failed to install ISR service: %s", esp_err_to_name(isr_err));
        }
        
        gpio_isr_handler_add(ina236->alert_pin, (gpio_isr_t)ina236->cb, (void*)ina236);
    }

    *handle = (ina236_handle_t)ina236;
    return ESP_OK;
}

esp_err_t ina236_get_voltage(ina236_handle_t handle, float *volt) {
    ina236_t *ina236 = (ina236_t *)handle;
    uint16_t raw;
    esp_err_t ret = ina236_read_reg(ina236, INA236_REG_VBUS, &raw);
    if (ret == ESP_OK) {
        *volt = (float)raw * INA236_VBUS_LSB;
    }
    return ret;
}

esp_err_t ina236_get_voltage_shunt(ina236_handle_t handle, float *vshunt) {
    if (!handle || !vshunt) return ESP_ERR_INVALID_ARG;
    ina236_t *ina236 = (ina236_t *)handle;
    uint16_t raw;
    esp_err_t ret = ina236_read_reg(ina236, INA236_REG_VSHUNT, &raw);
    if (ret == ESP_OK) {
        *vshunt = (float)((int16_t)raw) * INA236_VSHUNT_LSB;
    }
    return ret;
}

esp_err_t ina236_get_current(ina236_handle_t handle, float *curr) {
    ina236_t *ina236 = (ina236_t *)handle;
    uint16_t raw;
    esp_err_t ret = ina236_read_reg(ina236, INA236_REG_CURRENT, &raw);
    if (ret == ESP_OK) {
        *curr = (float)((int16_t)raw) * ina236->current_lsb;
    }
    return ret;
}

esp_err_t ina236_get_power(ina236_handle_t handle, float *power) {
    ina236_t *ina236 = (ina236_t *)handle;
    uint16_t raw;
    esp_err_t ret = ina236_read_reg(ina236, INA236_REG_POWER, &raw);
    if (ret == ESP_OK) {
        *power = (float)raw * 32.0f * ina236->current_lsb;
    }
    return ret;
}

esp_err_t ina236_set_alert(ina236_handle_t handle, const ina236_config_alert_t *conf) {
    ina236_t *ina236 = (ina236_t *)handle;
    ina236_reg_mask_t mask = { .all = 0 };
    uint16_t limit = 0;

    mask.bit.len = conf->len;
    mask.bit.apol = conf->apol;

    switch (conf->mode) {
        case INA236_ALERT_SOL: mask.bit.sol = 1; limit = (uint16_t)(conf->threshold / INA236_VSHUNT_LSB); break;
        case INA236_ALERT_SUL: mask.bit.sul = 1; limit = (uint16_t)(conf->threshold / INA236_VSHUNT_LSB); break;
        case INA236_ALERT_BOL: mask.bit.bol = 1; limit = (uint16_t)(conf->threshold / INA236_VBUS_LSB); break;
        case INA236_ALERT_BUL: mask.bit.bul = 1; limit = (uint16_t)(conf->threshold / INA236_VBUS_LSB); break;
        case INA236_ALERT_POL: mask.bit.pol = 1; limit = (uint16_t)(conf->threshold / (32.0f * ina236->current_lsb)); break;
        case INA236_ALERT_CNVR: mask.bit.cnvr = 1; break;
    }

    ina236_write_reg(ina236, INA236_REG_MASK, mask.all);
    if (conf->mode != INA236_ALERT_CNVR) {
        ina236_write_reg(ina236, INA236_REG_ALERT_LIM, limit);
    }
    return ESP_OK;
}

esp_err_t ina236_delete(ina236_handle_t handle) {
    if (!handle) return ESP_ERR_INVALID_ARG;
    ina236_t *ina236 = (ina236_t *)handle;
    
    if (ina236->alert_en) {
        gpio_isr_handler_remove(ina236->alert_pin);
    }
    
    i2c_master_bus_rm_device(ina236->i2c_dev);
    free(ina236);
    
    return ESP_OK;
}