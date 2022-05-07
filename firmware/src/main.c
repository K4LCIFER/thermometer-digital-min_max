


/* System Defines */
#define __AVR_ATmega328P__
#define F_CPU 8000000UL
/* AVR Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

typedef struct
{
    uint8_t *input_register;
    uint8_t *direction_register;
    uint8_t *output_register;
    uint8_t pin_bitmask;    // Represents the pin on the port.
} pin_t;

// PB1 is the data line for the sensor.
static const pin_t sensor_data_pin = {
    .direction_register = &DDRB,
    .output_register = &PORTB,
    .input_register=&PINB,
    .pin_bitmask = 1 << 0 
};

union SensorData
{
    uint64_t raw_sensor_data;
    struct
    {
        uint16_t humidity_data;
        uint16_t temperature_data;
    };
} sensor_data;

volatile uint8_t number_of_received_bits = 0;

void init(void);
uint8_t sensor_rx(void);

void main(void)
{
    init();

    _delay_ms(2000);    // Sensor bootup delay.

    sensor_rx();

    while(1){};
}


void init(void)
{
    // Set up the interrupts:
    sei();  // Enable global interrupts.
    DDRB |= (1 << DDB7); // Test led
}

uint8_t sensor_rx(void)
{
    number_of_received_bits = 0;

    // Pull the line low for a minimum of 1ms to initiate the transaction with
    // the sensor. Set the output register before the direction register to
    // prevent line loading?
    *sensor_data_pin.output_register &= ~sensor_data_pin.pin_bitmask;
    *sensor_data_pin.direction_register |= sensor_data_pin.pin_bitmask;
    _delay_ms(1);
    // Set as input (also passively pulls the line high through the pull-up):
    *sensor_data_pin.direction_register &=~ sensor_data_pin.pin_bitmask;
    _delay_us(80);   // Wait 80us for the sensor to acknowledge.
    // If the sensor does not bring the line low (not respdonding), then cancel
    // then transmission.
    if (*sensor_data_pin.input_register & sensor_data_pin.pin_bitmask)
    {
        return 1;   // Return error flag.
    }
    // Enable the timer capture interrupt to capture on a falling edge:
    TIFR1 |= (1 << ICF1);   // Clear the interrupt flag.
    TIMSK1 |= (1 << ICIE1); // Enable the capture interrupt.

    // Wait to acquire all the data from the sensor:
    while (number_of_received_bits < 41);

    // Verify the data with the checksum:
    uint8_t *raw_sensor_data_ptr = &sensor_data.raw_sensor_data;
    uint8_t checksum = raw_sensor_data_ptr[0];
    uint8_t sum = 0;
    for (uint8_t i = 1; i < 8; i++)
    {
        sum += raw_sensor_data_ptr[i];
    }
    if (sum != checksum)    // Bad data.
    {
        return 1;   // Return error flag.
    }
    // Parse the received sensor data:
    // Separate the humidity data:
    // NOTE: The temporary variables prevent overwriting of the union's data
    // until it is actually desired. Perhaps it would be better to just not
    // worry about wasting excess memory, and just use a single struct? Doing
    // that would be a simpler route.
    uint16_t humidity_data
        = (sensor_data.raw_sensor_data & 0xFFFF000000) >> 24;
    uint16_t temperature_data
        = (sensor_data.raw_sensor_data & 0x0000FFFF00) >> 8;
    sensor_data.humidity_data = humidity_data;
    sensor_data.temperature_data = temperature_data;

    return 0;
}

// Only need to time the period since the period is just 50us + the bit length.
// Interrupt service routine for stopping the timer to catch the end of the bit.
ISR (TIMER1_CAPT_vect)
{
    static uint16_t previous_timer_value;
    // Kickstart the bit stream with the falling edge of the start bit:
    if (number_of_received_bits == 0)
    {
        TCNT1 = 0;  // Clear the timer.
        TCCR1B |= (1 << CS11);  // Start the timer with CLK/8.
        previous_timer_value = 0;
        number_of_received_bits++;
    }
    else
    {
        uint16_t current_timer_value = ICR1;
        uint8_t bit_period = current_timer_value - previous_timer_value;

        previous_timer_value = current_timer_value;

        // Store the received bit.
        sensor_data.raw_sensor_data
            = (sensor_data.raw_sensor_data << 1) | (bit_period > 100);

        number_of_received_bits++;
        // If all the bits are received, then stop receiving.
        if (number_of_received_bits >= 41)
        {
            // Stop the timer.
            TIMSK1 &= ~(1 << ICIE1);    // Disable the timer capture interrupt.
        }
    }
}


