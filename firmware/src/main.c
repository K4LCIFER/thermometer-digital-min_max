


/* System Defines */
#define __AVR_ATmega328P__
#define F_CPU 8000000UL
#define INND_TS56
/* AVR Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
/* GCC Includes */
#include <stdbool.h>
/* Custom Includes */
#include "seven_segment.h"
/* Program Defines */
// TODO: Need to find a way to clean up the pins and ports with something like
// this.

#define SS_SEGMENTS_DIRECTION_PORT DDRD
#define SS_SEGMENTS_OUTPUT_PORT PORTD

#define SS_DISPLAY_ENABLE_DIRECTION_PORT DDRC
#define SS_DISPLAY_ENABLE_OUTPUT_PORT PORTC
#define SS_DISPLAY_0_ENABLE_PIN 0
#define SS_DISPLAY_1_ENABLE_PIN 1
#define SS_DISPLAY_2_ENABLE_PIN 2
#define SS_DISPLAY_3_ENABLE_PIN 3

#define SENSOR_DATA_DIRECTION_PORT DDRB
#define SENSOR_DATA_OUTPUT_PORT PORTB
#define SENSOR_DATA_INPUT_PORT PINB
#define SENSOR_DATA_PIN 0

// uint16_t received_temperature
// NOTE: volatile because they are being accessed and modified by interrupts.
volatile uint8_t number_of_received_bits = 0;
volatile uint64_t raw_sensor_data = 0;
volatile bool transmission_complete = true;
// uint16_t temperature;
uint8_t parsed_temperature[4];

uint8_t rising_edge_counter = 0;
uint8_t falling_edge_counter = 0;

struct SensorData
{
    uint16_t humidity;
    uint16_t temperature;
    uint8_t checksum_data;
};


void init(void);
uint64_t sensor_rx(void);
void parse_sensor_data(uint64_t data);
// void _display_digit(uint8_t digit);
void display_temperature(uint8_t temperature[]);


// struct ReceivedData;

void main(void)
{
{
    init();

    // The startup time of the sensor appears to be around 750us, but the
    // datasheet also says that it requires 2s between transmissions. The
    // following just prevents any data from being pulled from the sensor right
    // away after startup.
    SS_SEGMENTS_OUTPUT_PORT = 0xFF;
    SS_DISPLAY_ENABLE_OUTPUT_PORT |= (1 << SS_DISPLAY_0_ENABLE_PIN);
    _delay_ms(2000);
    uint8_t d = sensor_rx();
    if (d == 0)
    {
        PORTB |= (1 << DDB7);
    }
    parse_sensor_data(raw_sensor_data);
    display_temperature(parsed_temperature);
    while(1){}
}


void init(void)
{
    // Set all of the segment pins as outputs (they're all on port d)
    SS_SEGMENTS_DIRECTION_PORT = 0b11111111;

    // Set pins controlling the transistor multiplexers as outputs.
    // <display-2><display-1><display-0>
    // Enable Display 0
    SS_DISPLAY_ENABLE_DIRECTION_PORT |= (1 << SS_DISPLAY_0_ENABLE_PIN);
    // Enable Display 1
    SS_DISPLAY_ENABLE_DIRECTION_PORT |= (1 << SS_DISPLAY_1_ENABLE_PIN);
    // Enable Display 2
    SS_DISPLAY_ENABLE_DIRECTION_PORT |= (1 << SS_DISPLAY_2_ENABLE_PIN);
    // (Need a 4th display.)

    // Set up the interrupts:
    sei();  // Enable global interrupts.
    // PCICR |= (1 << PCIE0);  // Enable the interrupts for PCINT[7:0].

    // NOTE: Perhaps only enable the below interrupt, the moment that you are
    // going to use it to prevent unwanted calls.
    PCMSK0 |= (1 << PCINT0);    // Enable the interrupt for pin B1.


    // Set up the timer for use with the receive function:
    // TCCR1B |= (1 << CS11);  // CLK/8 prescaler
    TCCR1B |= (1 << ICES1); // Set ICP1 to capture a falling edge.


    DDRB |= (1 << DDB7); // Test led
}

// struct SensorData sensor_rx(void)
uint64_t sensor_rx(void)
{
    raw_sensor_data = 0;
    number_of_received_bits = 0;
    transmission_complete = false;

    // struct SensorData received_data;
    // Pull low 1ms minimum. This is done first so that the line isn't loaded
    // right away.
    SENSOR_DATA_OUTPUT_PORT &= ~(1 << SENSOR_DATA_PIN);
    // Set the output. Pulling the line low.
    SENSOR_DATA_DIRECTION_PORT |= (1 << SENSOR_DATA_PIN);
    _delay_ms(1);    // Hold the line low for a minimum of 1ms.
    SENSOR_DATA_DIRECTION_PORT &=~ (1 << SENSOR_DATA_PIN);   // Set as input.
    _delay_us(80);   // Wait 80us for the sensor to acknowledge.
    if (SENSOR_DATA_INPUT_PORT & SENSOR_DATA_PIN)   // The sensor is not responding.
    {
        return 0; // Exit out of the function.
    }
    _delay_us(135); // Wait 135us to go into the midle of the start data transmission bit.
    PCICR |= (1 << PCIE0);  // Enable the rising edge interrupt.

    while (!transmission_complete); // Wait here for the interrupt service routine to acquire data.

    return raw_sensor_data;
}

void parse_sensor_data(uint64_t data)
{
    bool is_negative = false;
    // bit shift to the right 8 to remove the checksum, and AND the first 16 bits
    // to remove the humiditiy data leaving the middle 16 bits representing the 
    // temperature.
    uint16_t temperature_data = (data >> 8) & 0x0000FFFF;   // Isolate the temperature bits

    // The first bit of the temperature data represents a negative temperature.
    if (temperature_data & (1 << 15))
    {
        parsed_temperature[3] = 1;
    }
    // Separate the returned temperature into an array of its digits so that it
    // is easier to display them on a seven segment.
    uint8_t temperature_digit_position = 0;
    while (temperature_digit_position <= 2)
    {
        parsed_temperature[temperature_digit_position] = temperature_data % 10;
        temperature_data = temperature_data / 10;
        temperature_digit_position++;
    }
}


void display_temperature(uint8_t* temperature)
{
    switch (temperature[3])
    {
        case 0:
            SS_SEGMENTS_OUTPUT_PORT = SS_0;
            break;
        case 1:
            SS_SEGMENTS_OUTPUT_PORT = SS_1;
            break;
        case 2:
            SS_SEGMENTS_OUTPUT_PORT = SS_2;
            break;
        case 3:
            SS_SEGMENTS_OUTPUT_PORT = SS_3;
            break;
        case 4:
            SS_SEGMENTS_OUTPUT_PORT = SS_4;
            break;
        case 5:
            SS_SEGMENTS_OUTPUT_PORT = SS_5;
            break;
        case 6:
            SS_SEGMENTS_OUTPUT_PORT = SS_6;
            break;
        case 7:
            SS_SEGMENTS_OUTPUT_PORT = SS_7;
            break;
        case 8:
            SS_SEGMENTS_OUTPUT_PORT = SS_8;
            break;
        case 9:
            SS_SEGMENTS_OUTPUT_PORT = SS_9;
            break;
    }
}

// Interrupt Service routine for the start of a data bit
ISR (PCINT0_vect)
{
    rising_edge_counter++;
    // Interrupt flag is automatically cleared when the ISR is called.
    // Start the timer (starts at beginning of the bit)
    TCNT1 = 0;  // Clear the timer.
    TCCR1B |= (1 << CS11);  // Start the timer using CLK/8

    TIMSK1 |= (1 << ICIE1); // Enable the timer capture interrupt.
    PCICR &= ~(1 << PCIE0); // Disable the PCINT0 interrupt.
}

// Interrupt service routine for stopping the timer to catch the end of the bit.
ISR (TIMER1_CAPT_vect)
{
    falling_edge_counter++;
    // float bit_length = ICR1 * TIMER_COUNT_PERIOD;
    if (ICR1 < 45)    // Is a 0
    {
        // Do nothing. Doing nothing is the same as writing a zero.
    }
    else if (ICR1 > 45)   // Is a 1
    {
        raw_sensor_data |= (1 << number_of_received_bits);
    }

    number_of_received_bits++;  // Incrememnt the received bit counter.

    // Continue the capturing if 40 bits have not been acquired yet:
    if (number_of_received_bits < 40)
    {
        TIMSK1 &= ~(1 << ICIE1);    // Disable the input capture interrupt.
        PCICR |= (1 << PCIE0); // Enable the PCINT0 interrupt on pin B0
    }
    else
    {
        // Disable both interrupts:
        TIMSK1 &= ~(1 << ICIE1);
        PCICR &= ~(1 << PCIE0);
        transmission_complete = true;
    }
}


