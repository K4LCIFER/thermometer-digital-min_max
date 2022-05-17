
#include "atmega328p_utils.h"
#include <stdint.h>
#include <stdbool.h>


void pin_config_output(pin_t pin)
{
    pin.base_register[1] |= pin.pin_bitmask;
}

void pin_config_input(pin_t pin)
{
    pin.base_register[1] &= ~pin.pin_bitmask;
}

void pin_set_output_value(pin_t pin, bool output_value)
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

bool pin_get_input_value(pin_t pin)
{
    return (pin.base_register[0] & pin.pin_bitmask);
}

void pin_pullup_enable(pin_t pin)
{
    pin.base_register[1] &= ~(pin.pin_bitmask); // Set as input
    pin.base_register[2] |= (pin.pin_bitmask);  // Output HIGH to enable pullup.

}