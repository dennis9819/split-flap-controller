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

#define STEPS_PER_REV 2025  // steps per revolution
#define STEPS_PER_FLAP 45   // steps per flap
#define STEPS_ADJ 0         // added per flap to compensate for motor power down
#define STEPS_OFFSET_DEF 1400   // ansolute offset between home and first flap
#define AMOUNTFLAPS 45      // amount of flaps installed in system
#define STEPS_AFTERROT 255  // value to goto after current target flap is reached
#define ERROR_DATASETS 8    // length of error array

#define MDELAY_STARTUP 1000 // delay to wait after motor startup
#define MHOME_TOLERANCE 1.5 // tolerance for intial homing procedure
#define MHOME_ERRDELTA 30   // maximum deviation between expected home and actual home
#define MVOLTAGE_FAULTRD 20 // max. amount of fault readings before flag is set
#define MVOLTAGE_LSTOP 128  // lower voltage threshold for fuse detection
#define MPWRSVG_TICKSTOP 50 // inactive ticks before motor shutdown

#define MISR_OCR1A 580      // tick timer (defines rotation speed)
// 450, 480 also possible ?

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void mctrl_init(int cal_offset);
void mctrl_step();
void mctrl_set(uint8_t flap, uint8_t fullRotation);

void getErr(int16_t* error);
uint8_t getSts();
uint16_t getVoltage();
void mctrl_power(uint8_t state);

#ifdef __cplusplus
}
#endif // __cplusplus
