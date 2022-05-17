

#define INND_TS56
#include <avr/io.h>
#include <avr/interrupt.h>
#include "seven_segment.h"
#include "display.h"
#include "pins.h"

volatile uint8_t display_buffer[4];// = {SS_0, SS_0, SS_0, SS_0};

const uint8_t ss_display_table[10] = {
    SS_0,
    SS_1,
    SS_2,
    SS_3,
    SS_4,
    SS_5,
    SS_6,
    SS_7,
    SS_8,
    SS_9,
};

void display_init(void)
{
    // Set the seven segment pins as outputs:
    for (uint8_t i = 0; i < NUMBER_OF_SS_SEGMENT_PINS; i++)
    {
        pin_config_output(ss_segment_pins[i]);
    }
    // Set the display_enable_pins as outputs:
    for (uint8_t i = 0; i < NUMBER_OF_SS_ENABLE_PINS; i++)
    {
        pin_config_output(ss_enable_pins[i]);
    }
    // Put the timer0 in CTC (Clear timer on compare match) mode (mode 2).
    TCCR0A |= (1 << WGM01);
    OCR0A = 125;    // Generates a 4ms delay for a total refresh rate of 60Hz.
}

void display_enable(void)
{
    TIFR0 |= (1 << OCF0A);  // Clear the Compare Match A interrupt flag.
    TIMSK0 |= (1 << OCIE0A);    // Enable the Compare Match A interrupt.
    TCCR0B |= (1 << CS02);  // Start the timer with CLK/256
}

void display_disable(void)
{
    // Clear the digit from the segment bus:
    for (uint8_t i = 0; i < NUMBER_OF_SS_SEGMENT_PINS; i++)
    {
        pin_set_output_value(ss_segment_pins[i], 0);
    }
    // Disable each display:
    for (uint8_t i =0; i < NUMBER_OF_SS_ENABLE_PINS; i++)
    {
        pin_set_output_value(ss_enable_pins[i], 0);
    }
    TIMSK0 &= ~(1 << OCIE0A);   // Disable the Compare Match A interrupt.
    TCCR0B &= ~(1 << CS02);     // Stop the timer
}

// Display refresh ISR:
ISR (TIMER0_COMPA_vect)
{
    // Keep in mind how many cycles this all takes to execute. This reduces the 
    // Effective minimum delay that is possible with this ISR.

    static uint8_t display_counter;

    // Clear the previous display.
    pin_set_output_value(ss_enable_pins[display_counter], 0);
    // Go to the next digit:
    if (display_counter == 3)
    {
        display_counter = 0;
    }
    else
    {
        display_counter++;
    }
    // Output the segment signals:
    for (uint8_t i = 0; i < NUMBER_OF_SS_SEGMENT_PINS; i++)
    {
        pin_set_output_value
        (
            ss_segment_pins[i],
            display_buffer[display_counter] & (1 << i)
        );
    } 
    // Enable the digit's display.
    pin_set_output_value(ss_enable_pins[display_counter], 1);
}