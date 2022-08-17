
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>


#define NUMBER_OF_SS_ENABLE_PINS 4
#define NUMBER_OF_SS_SEGMENT_PINS 8
#define DISPLAY_0 0
#define DISPLAY_1 1
#define DISPLAY_2 2
#define DISPLAY_3 3

extern volatile uint8_t display_buffer[4];
extern const uint8_t ss_display_table[10];

extern void display_init(void);
extern void display_enable(void);
extern void display_disable(void);

#endif