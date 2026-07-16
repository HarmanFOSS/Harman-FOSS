/**************************************************************************
 * @brief Access library for LPS25H barometric pressure sensor
 *
 * @details Encapsulate I2C access to a LPS25H sensor on a Linux machine
 * The LPS25H from ST Microelectronics is a barometric pressure sensor
 * @see http://www.st.com/web/en/catalog/sense_power/FM89/SC1316/PF255230
 *
 * @author Helmut Schmidt <https://github.com/huirad>
 * @copyright Copyright (C) 2016, Helmut Schmidt
 *
 * @license MPL-2.0 <http://spdx.org/licenses/MPL-2.0>
 *
 **************************************************************************/

#ifndef INCLUDE_LPS25H
#define INCLUDE_LPS25H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <math.h>


/** Part 0: I2C device names and device addresses
 *
 */

/** Typical I2C device names on Linux machines
  * For early Raspberry Pi machines, its typically "/dev/i2c-0"
  * For most later (starting already Q3/2012) Raspberry Pi machines, its typically "/dev/i2c-1"
  */
#define LPS25H_I2C_DEV_0 "/dev/i2c-0"
#define LPS25H_I2C_DEV_1 "/dev/i2c-1"
#define LPS25H_I2C_DEV_2 "/dev/i2c-2"
#define LPS25H_I2C_DEV_3 "/dev/i2c-3"
#define LPS25H_I2C_DEV_DEFAULT LPS25H_I2C_DEV_1

/** Possible I2C device addresses of the LPS25H
  * 0x5C is the default address
  */
#define LPS25H_ADDR_1 0x5C
#define LPS25H_ADDR_2 0x5D

/** Output data rate settings
  * See the LPS25H data sheet section on CTRL_REG1 for details
  * http://www.st.com/web/en/resource/technical/document/datasheet/DM00066332.pdf
  */
enum ELPS25HOutputDataRate
{
    LPS25H_ODR_1HZ      = 0x10,     //ODR:  1 Hz
    LPS25H_ODR_7HZ      = 0x20,     //ODR:  7 Hz
    LPS25H_ODR_12_5HZ   = 0x30,     //ODR: 12.5 Hz
    LPS25H_ODR_25HZ     = 0x40      //ODR: 25 Hz
};

/** Moving Average settings
  * See the LPS25H data sheet section on FIFO_CTRL for details
  * http://www.st.com/web/en/resource/technical/document/datasheet/DM00066332.pdf
  */
enum ELPS25HMovingAverage
{
    LPS25H_AVG_0        = 0x00,     //No moving average
    LPS25H_AVG_2        = 0x01,     //Moving average over 2 samples
    LPS25H_AVG_4        = 0x03,     //Moving average over 4 samples
    LPS25H_AVG_8        = 0x07,     //Moving average over 8 samples
    LPS25H_AVG_16       = 0x0F,     //Moving average over 16 samples
    LPS25H_AVG_32       = 0x1F      //Moving average over 32 samples
};

/** Part 1: Functions to access the LPS25H
 *
 */

/**
 * Initialize the LPS25H access.
 * Must be called before using any of the other functions.
 * @param i2c_device the name of the i2c device on which the LPS25H is attached
 * @param i2c_addr the I2C address of the LPS25H
 * @param odr output data rate
 * @param avg number of samples for moving average
 * @return true on success.
 */
bool lps25h_init(
    const char* i2c_device = LPS25H_I2C_DEV_DEFAULT, 
    uint8_t i2c_addr=LPS25H_ADDR_1, 
    ELPS25HOutputDataRate odr=LPS25H_ODR_25HZ, 
    ELPS25HMovingAverage avg=LPS25H_AVG_0
    ); 

/**
 * Release the LPS25H access.
 * @return true on success.
 */
bool lps25h_deinit();

/**
 * Read the current acceleration, angular rate and temperature from the LPS25H
 * Any pointer may be NULL to indicate that the corresponding data is not requested
 * @param pressure returns the barometric pressure as measured by gyroscope . Measurement unit is hPa (mbar).
 * @param temperature returns the temperature in degrees celsius
 * @param timestamp returns a system timestamp in ms (milliseconds) derived from clock_gettime(CLOCK_MONOTONIC);
 * @return true on success.
 * @note: Never call this function while the callback mechanism is running!
 */
bool lps25h_read(float* pressure, float* temperature, uint64_t* timestamp);

/** Part 2: Provide data via callback mechanism
 *
 */

/**
 * Callback type for LPS25H data.
 * Use this type of callback if you want to register for LPS25H data.
 * This callback may return buffered data (num_elements >1) depending on configuration
 * If the arrays contain buffered data (num_elements >1), the elements will be ordered with rising timestamps
 * Data in different arrays with the same index belong together
 * @param pressure pointer to an array of float with size num_elements containing pressure data
 * @param temperature pointer to an array of float with size num_elements containing temperature data
 * @param timestamp pointer to an array of uint64_t with size num_elements containing timestamps
 * @param num_elements: allowed range: >=1. If num_elements >1, buffered data are provided. 
 */
typedef void (*LPS25HCallback)(const float pressure[], const float temperature[], const uint64_t timestamp[], const uint16_t num_elements);

/**
 * Register LPS25H callback.
 * This is the recommended method for continuously accessing the LPS25H data
 * The callback will be invoked when new LPS25H data is available and the reader thread is running.
 * To start the reader thread, call lps25h_start_reader_thread().
 * @param callback The callback which should be registered.
 * @return True if callback has been registered successfully.
 */
bool lps25h_register_callback(LPS25HCallback callback);

/**
 * Deregister LPS25H callback.
 * After calling this method no new LPS25H data will be delivered to the client.
 * @param callback The callback which should be deregistered.
 * @return True if callback has been deregistered successfully.
 */
bool lps25h_deregister_callback(LPS25HCallback callback);

/**
 * Start the LPS25H reader thread.
 * This thread will call the callback function registered by lps25h_register_callback()
 * The thread may be started before of after registering the callback function
 * @param sample_interval Interval in ms (milliseconds) at which LPS25H data shall be read
 * @param num_samples Number of samples to read for one call of the callback function
 * @param average If true, the only the average (mean value) of the num_samples will be returned
 * @return True on success.
 * @note Be sure to select a meaningful combination of sample_interval and output data rate
 */
bool lps25h_start_reader_thread(uint64_t sample_interval, uint16_t num_samples, bool average);

/**
 * Stop the LPS25H reader thread.
 * @return True on success.
 */
bool lps25h_stop_reader_thread();

/** Part 3: Utility functions and conversion factors
 *
 */

//TODO add function for pressure to altitude conversion

 
/**
 * Get system timestamp
 * @return returns a system timestamp in ms (milliseconds) derived from clock_gettime(CLOCK_MONOTONIC);
 */
uint64_t lps25h_get_timestamp();


#ifdef __cplusplus
}
#endif

#endif //INCLUDE_LPS25H
