
#ifndef __ATMEGA328P_H__
#define __ATMEGA328P_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    volatile uint8_t *base_register;
    uint8_t pin_bitmask;
} pin_t;

extern void pin_config_output(pin_t pin);
extern void pin_config_input(pin_t pin);
extern void pin_set_output_value(pin_t pin, bool output_value);
extern bool pin_get_input_value(pin_t pin);
extern void pin_pullup_enable(pin_t pin);
// Apparently these need to be defined here since they are inline functions.
/* extern inline void pin_config_output(pin_t pin)
{
    pin.base_register[1] |= pin.pin_bitmask;
}

extern inline void pin_config_input(pin_t pin)
{
    pin.base_register[1] &= ~pin.pin_bitmask;
}

extern inline void pin_set_output_value(pin_t pin, bool output_value)
{
    if (output_value)
    {
        pin.base_register[2] |= pin.pin_bitmask;
    }
    else
    {
        pin.base_register[2] &= ~pin.pin_bitmask;
    }
}

extern inline bool pin_get_input_value(pin_t pin)
{
    return (pin.base_register[0] & pin.pin_bitmask);
}

extern inline void pin_pullup_enable(pin_t pin)
{
    pin.base_register[0] &= ~(pin.pin_bitmask); // Set as input
    pin.base_register[1] |= (pin.pin_bitmask);  // Output HIGH to enable pullup.

} */

#endif
