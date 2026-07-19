#pragma once

#include <Wire.h>

#include "logger.h"
#include "SensirionI2cScd4x.h"
#include "model/SCD41Data.h"

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

class SCD41 {
public:
    SCD41() = default;

    // lowPower = true  -> измерение раз в 30 с
    // lowPower = false -> измерение раз в 5 с
    void begin(bool lowPower = false) {
        // Инициализируем датчик
        _scd41.begin(Wire, SCD41_I2C_ADDR_62);

        // Ensure sensor is in clean state
        _error = _scd41.wakeUp();
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to execute wakeUp(): %s", _errorMessage);
        }
        delay(30); // время пробуждения датчика

        _error = _scd41.stopPeriodicMeasurement();
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to execute stopPeriodicMeasurement(): %s", _errorMessage);
        }
        delay(500); // датчик не принимает команды 500 мс после остановки

        _error = _scd41.reinit();
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to execute reinit(): %s", _errorMessage);
        }
        delay(30);

        uint64_t serialNumber = 0;

        // Read out information about the _scd41
        _error = _scd41.getSerialNumber(serialNumber);
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to execute getSerialNumber(): %s", _errorMessage);
            return;
        }
        // %llu не поддерживается vsnprintf на ESP8266, печатаем двумя половинами
        log(LOG_INFO, "serial number: %08lX%08lX",
            (uint32_t) (serialNumber >> 32), (uint32_t) serialNumber);

        // Запускаем периодические измерения, иначе данных не будет никогда
        _error = lowPower
                     ? _scd41.startLowPowerPeriodicMeasurement()
                     : _scd41.startPeriodicMeasurement();
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to start periodic measurement: %s", _errorMessage);
        }
    }

    void tick() {
        bool isDataReady = false;
        _error = _scd41.getDataReadyStatus(isDataReady);
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to execute getDataReadyStatus(): %s", _errorMessage);
            return;
        }
        if (!isDataReady) return;

        _error = _scd41.readMeasurement(
            _cachedData.co2, _cachedData.temperature, _cachedData.humidity);
        if (_error != NO_ERROR) {
            errorToString(_error, _errorMessage, sizeof _errorMessage);
            log(LOG_ERROR, "Error trying to execute readMeasurement(): %s", _errorMessage);
            return;
        }

        _cachedData.is_valid = true;
        _hasNewData = true;
    }

    // Метод получения данных
    bool get_data(SCD41Data &out) {
        out = _cachedData;
        bool hasNew = _hasNewData;
        _hasNewData = false;
        return hasNew;
    }

private:
    SensirionI2cScd4x _scd41;

    int16_t _error = NO_ERROR;
    char _errorMessage[64] = {};

    SCD41Data _cachedData = {};
    bool _hasNewData = false;
};
