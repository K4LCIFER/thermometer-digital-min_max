
#include "pins.h"
#include <avr/io.h>
#include "atmega328p_utils.h"

const pin_t sensor_data_pin = {&PINB, 1 << 0};
const pin_t ss_segment_pins[] = {
    {&PIND, 1 << 0},    // Segment A.
    {&PIND, 1 << 1},    // Segment B.
    {&PIND, 1 << 2},    // Segment C.
    {&PIND, 1 << 3},    // Segment D.
    {&PIND, 1 << 4},    // Segment E.
    {&PIND, 1 << 5},    // Segment F.
    {&PIND, 1 << 6},    // Segment G.
    {&PIND, 1 << 7},    // Segment H (DP).
};
const pin_t ss_enable_pins[] = {
    {&PINC, 1 << 0},    // Display/Digit 0 (LSB).
    {&PINC, 1 << 1},    // Display/Digit 1.
    {&PINC, 1 << 2},    // Display/Digit 2.
    {&PINC, 1 << 3},    // Display/Digit 3 (MSB).
};
const pin_t reset_saved_temps_btn_pin = {&PINB, 1 << 1};
const pin_t disp_min_temp_btn_pin = {&PINB, 1 << 2};
const pin_t disp_max_temp_btn_pin = {&PINB, 1 << 3};
const pin_t disp_current_temp_btn_pin = {&PINB, 1 << 4};