#pragma once

#include <LittleFS.h>

#include "bsec2.h"
#include "logger.h"
#include "../../src/model/BME688Data.h"

class BME688 {
public:
    // Файл с сохраненным состоянием калибровки BSEC
    static constexpr const char *STATE_FILE = "/bsec_state.bin";

    // Формат файла. Поднимай версию, если меняется набор подписок или
    // версия BSEC — старое состояние тогда само отбросится
    static constexpr uint32_t STATE_MAGIC = 0x42535543; // "BSUC"
    static constexpr uint8_t STATE_VERSION = 1;

    // Как часто перезаписываем состояние. Блоб ~238 байт, но LittleFS стирает
    // сектор 4 КБ целиком, а ресурс флеша конечен — чаще нельзя
    static constexpr uint32_t STATE_SAVE_PERIOD = 6UL * 60UL * 60UL * 1000UL;

    // Сохраняем только полностью откалиброванное состояние
    static constexpr uint8_t STATE_MIN_ACCURACY = 3;

    BME688() = default;

    // Сброс калибровки — например при переезде в другое помещение.
    // Возвращает true, если файл действительно был удален
    static bool resetSavedState() {
        if (!LittleFS.exists(STATE_FILE)) {
            log(LOG_INFO, "BSEC state file not found, nothing to reset");
            return false;
        }
        if (!LittleFS.remove(STATE_FILE)) {
            log(LOG_ERROR, "Failed to remove BSEC state file");
            return false;
        }
        log(LOG_INFO, "BSEC state reset");
        return true;
    }

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

        if (!_bme688.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
            _checkBME688Status();
        }

        // Восстанавливаем калибровку до updateSubscription (порядок из примеров Bosch)
        _loadState();

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
        _maybeSaveState();
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

    uint32_t _lastStateSave = 0;
    bool _hasSavedState = false;

    // Заголовок файла состояния — отсекает мусор и блобы от старой прошивки
    struct StateHeader {
        uint32_t magic;
        uint8_t version;
        uint16_t length;
    };

    void _loadState() {
        File file = LittleFS.open(STATE_FILE, "r");
        if (!file) {
            log(LOG_INFO, "No saved BSEC state, starting calibration from scratch");
            return;
        }

        StateHeader header{};
        if (file.read((uint8_t *) &header, sizeof(header)) != sizeof(header) ||
            header.magic != STATE_MAGIC ||
            header.version != STATE_VERSION ||
            header.length != BSEC_MAX_STATE_BLOB_SIZE) {
            file.close();
            log(LOG_WARNING, "Saved BSEC state is incompatible, discarding it");
            LittleFS.remove(STATE_FILE);
            return;
        }

        uint8_t state[BSEC_MAX_STATE_BLOB_SIZE] = {};
        size_t read = file.read(state, sizeof(state));
        file.close();

        if (read != sizeof(state)) {
            log(LOG_WARNING, "Saved BSEC state is truncated, discarding it");
            LittleFS.remove(STATE_FILE);
            return;
        }

        if (!_bme688.setState(state)) {
            _checkBME688Status();
            return;
        }

        // Состояние принято — следующая перезапись через полный период
        _hasSavedState = true;
        _lastStateSave = millis();
        log(LOG_INFO, "BSEC state restored");
    }

    void _maybeSaveState() {
        // Несошедшуюся калибровку сохранять нельзя — она только собьет алгоритм при старте
        if (_cachedData.iaq_accuracy < STATE_MIN_ACCURACY) return;

        uint32_t now = millis();

        // Первый выход на полную точность сохраняем сразу, дальше — по таймеру.
        // Разность, а не сравнение абсолютных значений — millis() переполняется через ~49 дней
        if (_hasSavedState && (now - _lastStateSave < STATE_SAVE_PERIOD)) return;

        _lastStateSave = now;
        _hasSavedState = true;
        _saveState();
    }

    void _saveState() {
        uint8_t state[BSEC_MAX_STATE_BLOB_SIZE] = {};
        if (!_bme688.getState(state)) {
            _checkBME688Status();
            return;
        }

        File file = LittleFS.open(STATE_FILE, "w");
        if (!file) {
            log(LOG_ERROR, "Failed to open BSEC state file for writing");
            return;
        }

        StateHeader header{STATE_MAGIC, STATE_VERSION, BSEC_MAX_STATE_BLOB_SIZE};
        bool ok = file.write((const uint8_t *) &header, sizeof(header)) == sizeof(header) &&
                  file.write(state, sizeof(state)) == sizeof(state);
        file.close();

        if (ok) {
            log(LOG_INFO, "BSEC state saved");
        } else {
            log(LOG_ERROR, "Failed to write BSEC state");
        }
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