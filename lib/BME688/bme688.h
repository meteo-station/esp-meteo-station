#pragma once

#include "bsec2.h"
#include "../../src/model/BME688Data.h"

class BME688 {
public:
    BME688() = default;

    void begin(float sampleRate) {
        bsecSensor sensorList[] = {
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_CO2_EQUIVALENT,
            BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
            BSEC_OUTPUT_RAW_PRESSURE,
            BSEC_OUTPUT_STATIC_IAQ,
            BSEC_OUTPUT_GAS_PERCENTAGE,
            BSEC_OUTPUT_STABILIZATION_STATUS,
            BSEC_OUTPUT_RUN_IN_STATUS,
        };

        Wire.begin();

        if (!_bme688.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
            _checkBME688Status();
        }

        if (sampleRate == BSEC_SAMPLE_RATE_ULP) {
            _bme688.setTemperatureOffset(TEMP_OFFSET_ULP);
        } else if (sampleRate == BSEC_SAMPLE_RATE_LP) {
            _bme688.setTemperatureOffset(TEMP_OFFSET_LP);
        }

        if (!_bme688.updateSubscription(sensorList, ARRAY_LEN(sensorList), sampleRate)) {
            _checkBME688Status();
        }

        // Мы используем статический метод класса как колбэк
        _bme688.attachCallback(_bsecStaticCallback);
    }

    void tick() {
        _bme688.run();
    }

    // Метод получения данных
    bool get_data(BME688Data &out) {
        out = _cachedData;
        bool hasNew = _hasNewData;
        _hasNewData = false;
        return hasNew;
    }

private:
    Bsec2 _bme688;

    inline static BME688Data _cachedData = {};
    inline static bool _hasNewData = false;

    static void _bsecStaticCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 b) {
        if (!outputs.nOutputs) return;

        for (uint8_t i = 0; i < outputs.nOutputs; i++) {
            const bsecData output = outputs.output[i];
            switch (output.sensor_id) {
                case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                    _cachedData.temperature = output.signal; break;
                case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                    _cachedData.humidity = output.signal; break;
                case BSEC_OUTPUT_RAW_PRESSURE:
                    _cachedData.pressure = output.signal / 1.33322f; break;
                case BSEC_OUTPUT_IAQ:
                    _cachedData.iaq_accuracy = output.accuracy;
                    _cachedData.iaq = output.signal; break;
                case BSEC_OUTPUT_CO2_EQUIVALENT:
                    _cachedData.eco2_accuracy = output.accuracy;
                    _cachedData.eco2 = output.signal; break;
                case BSEC_OUTPUT_GAS_PERCENTAGE:
                    _cachedData.gas_percentage_accuracy = output.accuracy;
                    _cachedData.gas_percentage = output.signal; break;
                case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                    _cachedData.evoc_accuracy = output.accuracy;
                    _cachedData.evoc = output.signal; break;
                case BSEC_OUTPUT_STATIC_IAQ:
                    _cachedData.iaq_static_accuracy = output.accuracy;
                    _cachedData.iaq_static = output.signal; break;
                case BSEC_OUTPUT_STABILIZATION_STATUS:
                    _cachedData.stabilization_status = output.signal; break;
                case BSEC_OUTPUT_RUN_IN_STATUS:
                    _cachedData.run_in_status = output.signal; break;
            }
        }
        _hasNewData = true;
        _cachedData.is_valid = true;
    }

    void _checkBME688Status() const {
        if (_bme688.status < BSEC_OK) {
            Serial.print("BSEC error code : ");
            Serial.println(_bme688.status);
        } else if (_bme688.status > BSEC_OK) {
            Serial.print("BSEC warning code : ");
            Serial.println(_bme688.status);
        }

        if (_bme688.sensor.status < BME68X_OK) {
            Serial.print("BME68X error code : ");
            Serial.println(_bme688.sensor.status);
        } else if (_bme688.sensor.status > BME68X_OK) {
            Serial.print("BME68X warning code : ");
            Serial.println(_bme688.sensor.status);
        }
    }

};