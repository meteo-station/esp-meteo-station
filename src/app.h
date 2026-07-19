#pragma once

#include <Arduino.h>

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

            // Читаем сенсоры. Если свежих данных нет — отправлять нечего
            if (!_meteoSensors.readMeteoData(now, _sensorData)) {
                return;
            }

            _publishMeteoData();
        }
    }

private:
    MeteoSensorData _sensorData;

    // Каждая метрика едет в свой топик: <base>/<датчик>/<метрика>, payload —
    // голое число. Раньше всё уезжало одним JSON в <base>/data, и потребители
    // (HA, telegraf, go-exporter) разбирали его сами. Проблема была в том, что
    // объект собирался только из валидных в этом цикле значений: датчик не
    // ответил или BSEC ещё не откалибровался — ключ просто отсутствовал, и на
    // той стороне шаблоны падали на "dict object has no attribute", сбрасывая
    // состояние. Отдельный топик на метрику убирает сам класс проблемы: нет
    // данных — нет публикации, а retained хранит последнее известное значение.
    static constexpr const char *MQTT_BASE_TOPIC = "esp-meteo-station/01";

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

    // Публикация одной метрики. Топик и payload собираются в локальные буферы:
    // держать их полями класса смысла нет, а на стеке ESP8266 32+16 байт дешевле,
    // чем постоянно занятая память.
    void _publishMetric(const char *sensor, const char *metric, float value, uint8_t decimals) {
        char topic[64];
        char payload[16];

        snprintf(topic, sizeof(topic), "%s/%s/%s", MQTT_BASE_TOPIC, sensor, metric);
        // %.*f вместо dtostrf: снимает вопрос с размером буфера под мантиссу.
        snprintf(payload, sizeof(payload), "%.*f", decimals, value);

        if (!_mqttClient.sendMeteoData(topic, payload)) {
            log(LOG_ERROR, "Failed to publish %s", topic);
        }
    }

    void _publishMeteoData() {
        if (_sensorData.bme280.is_valid) {
            _publishMetric("bme280", "t", _sensorData.bme280.temperature, 2);
            _publishMetric("bme280", "p", _sensorData.bme280.pressure, 2);
        }

        if (_sensorData.htu21d.is_valid) {
            _publishMetric("htu21d", "t", _sensorData.htu21d.temperature, 2);
            _publishMetric("htu21d", "h", _sensorData.htu21d.humidity, 2);
        }

        if (_sensorData.bme688.is_valid) {
            _publishMetric("bme688", "t", _sensorData.bme688.temperature, 2);
            _publishMetric("bme688", "p", _sensorData.bme688.pressure, 2);
            _publishMetric("bme688", "h", _sensorData.bme688.humidity, 2);

            // Значения публикуем только при ненулевой accuracy — до этого
            // BSEC отдает несошедшуюся калибровку. Саму accuracy не шлем.
            // Раньше это было главным источником "дырявых" сообщений: поле
            // то появлялось, то исчезало внутри одного и того же JSON.
            if (_sensorData.bme688.eco2_accuracy != 0) {
                _publishMetric("bme688", "eco2", _sensorData.bme688.eco2, 1);
            }

            if (_sensorData.bme688.evoc_accuracy != 0) {
                _publishMetric("bme688", "evoc", _sensorData.bme688.evoc, 3);
            }

            if (_sensorData.bme688.gas_percentage_accuracy != 0) {
                _publishMetric("bme688", "gas_perc", _sensorData.bme688.gas_percentage, 1);
            }

            if (_sensorData.bme688.iaq_accuracy != 0) {
                _publishMetric("bme688", "iaq", _sensorData.bme688.iaq, 1);
            }

            if (_sensorData.bme688.iaq_static_accuracy != 0) {
                _publishMetric("bme688", "iaq_stat", _sensorData.bme688.iaq_static, 1);
            }
        }

        if (_sensorData.scd41.is_valid) {
            _publishMetric("scd41", "t", _sensorData.scd41.temperature, 2);
            _publishMetric("scd41", "h", _sensorData.scd41.humidity, 2);
            _publishMetric("scd41", "co2", (float) _sensorData.scd41.co2, 0);
        }
    }
};
