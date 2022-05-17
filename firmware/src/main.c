
#define INND_TS56

/* System Defines */
#define F_CPU 8000000UL

/* AVR Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
/* GCC Includes */
#include <string.h>
#include <math.h>
#include <stdlib.h>
/* Library Includes */
#include "seven_segment.h"
#include "atmega328p_utils.h"
/* Program Includes */
#include "am2302_utils.h"
#include "pins.h"
#include "display.h"

// NOTE: Doesn't have to be global, but its currently like this for debugging purposes.
uint16_t temp_data; 

volatile uint16_t ovf_counter = 0;

volatile int current_temp;
volatile int max_temp;
volatile int min_temp;


// NOTE: Should probably disable button interrupts when the sensor is receiving
// to prevent interruption. Don't clear the flag, however, so when they get
// reeneabled, their ISR gets immediately called.
void init(void);
uint8_t get_temp(void);
void display_temp(uint16_t temp_data);
void process_buttons(void);

void main(void)
{
    init();
    // Clear the Timer 2 OVF interrupt flag.
    TIFR2 |= (1 << TOV2);
    // Enable the Timer 2 OVF interrupt.
    TIMSK2 |= (1 << TOIE2);
    // Start Timer 2 with CLK/1024.
    TCCR2B |= ((1 << CS22) | (1 << CS21) | (1 << CS20));
    while(1)
    {
        if (ovf_counter >= 100)
        {
            PCICR &= ~(1 << PCIE0); // Disable pushbutton interrupts.
            ovf_counter = 0;
            get_temp();
            if (current_temp > max_temp)
            {
                max_temp = current_temp;
            }
            if (current_temp < min_temp)
            {
                min_temp = current_temp;
            }
            PCICR |= (1 << PCIE0); // Enable pushbutton interrupts.
        }
    }
}

void init(void)
{

    // Reduce power consumption:
    PRR |= (1 << PRADC);    // Disables the ADC.
    PRR |= (1 << PRUSART0); // Disables the USART0
    PRR |= (1 << PRSPI);    // Disables the SPI
/*     PRR |= (1 << PRTIM0);   // Disables the Timer0 */
    PRR |= (1 << PRTWI);    // Disables the TWI
    SMCR |= ((1 << SM1) | (1 << SM0));  // Power save mode
    SMCR |= (1 << SE);  // Enable sleep.
    sleep_bod_disable();    // Disable BOD.

    // Configure the buttons:
    // Set the button pins as inputs, and enable their pull-ups:
    // Reset saved temperatures button.
    pin_config_input(reset_saved_temps_btn_pin);
    pin_pullup_enable(reset_saved_temps_btn_pin);
    // Display minimum temperature button.
    pin_config_input(disp_min_temp_btn_pin);
    pin_pullup_enable(disp_min_temp_btn_pin);
    // Display maximum temperature button.
    pin_config_input(disp_max_temp_btn_pin);
    pin_pullup_enable(disp_max_temp_btn_pin);
    // Display current temperature button.
    pin_config_input(disp_current_temp_btn_pin);
    pin_pullup_enable(disp_current_temp_btn_pin);

    sensor_init();
    display_init();


    sei();  // Enable global interrupts.

    // Initialize values:
    get_temp();
    max_temp = current_temp;
    min_temp = current_temp;

    // Set up the interrupts for hte buttons:
    // Enable interrupts for the 4 pushbuttons on their respective pins.
    PCMSK0 |= ((1 << PCINT1) | (1 << PCINT2) | (1 << PCINT3) | (1 << PCINT4));
    PCIFR |= (1 << PCIF0); // Ensure that the button interrupt is cleared.
    PCICR |= (1 << PCIE0); // Enable pushbutton interrupts.

    DDRB |= (1 << DDB7); // Test LED.
}


uint8_t get_temp(void)
{
    sensor_rx(sensor_data_pin);    // Retreive data from the sensor.
    
    // Extract the temperature digits.
    /*uint16_t*/ temp_data = (raw_sensor_data & TEMP_DATA_BITMASK) 
        >> TEMP_DATA_OFFSET;
    current_temp = 1;   // Allows for easy multiplication.
    
    // Check if there if the negative temperature flag is set:
    if (raw_sensor_data & NEG_TEMP_BITMASK)
    {
        current_temp *= -1; // Floating point value of the temperature.
    }
    // Udate the current temperature value.
    current_temp *= temp_data;
    
    return 0;
}

// TODO: Add the a timer overflow catcher in case of a failed transmission.

// ISR for buttons:
ISR (PCINT0_vect)
{
    // Stop the temperature retreival timer.
    TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
    if (!pin_get_input_value(disp_current_temp_btn_pin))
    {
        if (current_temp < 0)
        {
            display_buffer[0] = SS_NEG;
        }
        for (uint8_t i = 0; i < 3; i++)
        {
            // Extract the digit.
            display_buffer[i] = ss_display_table[
                abs(current_temp / (int)pow(10,i)) % 10
            ];
        }
        display_buffer[1] |= SS_DEC;
        // Enable the display
        display_enable();
        // Reset the timer.
        TCNT1 = 0;
        // Clear the compare match A interrupt flag.
        TIFR1 |= (1 << OCF1A);
        // Interrupt on compare match.
        TIMSK1 |= (1 << OCIE1A);
        // Compare match at 5s (~39063 cycles)
        OCR1A = 39063; // 5s * (8MHz / 1024)
        // Enable the timer with CLK/1024
        TCCR1B |= ((1 << CS12) | (1 << CS10));
    }
    else if (!pin_get_input_value(disp_max_temp_btn_pin))
    {
        if (max_temp < 0)
        {
            display_buffer[0] = SS_NEG;
        }
        for (uint8_t i = 0; i < 3; i++)
        {
            // Extract the digit.
            display_buffer[i] = ss_display_table[
                abs(max_temp / (int)pow(10,i)) % 10
            ];
        }
        display_buffer[1] |= SS_DEC;
        // Enable the display
        display_enable();
        // Reset the timer.
        TCNT1 = 0;
        // Clear the compare match A interrupt flag.
        TIFR1 |= (1 << OCF1A);
        // Interrupt on compare match.
        TIMSK1 |= (1 << OCIE1A);
        // Compare match at 5s (~39063 cycles)
        OCR1A = 39063; // 5s * (8MHz / 1024)
        // Enable the timer with CLK/1024
        TCCR1B |= ((1 << CS12) | (1 << CS10));
    }
    else if (!pin_get_input_value(disp_min_temp_btn_pin))
    {
        if (min_temp < 0)
        {
            display_buffer[0] = SS_NEG;
        }
        for (uint8_t i = 0; i < 3; i++)
        {
            // Extract the digit.
            display_buffer[i] = ss_display_table[
                abs(min_temp / (int)pow(10,i)) % 10
            ];
        }
        display_buffer[1] |= SS_DEC;
        // Enable the display
        display_enable();
        // Reset the timer.
        TCNT1 = 0;
        // Clear the compare match A interrupt flag.
        TIFR1 |= (1 << OCF1A);
        // Interrupt on compare match.
        TIMSK1 |= (1 << OCIE1A);
        // Compare match at 5s (~39063 cycles)
        OCR1A = 39063; // 5s * (8MHz / 1024)
        // Enable the timer with CLK/1024
        TCCR1B |= ((1 << CS12) | (1 << CS10));
    }
    else if (!pin_get_input_value(reset_saved_temps_btn_pin))
    {
        max_temp = current_temp;
        min_temp = current_temp;
    }
}

// Display timeout ISR. (Turns off the display after 5s).
ISR (TIMER1_COMPA_vect)
{
    // Turn off the display.
    display_disable();
    // Stop the timer.
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
    // Disable the timer interrupts.
    TIMSK1 &= ~(1 << OCIE1A);
    // Restart the temperature retreival timer.
    TCCR2B |= ((1 << CS22) | (1 << CS21) | (1 << CS20));
}

ISR (TIMER2_OVF_vect)
{
    ovf_counter++;
}



