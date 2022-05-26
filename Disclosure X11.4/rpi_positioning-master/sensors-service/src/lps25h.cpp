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


/** ===================================================================
 * 1.) INCLUDES
 */

 //provided interface
#include "lps25h.h"

//linux i2c access
#include "i2ccomm.h"

//standard c library functions
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>



/** ===================================================================
 * 2.) LPS25H magic numbers
 * Source: http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00066332.pdf
 */

/** LPS25H register addresses
 *  See the application note TN1227 on how to interpret the sensor readings
 *  http://www.st.com/st-web-ui/static/active/en/resource/technical/document/technical_note/DM00242306.pdf
 *  See the application note AN4450 on how to read the FIFO
 *  http://www.st.com/st-web-ui/static/active/jp/resource/technical/document/application_note/DM00108837.pdf
 *  The temperature register must be read separately.
 */
 
#define LPS25H_REG_WHO_AM_I         0x0F
#define LPS25H_REG_RES_CONF         0x10
#define LPS25H_REG_CTRL_REG1        0x20
#define LPS25H_REG_CTRL_REG2        0x21
#define LPS25H_REG_STATUS_REG       0x27
#define LPS25H_REG_PRESS_OUT_XL     0x28
#define LPS25H_REG_PRESS_OUT_L      0x29
#define LPS25H_REG_PRESS_OUT_H      0x2A
#define LPS25H_REG_TEMP_OUT_L       0x2B
#define LPS25H_REG_TEMP_OUT_H       0x2C
#define LPS25H_REG_FIFO_CTRL        0x2E
 
 /** LPS25H register values
  */
#define LPS25H_WHO_AM_I             0xBD
#define LPS25H_CTRL_REG1__RESET     0x02    //PD=0, RESET_AZ=1
#define LPS25H_CTRL_REG1__INIT      0x84    //PD=1, BDU=1
#define LPS25H_CTRL_REG2__FIFO_OFF  0x00    //FIFO_EN=0
#define LPS25H_CTRL_REG2__FIFO_ON   0x40    //FIFO_EN=1, but do not set BOOT or SWRESET
#define LPS25H_RES_CONF__INIT       0x0D    //AVGT=16, AVGP=512 internal averaging
#define LPS25H_FIFO_CTRL__BYPASS    0x00    //BYPASS MODE
#define LPS25H_FIFO_CTRL__MEAN      0xC0    //FIFO_MEAN MODE
#define LPS25H_STATUS_REG__P_DA     0x02    //Pressure data available
#define LPS25H_STATUS_REG__T_DA     0x01    //Temperature data available
 

 /** LPS25H conversion factors
  * Pressure scale: 4096 LSB/hPa
  * Temperature scale: 480 LSB/°C
  * Temperature in degrees C [T(°C)] = 42.5 + (TEMP_OUT / 480)
  */
#define LPS25H_PRESS_SCALE  4096.0
#define LPS25H_TEMP_SCALE    480.0
#define LPS25H_TEMP_BIAS      42.5


/** ===================================================================
 * 3.) PRIVATE VARIABLES AND FUNCTIONS
 * Functions starting with conv_ convert the raw data to common measurement units
 */

/** LSM9DS1 reader thread control
 */
static volatile int _lps25h_reader_loop = 0;
static pthread_t _reader_thread;
static uint64_t _sample_interval;
static uint16_t _num_samples;
static bool _average;

/** Callback function and associated mutex
 */
static pthread_mutex_t _mutex_cb  = PTHREAD_MUTEX_INITIALIZER;
static volatile LPS25HCallback _cb = 0;

static i2ccomm _i2ccomm;

static bool lps25h_config(ELPS25HMovingAverage avg)
{
    bool result = true;
    uint8_t whoami;
    uint8_t reg2;
    uint8_t fifo_ctrl;
    if (avg==LPS25H_AVG_0)
    {   //FIFO OFF
        reg2 = LPS25H_CTRL_REG2__FIFO_OFF;
        fifo_ctrl = LPS25H_FIFO_CTRL__BYPASS;
    }
    else
    {
        reg2 = LPS25H_CTRL_REG2__FIFO_ON;
        fifo_ctrl = LPS25H_FIFO_CTRL__MEAN | avg;
    }

    //Reset the LPS25H
    result = _i2ccomm.write_uint8(LPS25H_REG_CTRL_REG1, LPS25H_CTRL_REG1__RESET);
    //default settings
    if(result)
    {
        result = _i2ccomm.write_uint8(LPS25H_REG_CTRL_REG2, reg2);
    }
    if(result)
    {
        result = _i2ccomm.write_uint8(LPS25H_REG_RES_CONF, LPS25H_RES_CONF__INIT);
    }   
    if (result)
    {
        result = _i2ccomm.write_uint8(LPS25H_REG_FIFO_CTRL, fifo_ctrl);
    }
    //wait 100ms to be on the safe side
    usleep(100000);
    //Test the WHO_AM_I register
    if (result)
    {
        result = _i2ccomm.read_uint8(LPS25H_REG_WHO_AM_I, &whoami);
        result = result && (LPS25H_WHO_AM_I == whoami);
    }
    return result;
}

static bool lps25h_setODR(ELPS25HOutputDataRate odr)
{
    bool result = true;
    result = _i2ccomm.write_uint8(LPS25H_REG_CTRL_REG1, LPS25H_CTRL_REG1__INIT | odr);
    //wait 100ms to guarantee that sensor data is available at next read attempt
    //TODO Note: 100ms may be too short for slow sample rates
    usleep(100000);
    return result;
}


static float conv_pressure(int32_t raw_pressure)
{
    return raw_pressure / LPS25H_PRESS_SCALE;
}

static float conv_temp(int16_t raw_temp)
{
    return raw_temp / LPS25H_TEMP_SCALE + LPS25H_TEMP_BIAS;
}


static uint64_t sleep_until(uint64_t wakeup)
{
    uint64_t start =  lps25h_get_timestamp();

    if (wakeup > start)
    {
        uint64_t diff = wakeup - start;
        struct timespec t;
        t.tv_sec = diff / 1000;
        t.tv_nsec = (diff - t.tv_sec*1000) * 1000000;
        while(nanosleep(&t, &t));
    }

    uint64_t stop =  lps25h_get_timestamp();
    return stop-start;
}

static bool fire_callback(const float pressure[], const float temperature[], const uint64_t timestamp[], const uint16_t num_elements, bool average)
{
    pthread_mutex_lock(&_mutex_cb);
    if (_cb)
    {
        if (average && num_elements)
        {
            float av_pressure = pressure[0];
            float av_temperature = temperature[0];
            for (uint16_t i=1; i<num_elements; i++)
            {
                av_pressure += pressure[i];
                av_temperature += temperature[i];
            }
            av_pressure /= num_elements;
            av_temperature /= num_elements;
            uint64_t last_timestamp = timestamp[num_elements-1];
            _cb(&av_pressure, &av_temperature, &last_timestamp, 1);
        }
        else
        {
            _cb(pressure, temperature, timestamp, num_elements);
        }
    }
    pthread_mutex_unlock(&_mutex_cb);
}

/**
 * Worker thread to read LPS25H data
 * @param param pointer to parameters (currently unused)
 */
static void* lps25h_reader_thread(void* param)
{
    float pressure[_num_samples];
    float temperature[_num_samples];
    uint64_t timestamp[_num_samples];

    uint16_t sample_idx = 0;

    uint64_t next =  lps25h_get_timestamp();
    uint64_t next_cb = next;

    while (_lps25h_reader_loop)
    {
        if (lps25h_read(&pressure[sample_idx], &temperature[sample_idx], &timestamp[sample_idx]))
        {
            sample_idx++;
        }
        else
        {
            //if there is no new measurement available, just skip it
            //TODO more sophisticated exception handling
        }


        //fire callback when either the requested number of samples has been acquired or the corresponding time is over
        if ((sample_idx == _num_samples) || (lps25h_get_timestamp() > next_cb))
        {
            fire_callback(pressure, temperature, timestamp, sample_idx, _average);
            sample_idx = 0;
            next_cb += _sample_interval*_num_samples;
        }
        //wait until next sampling timeslot
        next = next + _sample_interval;
        sleep_until(next);
    }
}




/** ===================================================================
 * 4.) FUNCTIONS IMPLEMENTING THE PUBLIC INTERFACE OF lsm9ds1.h
 */

bool lps25h_init(const char* i2c_device, uint8_t i2c_addr, ELPS25HOutputDataRate odr, ELPS25HMovingAverage avg)
{
    bool result = false;
    result = _i2ccomm.init(i2c_device, i2c_addr);
    if (result)
    {
        result = lps25h_config(avg);
    }
    if (result)
    {
        result = lps25h_setODR(odr); //also triggers wake up from power down
    }
    return result;
}

bool lps25h_deinit()
{
    bool result = false;
    result = _i2ccomm.deinit();
    return result;
}


bool lps25h_read(float* pressure, float* temperature, uint64_t* timestamp)
{
    bool result = true;
    bool da_pressure = false;
    bool da_temperature = false;
    uint8_t reg_status;
    uint8_t reg_pressure[3];
    uint8_t reg_temperature[2];
    int32_t raw_pressure = 0xFFFFFFFF;
    int16_t raw_temperature = 0xFFFF;

    if (timestamp != NULL)
    {
        *timestamp = lps25h_get_timestamp();
    }

    if (_i2ccomm.read_uint8(LPS25H_REG_STATUS_REG, &reg_status))
    {
        da_pressure = (reg_status & LPS25H_STATUS_REG__P_DA) == LPS25H_STATUS_REG__P_DA;
        da_temperature = (reg_status & LPS25H_STATUS_REG__T_DA) == LPS25H_STATUS_REG__T_DA;
    }
    else
    {
        result = false;
    }

    if (pressure)
    {
        //For the LPS25H the auto-address increment must be explicitly set by the MSB of the SUB
        //See section 5.2.1 of the data sheet
        //Note that this is different from other ST sensors such as the LSM9DS1
        if (da_pressure && _i2ccomm.read_block(LPS25H_REG_PRESS_OUT_XL|0x80, reg_pressure, 3))
        {
            //TODO check typecast for negative values
            raw_pressure = (((int32_t)reg_pressure[2]) << 16) | (reg_pressure[1] << 8) | reg_pressure[0];
            *pressure = conv_pressure(raw_pressure);
        }
        else
        {
            result = false;
        }
    }

    if (temperature)
    {
        //For the LPS25H the auto-address increment must be explicitly set by the MSB of the SUB
        //See section 5.2.1 of the data sheet
        //Note that this is different from other ST sensors such as the LSM9DS1    
        if (da_temperature && _i2ccomm.read_block(LPS25H_REG_TEMP_OUT_L|0x80, reg_temperature, 2))
        {
            raw_temperature = (((int16_t)reg_temperature[1]) << 8) | reg_temperature[0];
            *temperature = conv_temp(raw_temperature);
        }
        else
        {
            result = false;
        }
    }
    
    return result;
}

bool lps25h_register_callback(LPS25HCallback callback)
{
    if(_cb != 0)
    {
        return false; //if already registered
    }

    pthread_mutex_lock(&_mutex_cb);
    _cb = callback;
    pthread_mutex_unlock(&_mutex_cb);

    return true;
}

bool lps25h_deregister_callback(LPS25HCallback callback)
{
    if(_cb == callback && _cb != 0)
    {
        return false; //if already registered
    }

    pthread_mutex_lock(&_mutex_cb);
    _cb = 0;
    pthread_mutex_unlock(&_mutex_cb);

    return true;
}

bool lps25h_start_reader_thread(uint64_t sample_interval, uint16_t num_samples, bool average)
{
    if (_lps25h_reader_loop)
    {
        return false; //thread already running
    }
    if (sample_interval == 0)
    {
        return false;
    }
    if (num_samples == 0)
    {
        return false;
    }

    _sample_interval = sample_interval;
    _num_samples = num_samples;
    _average = average;

    _lps25h_reader_loop = 1;

    int res = pthread_create (&_reader_thread, NULL, lps25h_reader_thread, NULL);

    if (res != 0)
    {
        _lps25h_reader_loop = 0;
        return false;
    }

    return true;
}

bool lps25h_stop_reader_thread()
{
    _lps25h_reader_loop = 0;
    pthread_join (_reader_thread, NULL);
    return true;
}

uint64_t lps25h_get_timestamp()
{
  struct timespec time_value;
  if (clock_gettime(CLOCK_MONOTONIC, &time_value) != -1)
  {
    return (time_value.tv_sec*1000 + time_value.tv_nsec/1000000);
  }
  else
  {
    return 0xFFFFFFFFFFFFFFFF;
  }
}

