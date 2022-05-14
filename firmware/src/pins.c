
#include <avr/io.h>
#include "pins.h"
#include "atmega328p.h"

extern const pin_t sensor_data_pin = {&PINB, 1 << 0};
extern const pin_t ss_segment_pins = {&PIND, 0xFF};
extern const pin_t ss0_enable_pin = {&PINC, 1 << 0};
extern const pin_t ss1_enable_pin = {&PINC, 1 << 1};
extern const pin_t ss2_enable_pin = {&PINC, 1 << 2};
extern const pin_t ss3_enable_pin = {&PINC, 1 << 3};
extern const pin_t reset_saved_temps_btn_pin = {&PINB, 1 << 1};
extern const pin_t disp_min_temp_btn_pin = {&PINB, 1 << 2};
extern const pin_t disp_max_temp_btn_pin = {&PINB, 1 << 3};
extern const pin_t disp_current_temp_btn_pin = {&PINB, 1 << 4};