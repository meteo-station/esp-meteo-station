#pragma once

#include "config.h"
#include <GyverBME280.h>
#include <GyverHTU21D.h>

#include "logger.h"
#include "../lib/SCD41/scd41.h"
#include "../lib/BME688/bme688.h"
#include "model/meteoSensorData.h"

class MeteoSensors {
public:
    void begin() {
        Wire.begin();
        _htu.begin();
        _htu_is_ready = false;
        _bme280.begin(I2C_ADDR_BME280);
        _bme688.begin(BME688_SAMPLE_RATE);
        _scd41.begin(SCD41_LOW_POWER);
    }

    void tick() {
        // Асинхронное чтение HTU21D
        _htu_is_ready = _htu.readTick(MIN_INTERVAL);
        _bme688.tick();
        _scd41.tick();
    }

    bool readMeteoData(uint32_t now, MeteoSensorData &out) {
        if (now - lastUpdate < MIN_INTERVAL) {
            return false;
        }
        lastUpdate = now;

        out.bme280.temperature = _bme280.readTemperature();
        out.bme280.pressure = _bme280.readPressure() / 133.322f;
        if (out.bme280.temperature == 0 || out.bme280.pressure == 0) {
            out.bme280.is_valid = false;
        } else {
            out.bme280.is_valid = true;
        }

        if (_htu_is_ready) {
            out.htu21d.temperature = _htu.getTemperature();
            out.htu21d.humidity = _htu.getHumidity();
            out.htu21d.is_valid = true;
        } else {
            out.htu21d.is_valid = false;
            log(LOG_WARNING, "HTU21D is not ready");
        }

        // BME688 обновляем только если в нем реально что-то появилось
        BME688Data bme688Fresh{};
        if (_bme688.get_data(bme688Fresh)) {
            out.bme688 = bme688Fresh;
        } else {
            out.bme688.is_valid = false;
        }

        // SCD41 обновляем только если в нем реально что-то появилось
        SCD41Data scd41Fresh{};
        if (_scd41.get_data(scd41Fresh)) {
            out.scd41 = scd41Fresh;
        } else {
            out.scd41.is_valid = false;
        }
        return true;
    }

private:
    BME688 _bme688;
    GyverBME280 _bme280;
    GyverHTU21D _htu;
    SCD41 _scd41;
    bool _htu_is_ready;
    uint32_t lastUpdate = 0;
    static constexpr uint32_t MIN_INTERVAL = 1000;
};
