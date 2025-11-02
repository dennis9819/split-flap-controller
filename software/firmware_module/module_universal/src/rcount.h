/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */

#include "global.h"

#define RC_BASEADDR 0x100

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void incrementCounter();
uint32_t rc_getCounter();
void rc_tick();
#ifdef __cplusplus
}
#endif // __cplusplus
