#pragma once

#include <Arduino.h>

#include <ArduinoJson.h>
#include "db.h"
#include "meteoSensors.h"
#include "mqtt.h"

MeteoSensors sensors;
MQTTClient mqtt_client;

class App {
public:
    // Конструктор принимает все объекты по ссылке
    App(MeteoSensors &sensors,
        MQTTClient &mqttClient,
        GyverDBFile &db
    )
        : _meteoSensors(sensors),
          _mqttClient(mqttClient),
          _db(db) {
    }

    void begin() {
        _meteoSensors.begin();
        _mqttClient.begin(_db[mqtt_address], _db[mqtt_port].toInt16(), _db[mqtt_user], _db[mqtt_pass]);
    }

    void tick() {
        uint32_t now = millis();

        // Проверяем статус мигания
        _checkBlinkStatus();

        // Поддерживаем коннект MQTT
        _mqttClient.tick(now);

        // Тикаем сенсоры, чтобы все работало
        _meteoSensors.tick();

        // Если нет коннекта MQTT
        if (!_mqttClient.isConnected()) {

            // Мигаем и выходим
            _triggerBlink();
            return;
        }

        // Раз в секунду
        if (now - lastMessageSentAt >= 1000) {
            lastMessageSentAt = now;

            // Читаем сенсоры
            _meteoSensors.readMeteoData(now, _sensorData);

            // Формируем JSON, убеждаемся, что он не пустой
            if (_buildMeteoDataJSON(payload, sizeof(payload)) && strcmp(payload, "{}") != 0) {

                // Логгируем JSON
                Serial.println(payload);

                // Отправляем данные
                if (_mqttClient.sendMeteoData("esp-meteo-station/01/data", payload) == false) {
                    Serial.println("Failed to send meteo data");
                }
            }
        }
    }

private:
    MeteoSensorData _sensorData;

    char payload[512];

    uint32_t lastMessageSentAt = 0;
    bool _isBlinking = false;
    uint32_t _lastBlinkChangingStateTime = 0;

    MeteoSensors &_meteoSensors;
    MQTTClient &_mqttClient;
    GyverDBFile &_db;

    void _checkBlinkStatus() {
        if (_isBlinking && (millis() - _lastBlinkChangingStateTime >= 100)) {
            digitalWrite(LED_BUILTIN, HIGH);
            _isBlinking = false;
            _lastBlinkChangingStateTime = millis();
        }
    }

    void _triggerBlink() {
        if (!_isBlinking && (millis() - _lastBlinkChangingStateTime >= 100)) {
            digitalWrite(LED_BUILTIN, LOW);
            _lastBlinkChangingStateTime = millis();
            _isBlinking = true;
        }
    }

    bool _buildMeteoDataJSON(
        char *buffer,
        size_t bufferSize
    ) {
        JsonDocument doc;

        doc["bme280_t"] = _sensorData.bme280.temperature;
        doc["bme280_p"] = _sensorData.bme280.pressure;

        doc["htu21d_t"] = _sensorData.htu21d.temperature;
        doc["htu21d_h"] = _sensorData.htu21d.humidity;

        if (_sensorData.bme688.is_valid) {
            doc["bme688_t"] = _sensorData.bme688.temperature;
            doc["bme688_p"] = _sensorData.bme688.pressure;
            doc["bme688_h"] = _sensorData.bme688.humidity;
            doc["bme688_eco2"] = _sensorData.bme688.eco2;
            doc["bme688_evo2_acc"] = _sensorData.bme688.evoc_accuracy;
            doc["bme688_evoc"] = _sensorData.bme688.evoc;
            doc["bme688_evoc_acc"] = _sensorData.bme688.evoc_accuracy;
            doc["bme688_gas_perc"] = _sensorData.bme688.gas_percentage;
            doc["bme688_gas_perc_acc"] = _sensorData.bme688.gas_percentage_accuracy;
            doc["bme688_iaq"] = _sensorData.bme688.iaq;
            doc["bme688_iaq_acc"] = _sensorData.bme688.iaq_accuracy;
            doc["bme688_iaq_stat"] = _sensorData.bme688.iaq_static;
            doc["bme688_iaq_stat_acc"] = _sensorData.bme688.iaq_static_accuracy;
            doc["bme688_stab_stat"] = _sensorData.bme688.stabilization_status;
            doc["bme688_run_in_stat"] = _sensorData.bme688.run_in_status;
        }

        return serializeJson(doc, buffer, bufferSize) > 0;
    }
};
