 /**************************************************************************
 * @brief Extension of GENIVI API for barometer pressure sensor
 *
 * @author Helmut Schmidt <https://github.com/huirad>
 * @copyright Copyright (C) 2016, Helmut Schmidt
 *
 * @license MPL-2.0 <http://spdx.org/licenses/MPL-2.0>
 *
 **************************************************************************/
 
#ifndef INCLUDED_X_Barometer
#define INCLUDED_X_Barometer

#include "sns-meta-data.h"
#include "sns-status.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TBarometerData::validityBits provides information about the currently valid signals of the barometer data.
 * It is a or'ed bitmask of the EBarometerValidityBits values.
 */
typedef enum {
    BAROMETER_PRESSURE_VALID       = 0x00000001,   /**< Validity bit for field TBarometerData::pressure. */
    BAROMETER_TEMPERATURE_VALID    = 0x00000002    /**< Validity bit for field TBarometerData::pressure. */
} EBarometerValidityBits;

/**
 * Thee barometer sensor service provides the barometer pressure.
 */
typedef struct {
    uint64_t timestamp;     /**< Timestamp of the acquisition of the barometer signal [ms].
                                 All sensor/GNSS timestamps must be based on the same time source. */
    float pressure;         /**< Barometer pressure in hPa [mbar].*/
    float temperature;      /**< Temperature reading of the barometer pressure sensor.
                                 If available it can be used for temperature compensation. 
                                 The measurement unit is unspecified. 
                                 Degrees celsius are preferred but any value linearly dependent on the temperature is fine.*/
    uint32_t validityBits;  /**< Bit mask indicating the validity of each corresponding value.
                                 [bitwise or'ed @ref EBarometerValidityBits values].
                                 Must be checked before usage. */
} TBarometerData;

/**
 * Callback type for barometer sensor service.
 * Use this type of callback if you want to register for barometer data.
 * This callback may return buffered data (numElements >1) for different reasons
 *   for (large) portions of data buffered at startup
 *   for data buffered during regular operation e.g. for performance optimization (reduction of callback invocation frequency)
 * If the array contains (numElements >1), the elements will be ordered with rising timestamps
 * @param barometerData pointer to an array of TBarometerData with size numElements
 * @param numElements: allowed range: >=1. If numElements >1, buffered data are provided. 
 */
typedef void (*BarometerCallback)(const TBarometerData barometerData[], uint16_t numElements);

/**
 * Initialization of the barometer sensor service.
 * Must be called before using the barometer sensor service to set up the service.
 * @return True if initialization has been successfull.
 */
bool snsBarometerInit();

/**
 * Destroy the barometer sensor service.
 * Must be called after using the barometer sensor service to shut down the service.
 * @return True if shutdown has been successfull.
 */
bool snsBarometerDestroy();

/**
 * Provide meta information about sensor service.
 * The meta data of a sensor service provides information about it's name, version, type, subtype, sampling frequency etc.
 * @param data Meta data content about the sensor service.
 * @return True if meta data is available.
 */
bool snsBarometerGetMetaData(TSensorMetaData *data);

/**
 * Method to get the barometer data at a specific point in time.
 * @param barometer After calling the method the currently available barometer data is written into barometer.
 * @return Is true if data can be provided and false otherwise, e.g. missing initialization
 */
bool snsBarometerGetBarometerData(TBarometerData* barometerData);

/**
 * Register barometer callback.
 * The callback will be invoked when new barometer data is available.
 * @param callback The callback which should be registered.
 * @return True if callback has been registered successfully.
 */
bool snsBarometerRegisterCallback(BarometerCallback callback);

/**
 * Deregister barometer callback.
 * After calling this method no new barometer data will be delivered to the client.
 * @param callback The callback which should be deregistered.
 * @return True if callback has been deregistered successfully.
 */
bool snsBarometerDeregisterCallback(BarometerCallback callback);

/**
 * Method to get the barometer sensor status at a specific point in time.
 * @param status After calling the method the current barometer sensor status is written into status
 * @return Is true if data can be provided and false otherwise, e.g. missing initialization
 */
bool snsBarometerGetStatus(TSensorStatus* status);

/**
 * Register barometersensor status callback.
 * This is the recommended method for continuously monitoring the barometer sensor status.
 * The callback will be invoked when new barometer sensor status data is available.
 * @param callback The callback which should be registered.
 * @return True if callback has been registered successfully.
 */
bool snsBarometerRegisterStatusCallback(SensorStatusCallback callback);

/**
 * Deregister barometer sensor status callback.
 * After calling this method no new barometer sensor status updates will be delivered to the client.
 * @param callback The callback which should be deregistered.
 * @return True if callback has been deregistered successfully.
 */
bool snsBarometerDeregisterStatusCallback(SensorStatusCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_X_Barometer */
