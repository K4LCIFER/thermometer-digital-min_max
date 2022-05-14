


/* System Defines */
#define F_CPU 8000000UL
#define INND_TS56

/* AVR Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
/* GCC Includes */
#include <string.h>
/* External Includes */
#include "seven_segment.h"



/* Pogram constant defines */
#define BIT_PERIOD_THRESHOLD 100

// #define HUMIDITY_DATA_BITMASK   0xFFFF000000
// #define HUMIDITY_DATA_OFFSET    24
#define TEMP_DATA_BITMASK    0x0000FFFF00
#define TEMP_DATA_OFFSET     8

#define DISPLAY_0 0
#define DISPLAY_1 1
#define DISPLAY_2 2
#define DISPLAY_3 3

#define NEG_TEMP_BITMASK 0x8000



typedef struct
{
    // All three of the following have to be volatile to remove a warning. I
    // think that it is because PINx, DDRx, and PORTx are all declared as
    // volatile, so attaching a volatile variable to something non volatile
    // casts it to a non volatile variable.
    volatile uint8_t *input_register;
    volatile uint8_t *direction_register;
    volatile uint8_t *output_register;
    uint8_t pin_bitmask;    // Represents the pin on the port.
} pin_t;

// AM2302 Sensor data line pin (B0):
static const pin_t sensor_data_pin = {
    .input_register = &PINB,
    .direction_register = &DDRB,
    .output_register = &PORTB,
    .pin_bitmask = 1 << 0,
};

// Display segment pins (D[0-7]):
static const pin_t segment_pins = 
{
    .direction_register = &DDRD,
    .output_register = &PORTD,
    .pin_bitmask = 0xFF,
};

// Display enable (through the transistors) pins (C[0-3]):
static const pin_t display_enable_pins[] = {
    {
        .direction_register = &DDRC,
        .output_register = &PORTC,
        .pin_bitmask = 1 << 0,
    },
    {
        .direction_register = &DDRC,
        .output_register = &PORTC,
        .pin_bitmask = 1 << 1,
    },
    {
        .direction_register = &DDRC,
        .output_register = &PORTC,
        .pin_bitmask = 1 << 2,
    },
    {
        .direction_register = &DDRC,
        .output_register = &PORTC,
        .pin_bitmask = 1 << 3,
    },
};

// Pushbutton signal pins:
// NOTE: Maybe use the internal pullups of the microcontroller.
// NOTE: Should probably pull the pushbutton to ground instead of VCC.
// Display saved minimum temperature button pin (B1):
// Reset saved temperatures button pin (B4):
static const pin_t reset_saved_temps_btn_pin = {
    .input_register = &PINB,
    .direction_register = &DDRB,
    .output_register = &PORTB,
    .pin_bitmask = 1 << 1,
};
static const pin_t min_temp_btn_pin = {
    .input_register = &PINB,
    .direction_register = &DDRB,
    .output_register = &PORTB,
    .pin_bitmask = 1 << 2,
};
// Display saved maximum temperature button pin (B2):
static const pin_t max_temp_btn_pin = {
    .input_register = &PINB,
    .direction_register = &DDRB,
    .output_register = &PORTB,
    .pin_bitmask = 1 << 3,
};
// Display current temperature button pin (B3):
static const pin_t current_temp_btn_pin = {
    .input_register = &PINB,
    .direction_register = &DDRB,
    .output_register = &PORTB,
    .pin_bitmask = 1 << 4,
};

union SensorData
{
    uint64_t raw_sensor_data;
    struct
    {
        // uint16_t humidity_data;
        uint16_t temp_data;
    };
} sensor_data;

volatile uint8_t number_of_captures = 0;
volatile uint16_t ovf_counter = 0;

volatile float max_temp;
volatile float min_temp;
volatile float current_temp;
uint8_t max_temp_digits[4];
uint8_t min_temp_digits[4];
uint8_t current_temp_digits[4]; // Might not need this.


void init(void);
uint8_t get_temp(void);
uint8_t sensor_rx(void);
// NOTE: Maybe instead of making this array fixed in size, I should declare it
// as a pointer, and then just get the size of the passed array when I use it
// in the function.
void display_digits(volatile uint8_t digit_array[4]);




void main(void)
{
    init();

    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);  // Start Timer2 with CLK/1024.
    TIFR2 |= (1 << TOV2);
    TIMSK2 |= (1 << TOIE2); // Enable overflow interrupt for timer 2

    while(1)
    {
        sleep_mode();
    }
}

void init(void)
{

    // Reduce power consumption:
    PRR |= (1 << PRADC);    // Disables the ADC.
    PRR |= (1 << PRUSART0); // Disables the USART0
    PRR |= (1 << PRSPI);    // Disables the SPI
    PRR |= (1 << PRTIM0);   // Disables the Timer0
    PRR |= (1 << PRTWI);    // Disables the TWI
    SMCR |= ((1 << SM1) | (1 << SM0));  // Power save mode
    SMCR |= (1 << SE);  // Enable sleep.
    sleep_bod_disable();    // Disable BOD.

    // Set the seven segment pins as outputs:
    *segment_pins.direction_register = segment_pins.pin_bitmask;
    // Set the display_enable_pins as outputs:
    // Display 0
    *display_enable_pins[DISPLAY_0].direction_register 
        |= display_enable_pins[DISPLAY_0].pin_bitmask;
    // Display 1
    *display_enable_pins[DISPLAY_1].direction_register 
        |= display_enable_pins[DISPLAY_1].pin_bitmask;
    // Display 2
    *display_enable_pins[DISPLAY_2].direction_register 
        |= display_enable_pins[DISPLAY_2].pin_bitmask;
    // Display 3
    *display_enable_pins[DISPLAY_3].direction_register 
        |= display_enable_pins[DISPLAY_3].pin_bitmask;

    // Configure the buttons:
    // Set the button pins as inputs, and enable their pull-ups:
    // Reset saved temperatures button.
    *reset_saved_temps_btn_pin.direction_register 
        &= ~reset_saved_temps_btn_pin.pin_bitmask;
    *reset_saved_temps_btn_pin.output_register 
        |= reset_saved_temps_btn_pin.pin_bitmask;
    // Display minimum temperature button.
    *min_temp_btn_pin.direction_register &= ~min_temp_btn_pin.pin_bitmask;
    *min_temp_btn_pin.output_register |= min_temp_btn_pin.pin_bitmask;
    // Display maximum temperature button.
    *max_temp_btn_pin.direction_register &= ~max_temp_btn_pin.pin_bitmask;
    *max_temp_btn_pin.output_register |= max_temp_btn_pin.pin_bitmask;
    // Display current temperature button.
    *current_temp_btn_pin.direction_register 
        &= ~current_temp_btn_pin.pin_bitmask;
    *current_temp_btn_pin.output_register |= current_temp_btn_pin.pin_bitmask;

    _delay_ms(2000);    // Sensor bootup delay.

    sei();  // Enable global interrupts.

    // Initialize values:
    get_temp();
    memcpy(max_temp_digits, current_temp_digits, sizeof(current_temp_digits));
    memcpy(min_temp_digits, current_temp_digits, sizeof(current_temp_digits));
    max_temp = current_temp;
    min_temp = current_temp;

    // Set up the interrupts:
    // Enable interrupts for the 4 pushbuttons on their respective pins.
    PCMSK0 |= ((1 << PCINT1) | (1 << PCINT2) | (1 << PCINT3) | (1 << PCINT4));
    PCIFR |= (1 << PCIF0); // Ensure that the button interrupt is cleared.
    PCICR |= (1 << PCIE0); // Enable pushbutton interrupts.

    DDRB |= (1 << DDB7); // Test LED.
}

uint8_t sensor_rx(void)
{
    number_of_captures = 0;

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
    while (number_of_captures < 41);
    TIMSK1 &= ~(1 << ICIE1);    // Disable the timer capture interrupt.
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (CS10));    // Stop the timer.

    // Verify the data with the checksum:
    // TODO: Need to rewrite this to handle a sum overflow. The checksum is only
    // the last 8 bits of the sum.
    uint8_t *raw_sensor_data_bytes = (uint8_t *)&sensor_data.raw_sensor_data;
    uint8_t checksum = raw_sensor_data_bytes[0];
    uint8_t sum = 0;
    for (uint8_t i = 1; i < 8; i++)
    {
        sum = sum + raw_sensor_data_bytes[i];
    }
    if ((uint8_t)sum != checksum)    // Bad data.
    {
        return 1;   // Return error flag.
    }
    // Parse the received sensor data:
    // NOTE: The temporary variables prevent overwriting of the union's data
    // until it is actually desired. Perhaps it would be better to just not
    // worry about wasting excess memory, and just use a single struct? Doing
    // that would be a simpler route.
    sensor_data.temp_data = (sensor_data.raw_sensor_data & TEMP_DATA_BITMASK) 
        >> TEMP_DATA_OFFSET;

    return 0;
}

uint8_t get_temp(void)
{
    sensor_rx();    // Retreive data from the sensor.
    current_temp = 1;   // Allows for easy multiplication.
    // Store the floating point absolute value of the temperature.
    // Check if there if the negative temperature flag is set:
    if (sensor_data.temp_data & NEG_TEMP_BITMASK)
    {
        current_temp *= -1; // Floating point value of the temperature.
        current_temp_digits[3] = '-';   // Digit array.
        // Remove the negative temperature flag from the temperature data to
        // just leave the digits remaining.
        sensor_data.temp_data &= ~(NEG_TEMP_BITMASK);
    }
    // Save the floating point value of the temperature.
    current_temp *= sensor_data.temp_data / 10;
    // Extract the digits from the temperature data:
    for (uint8_t i = 0;
            i < (sizeof(current_temp_digits) / sizeof(current_temp_digits[1])) 
                - 1; 
            i++)
    {
        // Extract the digit.
        current_temp_digits[i] = sensor_data.temp_data % 10;
        // Shift the digits to the right by 1.
        sensor_data.temp_data /= 10;
    }
    return 0;
}

void display_digits(volatile uint8_t digit_array[4])
{
    uint8_t output_digit;
    // - Start a 16-bit timer at CLK/1024
    // - When the timer reaches 39062(.5) cycles (5s), stop the display (clear
    // the digits).
    // - TODO: Add PWM to save even more power? Or just loop through the digits
    // on a timer to function as a PWM. The duty cylce would only ever be able
    // to be max 25%, though. Higher brightness would require higher frequency.
    TCNT1 = 0;  // Clear the timer.
    // Reset the clock prescaler/stop the timer.
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
    TCCR1B |= (1 << CS12) | (1 << CS10);    // Star the timer with CLK/1024.

    while (1)
    {
        if (TCNT1 > 39062) // Display the digits for 5 seconds.
        {
            *segment_pins.output_register = SS_CLEAR;
            return;
        }
        for (uint8_t i = 0; i < 3; i++)
        {
            switch (digit_array[i])
            {
                case '-':
                    output_digit = SS_NEG;
                    break;
                case 0:
                    output_digit = SS_0;
                    break;
                case 1:
                    output_digit = SS_1;
                    break;
                case 2:
                    output_digit = SS_2;
                    break;
                case 3:
                    output_digit = SS_3;
                    break;
                case 4:
                    output_digit = SS_4;
                    break;
                case 5:
                    output_digit = SS_5;
                    break;
                case 6:
                    output_digit = SS_6;
                    break;
                case 7:
                    output_digit = SS_7;
                    break;
                case 8:
                    output_digit = SS_8;
                    break;
                case 9:
                    output_digit = SS_9;
                    break;
            }
            // If the digit is the second digit on the display, add a decimal 
            // to the number.
            if (i == 1)
            {
                output_digit |= SS_DEC;
            }
            // Ensure all displays are initially disabled.
            for (
                    uint8_t display_enable_pin = 0;
                    display_enable_pin < (
                        sizeof(display_enable_pins)
                            / sizeof(display_enable_pins[0])
                        );
                    display_enable_pin++
                )
            {
                *display_enable_pins[display_enable_pin].output_register
                    &= ~(display_enable_pins[display_enable_pin].pin_bitmask);
            }
            // Enable the desired display.
            *display_enable_pins[i].output_register 
                |= display_enable_pins[i].pin_bitmask;
            // Output the digit to the display.
            *segment_pins.output_register = output_digit;
        }
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

        previous_timer_value = current_timer_value;

        // Store the received bit.
        sensor_data.raw_sensor_data = (sensor_data.raw_sensor_data << 1) 
            | (bit_period > BIT_PERIOD_THRESHOLD);

        number_of_captures++;
        // If all the bits are received, then stop receiving.
    }
}

// TODO: Add the a timer overflow catcher in case of a failed transmission.

// ISR for buttons:
ISR (PCINT0_vect)
{
    TIMSK2 &= ~(1 << TOIE2); // Disable overflow interrupt for timer 2
    // NOTE: Should probably disable the button interrupt?
    _delay_ms(1);    // Debounce? Could try shortening this.
    sei();
    if (!(*reset_saved_temps_btn_pin.input_register 
                & reset_saved_temps_btn_pin.pin_bitmask))
    {
        get_temp();
        memcpy(
                max_temp_digits,
                current_temp_digits,
                sizeof(current_temp_digits)
        );
        memcpy(
                min_temp_digits,
                current_temp_digits,
                sizeof(current_temp_digits)
        );
        max_temp = current_temp;
        min_temp = current_temp;
    }
    else if (!(*min_temp_btn_pin.input_register & min_temp_btn_pin.pin_bitmask))
    {
        display_digits(min_temp_digits);
    }
    else if (!(*max_temp_btn_pin.input_register & max_temp_btn_pin.pin_bitmask))
    {
        display_digits(max_temp_digits);
    }
    else if (!(*current_temp_btn_pin.input_register 
                & current_temp_btn_pin.pin_bitmask))
    {
        get_temp(); 
        display_digits(current_temp_digits);
    }
    TCNT2= 0;
    TIFR2 |= (1 << TOV2);
    TIMSK2 |= (1 << TOIE2); // Enable overflow interrupt for timer 2
}

ISR (TIMER2_OVF_vect)
{
    if (ovf_counter < 1000)
    {
        ovf_counter++;
    }
    else
    {
        sei();
        ovf_counter = 0;
        get_temp();
        if (current_temp < min_temp)
        {
            min_temp = current_temp;
            memcpy(
                    min_temp_digits,
                    current_temp_digits,
                    sizeof(current_temp_digits)
            );
        }
        else if (current_temp > max_temp)
        {
            max_temp = current_temp;
            memcpy(
                    max_temp_digits,
                    current_temp_digits,
                    sizeof(current_temp_digits)
            );
        }
    }
}

