
#ifndef __ATMEGA328P_H__
#define __ATMEGA328P_H__

#include <stdint.h>
#include <stdbool.h>

extern typedef struct
{
/*     volatile uint8_t *input_register;
    volatile uint8_t *direction_register;
    volatile uint8_t *output_register; */
    volatile uint8_t base_register;
    uint8_t pin_bitmask;
} pin_t;

extern inline void pin_set_output(pin_t pin);
extern inline void pin_set_input(pin_t pin);
extern inline void pin_output_high(pin_t pin);
extern inline void pin_output_low(pin_t pin);
extern inline bool pin_get_input_state(pin_t pin);
extern inline void pin_pullup_enable(pin_t pin);

#endif
