#pragma once

#define MP_A PC0
#define MP_B PC1
#define MP_C PC2
#define MP_D PC3

#define STEPS_PRE_REV 2025
#define STEPS_PRE_FLAP 45
#define STEPS_ADJ 0       // added per flap to compensate for motor power down
#define AMOUNTFLAPS 45

#define ERROR_DATASETS 8

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "rcount.h"

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