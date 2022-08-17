
#define F_CPU 8000000UL

#include "am2302_utils.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include "atmega328p_utils.h"

volatile uint8_t number_of_captures;
volatile uint64_t raw_sensor_data;
volatile uint16_t times[40];    // Only for debugging purposes.

void sensor_init(void)
{
    _delay_ms(2000);    // Sensor bootup delay.
}

uint8_t sensor_rx(pin_t pin)
{
    number_of_captures = 0;

    // Pull the line low for a minimum of 1ms to initiate the transaction with
    // the sensor. Set the output register before the direction register to
    // prevent line loading?
    pin_set_output_value(pin, 0);
    pin_config_output(pin);
    _delay_ms(1);
    // Set as input (also passively pulls the line high through the pull-up):
    pin_config_input(pin);
    _delay_us(80);   // Wait 80us for the sensor to acknowledge.
    // If the sensor does not bring the line low (not respdonding), then cancel
    // then transmission.
    if (pin_get_input_value(pin))
    {
        return 1;   // Return error flag.
    }
    // Enable the timer capture interrupt to capture on a falling edge:
    TIFR1 |= (1 << ICF1);   // Clear the interrupt flag.
    TIMSK1 |= (1 << ICIE1); // Enable the capture interrupt.

    // Wait to acquire all the data from the sensor:
    while (number_of_captures < 41);
    TIMSK1 &= ~(1 << ICIE1);    // Disable the timer capture interrupt.
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (CS10));    // Stop the timer.

    // Verify the data with the checksum:
    // The checksum is the last byte (LSB) of the raw sensor data.
    uint8_t checksum = ((uint8_t *)&raw_sensor_data)[0];
    // Calculate the sum to verify against the checksum.
    uint8_t sum = 0;
    for (uint8_t i = 1; i < 8; i++)
    {
        // Step along each byte in the raw sensor data (excluding the checksum).
        sum = sum + ((uint8_t*)&raw_sensor_data)[i];
    }
    if ((uint8_t)sum != checksum)    // Bad data.
    {
        return 1;   // Return error flag.
    }
    else
    {
        return 0;   // Return success.
    }
}

// Only need to time the period since the period is just 50us + the bit length.
// Interrupt service routine for stopping the timer to catch the end of the bit.
ISR (TIMER1_CAPT_vect)
{
    static uint16_t previous_timer_value;
    // Kickstart the bit stream with the falling edge of the start bit:
    if (number_of_captures == 0)
    {
        TCNT1 = 0;  // Clear the timer.
        // Reset the clock prescaler/stop the timer.
        TCCR1B &= ~((1 << CS12) | (1 << CS11) | (CS10));
        TCCR1B |= (1 << CS11);  // Start the timer with CLK/8.
        previous_timer_value = 0;
        number_of_captures++;
    }
    else
    {
        uint16_t current_timer_value = ICR1;
        uint8_t bit_period = current_timer_value - previous_timer_value;
        times[number_of_captures-1] = bit_period;

        previous_timer_value = current_timer_value;

        // Store the received bit.
        raw_sensor_data = (raw_sensor_data << 1) 
            | (bit_period > BIT_PERIOD_THRESHOLD);

        number_of_captures++;
        // If all the bits are received, then stop receiving.
    }
}



