#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void incrementCounter();
uint32_t rc_getCounter();
void rc_tick();
#ifdef __cplusplus
}
#endif // __cplusplus

