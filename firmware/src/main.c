


/* System Defines */
#define __AVR_ATmega328P__
#define F_CPU 8000000UL
/* AVR Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define SENSOR_DATA_DIRECTION_PORT DDRB
#define SENSOR_DATA_OUTPUT_PORT PORTB
#define SENSOR_DATA_INPUT_PORT PINB
#define SENSOR_DATA_PIN 0

//base[0] = PIN, base[1] = DDR, base[2] = PORT, pinbm = pin bitmask
typedef struct
{
    uint8_t* register;
    uint8_t bitmask;
} pin_t;

//pins
static const pin_t sensor_pin = { &PINB, 1 << 0 }; //PB1

volatile uint8_t number_of_received_bits = 0;
volatile uint64_t raw_sensor_data = 0;

union SensorData
{
    uint64_t raw_sensor_data;
    struct
    {
        uint16_t humidity_data;
        uint16_t temperature_data;
        uint16_t checksum;
    }
};


void init(void);
uint64_t sensor_rx(void);

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
    PCMSK0 |= (1 << PCINT0);    // Enable the interrupt for pin B1.

    DDRB |= (1 << DDB7); // Test led
}

// struct SensorData sensor_rx(void)
uint64_t sensor_rx(void)
{
    raw_sensor_data = 0;
    number_of_received_bits = 0;

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
    // Clear the interrupt flags:
    TIFR1 |= (1 << ICF1);
    PCIFR |= (1 << PCIF0);
    PCICR |= (1 << PCIE0);  // Enable the rising edge interrupt.
    TIMSK1 |= (1 << ICIE1); // Enable the timer capture interrupt.

    while (number_of_received_bits < 40); // Wait here for the interrupt service routine to acquire data.
    return raw_sensor_data;
}

// Interrupt Service routine for the start of a data bit
ISR (PCINT0_vect)
{
    rising_edge_counter++;
    // Interrupt flag is automatically cleared when the ISR is called.
    // Start the timer (starts at beginning of the bit)
    TCNT1 = 0;  // Clear the timer.
    TCCR1B |= (1 << CS11);  // Start the timer using CLK/8

    PCIFR |= (1 << PCIF0);
    TIMSK1 |= (1 << ICIE1); // Enable the timer capture interrupt.
    PCICR &= ~(1 << PCIE0); // Disable the PCINT0 interrupt.
}

// Interrupt service routine for stopping the timer to catch the end of the bit.
ISR (TIMER1_CAPT_vect)
{
    uint8_t captured_timer_value = ICR1;
    falling_edge_counter++;
    timestamps[number_of_received_bits] = captured_timer_value;
    raw_sensor_data <<= 1;
    if (captured_timer_value > 45)   // Is a 1
    {
        raw_sensor_data |= 1;
    }

    number_of_received_bits++;  // Incrememnt the received bit counter.

    // Continue the capturing if 40 bits have not been acquired yet:
    if (number_of_received_bits < 40)
    {
        PCIFR |= (1 << PCIF0);
        TIMSK1 &= ~(1 << ICIE1);    // Disable the input capture interrupt.
        PCICR |= (1 << PCIE0); // Enable the PCINT0 interrupt on pin B0
    }
    else
    {
        // Disable both interrupts:
        TIMSK1 &= ~(1 << ICIE1);
        PCICR &= ~(1 << PCIE0);
    }
}


