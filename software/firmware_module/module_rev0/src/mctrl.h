/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */


#include "global.h"
#include "rcount.h"

#pragma once


#define STEPS_PER_REV 2025
#define STEPS_PER_FLAP 45
#define STEPS_ADJ 0       // added per flap to compensate for motor power down
#define AMOUNTFLAPS 45

#define ERROR_DATASETS 8

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void mctrl_init();
void mctrl_step();
void mctrl_set(uint8_t flap, uint8_t fullRotation);

void getErr(int16_t* error);
uint8_t getSts();
uint16_t getVoltage();
void mctrl_power(uint8_t state);

#ifdef __cplusplus
}
#endif // __cplusplus
