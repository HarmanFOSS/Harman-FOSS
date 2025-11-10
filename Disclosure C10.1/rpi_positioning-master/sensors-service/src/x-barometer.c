 /**************************************************************************
 * @brief Extension of positionin PoC for barometer pressure sensor
 *
 * @author Helmut Schmidt <https://github.com/huirad>
 * @copyright Copyright (C) 2016, Helmut Schmidt
 *
 * @license MPL-2.0 <http://spdx.org/licenses/MPL-2.0>
 *
 **************************************************************************/

#include "globals.h"
#include "x-barometer.h"
#include "sns-meta-data.h"

static pthread_mutex_t mutexCb  = PTHREAD_MUTEX_INITIALIZER;   //protects the callbacks
static pthread_mutex_t mutexData = PTHREAD_MUTEX_INITIALIZER;  //protects the data

static volatile BarometerCallback cbBarometer = 0;
static TBarometerData gBarometerData = {0};

static TSensorStatus gStatus = {0};
static volatile SensorStatusCallback cbStatus = 0;


bool iBarometerInit()
{
    pthread_mutex_lock(&mutexCb);
    cbBarometer = 0;
    pthread_mutex_unlock(&mutexCb);

    pthread_mutex_lock(&mutexData);
    gBarometerData.validityBits = 0;
    pthread_mutex_unlock(&mutexData);

    return true;
}

bool iBarometerDestroy()
{
    pthread_mutex_lock(&mutexCb);
    cbBarometer = 0;
    pthread_mutex_unlock(&mutexCb);

    return true;
}

bool snsBarometerGetBarometerData(TBarometerData* barometerData)
{
    bool retval = false;
    if(barometerData)
    {
        pthread_mutex_lock(&mutexData);
        *barometerData = gBarometerData;
        pthread_mutex_unlock(&mutexData);
        retval = true;
    }
    return retval;
}

bool snsBarometerRegisterCallback(BarometerCallback callback)
{
    bool retval = false;

    pthread_mutex_lock(&mutexCb);
    //only if valid callback and not already registered
    if(callback && !cbBarometer)
    {
        cbBarometer = callback;
        retval = true;
    }
    pthread_mutex_unlock(&mutexCb);

    return retval;
}

bool snsBarometerDeregisterCallback(BarometerCallback callback)
{
    bool retval = false;

    pthread_mutex_lock(&mutexCb);
    if((cbBarometer == callback) && callback)
    {
        cbBarometer = 0;
        retval = true;
    }
    pthread_mutex_unlock(&mutexCb);

    return retval;
}

bool snsBarometerGetMetaData(TSensorMetaData *data)
{
    bool retval = false;    
    
    // if(data) 
    // {
        // pthread_mutex_lock(&mutexData);
        // *data = gSensorsMetaData[1];
        // pthread_mutex_unlock(&mutexData);
        // retval = true;
    // }

    return retval;
}

void updateBarometerData(const TBarometerData barometerData[], uint16_t numElements)
{
    if (barometerData != NULL && numElements > 0)
    {
        pthread_mutex_lock(&mutexData);
        gBarometerData = barometerData[numElements-1];
        pthread_mutex_unlock(&mutexData);
        pthread_mutex_lock(&mutexCb);
        if (cbBarometer)
        {
            cbBarometer(barometerData, numElements);
        }
        pthread_mutex_unlock(&mutexCb);
    }
}

bool snsBarometerGetStatus(TSensorStatus* status)
{
    bool retval = false;
    if(status)
    {
        pthread_mutex_lock(&mutexData);
        *status = gStatus;
        pthread_mutex_unlock(&mutexData);
        retval = true;
    }
    return retval;
}

bool snsBarometerRegisterStatusCallback(SensorStatusCallback callback)
{
    bool retval = false;

    pthread_mutex_lock(&mutexCb);
    //only if valid callback and not already registered
    if(callback && !cbStatus)
    {
        cbStatus = callback;
        retval = true;
    }
    pthread_mutex_unlock(&mutexCb);

    return retval;
}

bool snsBarometerDeregisterStatusCallback(SensorStatusCallback callback)
{
    bool retval = false;

    pthread_mutex_lock(&mutexCb);
    if((cbStatus == callback) && callback)
    {
        cbStatus = 0;
        retval = true;
    }
    pthread_mutex_unlock(&mutexCb);

    return retval;
}

void updateBarometerStatus(const TSensorStatus* status)
{
    if (status)
    {
        pthread_mutex_lock(&mutexData);
        gStatus = *status;
        pthread_mutex_unlock(&mutexData);
        pthread_mutex_lock(&mutexCb);
        if (cbStatus)
        {
            cbStatus(status);
        }
        pthread_mutex_unlock(&mutexCb);
    }
}