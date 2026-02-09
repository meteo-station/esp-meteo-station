#pragma once

#include "config.h"
#include <GyverBME280.h>
#include <GyverHTU21D.h>
#include "../lib/BME688/bme688.h"
#include "model/meteoSensorData.h"

class MeteoSensors {
public:
    void begin() {
        _htu.begin();
        _bme280.begin(I2C_ADDR_BME280);
        _bme688.begin(BME688_SAMPLE_RATE);
    }

    void tick() {
        // Асинхронное чтение HTU21D
        _htu.readTick(MIN_INTERVAL);
        _bme688.tick();
    }

    bool readMeteoData(uint32_t now, MeteoSensorData &out) {
        if (now - lastUpdate < MIN_INTERVAL) {
            return false;
        }
        lastUpdate = now;

        out.bme280.temperature = _bme280.readTemperature();
        out.htu21d.temperature = _htu.getTemperature();
        out.htu21d.humidity = _htu.getHumidity();
        out.bme280.pressure = _bme280.readPressure() / 133.322f;

        // BME688 обновляем только если в нем реально что-то появилось
        BME688Data bme688Fresh{};
        if (_bme688.get_data(bme688Fresh)) {
            out.bme688 = bme688Fresh;
        } else {
            out.bme688 = {};
        }
        return true;
    }

private:
    BME688 _bme688;
    GyverBME280 _bme280;
    GyverHTU21D _htu;
    uint32_t lastUpdate = 0;
    static constexpr uint32_t MIN_INTERVAL = 1000;
};
