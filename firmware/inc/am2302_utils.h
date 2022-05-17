
#ifndef __AM2302_UTILS_H__
#define __AM2302_UTILS_H__

#include <stdint.h>
#include "atmega328p_utils.h"   // Maybe change to gpio_utils to be more general?

#define BIT_PERIOD_THRESHOLD 100
// Position of the negative temperature flag within the raw sensor data.
#define NEG_TEMP_BITMASK 0x800000
// Position of the temperature data within the raw sensor data.
#define TEMP_DATA_BITMASK    0x0000FFFF00
// The offset from the LSB of the temperature data within the raw sensor data.
#define TEMP_DATA_OFFSET     8
#define HUMIDITY_DATA_BITMASK
#define HUMIDITY_DATA_OFFSET

extern volatile uint8_t number_of_captures;
extern volatile uint64_t raw_sensor_data;

/**
 * @brief Initializes the sensor before use.
 * 
 * @details The function creates a delay of 2s to allow the sensor enough time
 * to boot up.
 * 
 * @param[in] Nothing.
 * @return Nothing.
 */
extern void sensor_init(void);

/**
 * @brief Receives data from the AM2302 sensor.
 * 
 * @param pin The pin to be used as the data pin for the sensor.
 * @returns The transmission success (0) or failure (1) flag.
 * 
 * @details The transaction with then sensor goes through the following steps:
 * 1. Bring the data line low for a minimum of 1ms
 * 2. Wait (20us-40us) for the sensors response where it brings the data line
 * low for 80us.
 * 3. The sensor then brings the line high for 80us.
 * 4. The data transmission begins with the most significant bit first:
 *   - A 1 corresponds to a pulse-width of 70us.
 *   - A 0 correspons to a pulse-width of 26us.
 *   - The low time separating the bits being 50us.
 * 5. The sensor will send 40 bits:
 *   - The bytes 0xFFFF000000 are the humidity data.
 *   - The bytes 0x0000FFFF00 are the temperature data.
 *   - The byte 0x00000000FF is the checksum.
 * 6. The transaction ends by the sensor pulling the line high.
 */
extern uint8_t sensor_rx(pin_t pin);

#endif