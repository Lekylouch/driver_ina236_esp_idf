/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Modified by Lekylouch, 2026-04-02
 */


#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Adresses des registres
typedef enum {
    INA236_REG_CFG          = 0x00,
    INA236_REG_VSHUNT       = 0x01,
    INA236_REG_VBUS         = 0x02,
    INA236_REG_POWER        = 0x03,
    INA236_REG_CURRENT      = 0x04,
    INA236_REG_CALIBRATION  = 0x05,
    INA236_REG_MASK         = 0x06,
    INA236_REG_ALERT_LIM    = 0x07,
    INA236_REG_MAF_ID       = 0x3E,
    INA236_REG_DEVID        = 0x3F,
} ina236_reg_addr_t;



// Constantes LSB fixes (Datasheet)
#define INA236_VSHUNT_LSB   2.5e-6f  // 2.5 µV
#define INA236_VBUS_LSB     1.6e-3f  // 1.6 mV
#define INA236_DEVICE_ID    0xA080

typedef union {
    struct __attribute__((packed)) {
        uint16_t mode: 3;      
        uint16_t vshct: 3;     
        uint16_t vbusct: 3;    
        uint16_t avg: 3;       
        uint16_t adcrange: 1;  
        uint16_t reserved: 2;
        uint16_t rst: 1;
    } bit;
    uint16_t all;
} ina236_reg_cfg_t;

typedef union {
    struct __attribute__((packed)) {
        uint16_t len:       1; 
        uint16_t apol:      1; 
        uint16_t ovf:       1; 
        uint16_t cvrf:      1; 
        uint16_t aff:       1; 
        uint16_t memerror:  1; 
        uint16_t reserved:  4; 
        uint16_t cnvr:      1; 
        uint16_t pol:       1; 
        uint16_t bul:       1; 
        uint16_t bol:       1; 
        uint16_t sul:       1; 
        uint16_t sol:       1; 
    } bit;
    uint16_t all;
} ina236_reg_mask_t;

#ifdef __cplusplus
}
#endif