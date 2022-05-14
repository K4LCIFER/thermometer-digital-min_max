
#include "atmega328p.h"

extern inline void pin_set_output(pin_t pin)
{
/*     *pin.direction_register |= pin.pin_bitmask; */
    pin.base_register[1] |= pin.pin_bitmask;
}

extern inline void pin_set_input(pin_t pin)
{
/*     *pin.direction_register &= ~pin.pin_bitmask; */
    pin.base_register[1] &= ~pin.pin_bitmask;
}

extern inline void pin_output_high(pin_t pin)
{
/*     *pin.output_register |= pin.pin_bitmask; */
    pin.base_register[2] |= pin.pin_bitmask;
}

extern inline void pin_output_low(pin_t pin)
{
/*     *pin.output_register &= ~pin.bitmask; */
    pin.base_register[2] &= ~pin.pin_bitmask;
}

extern inline bool pin_get_input_state(pin_t pin)
{
/*     return *pin.input_register & pin.pin_bitmask; */
    return (pin.base_register[0] & pin.pin_bitmask)
}

extern inline void pin_pullup_enable(pin_t pin)
{
    pin.base_register[0] &= ~(pin.pin_bitmask); // Set as input
    pin.base_register[1] |= (pin.pin_bitmask);  // Output HIGH to enable pullup.

}
