
#ifndef __PINS_H__
#define __PINS_H__

#include <avr/io.h>
#include "atmega328p.h"

extern const pin_t sensor_data_pin;
extern const pin_t ss_segment_pins;
extern const pin_t ss0_enable_pin;
extern const pin_t ss1_enable_pin;
extern const pin_t ss2_enable_pin;
extern const pin_t ss3_enable_pin;
extern const pin_t reset_saved_temps_btn_pin;
extern const pin_t disp_min_temp_btn_pin;
extern const pin_t disp_max_temp_btn_pin;
extern const pin_t disp_current_temp_btn_pin;

#endif