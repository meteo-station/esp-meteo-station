#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

#include "config.h"
#include "logger.h"

class MQTTClient {
public:
    MQTTClient() {
        _client.onConnect([this](bool sessionPresent) {
            log(LOG_INFO, "Connected to MQTT");
            _connecting = false; // Сбрасываем флаг попытки
        });

        _client.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
            log(LOG_ERROR, "Disconnected from MQTT. Reason: %d", (int)reason);
            _connecting = false; // Готовы пробовать снова
        });
        _client.onPublish([this](uint16_t packetId) {
            _pendingMessages--; // Пакет успешно улетел
        });
    }

    void begin(const String &serverURL, uint16_t port = 1883, const String &username = "", const String &password = "") {
        _server = serverURL;
        _port = port;
        _username = username;
        _password = password;

        _client.setServer(_server.c_str(), _port);
        _client.setKeepAlive(60);
        _client.setCredentials(_username.c_str(), _password.c_str());
        log(LOG_INFO, "MQTT Client ID: %s", PROJECT_NAME);
        _client.setClientId(PROJECT_NAME);

        // Первая попытка подключения
        _connectToMqtt();
    }

    void tick(uint32_t now) {

        // Даем перехватить управление сетевому стеку
        delay(5);

        if (_client.connected() && now - _lastLogTime > 5000) {
            _lastLogTime = now;
            log(LOG_DEBUG, "Pending messages in MQTT queue: %d", _pendingMessages);
        }

        // Если уже подключены или прямо сейчас в процессе коннекта — ничего не делаем
        if (_client.connected() || _connecting) {
            return;
        }

        // Если WiFi есть, а MQTT нет — пробуем подключиться
        if (WiFi.status() == WL_CONNECTED) {
            // Чтобы не долбить брокера слишком часто (если он лежит),
            // всё же лучше делать паузу в 1-2 секунды между попытками
            if (now - _lastConnectAttempt > 2000) {
                _lastConnectAttempt = now;
                _connecting = true; // Ставим флаг, чтобы не вызывать connect() повторно в следующем loop
                log(LOG_INFO, "Starting connection to MQTT...");
                _client.connect();
            }
        }
    }
    bool sendMeteoData(const char *topic, const char *payload, bool retain = true) {
        if (!_client.connected()) return false;

        uint16_t packetId = _client.publish(topic, 1, retain, payload);
        if (packetId > 0) {
            _pendingMessages++;
            return true;
        }
        return false;
    }

    bool isConnected() {
        return _client.connected();
    }

    void changeURL(const String &serverURL, uint16_t port = 1883, const String &username = "", const String &password = "") {
        _server = serverURL;
        _port = port;
        _username = username;
        _password = password;

        _client.setServer(_server.c_str(), _port);

    }

private:
    void _connectToMqtt() {
        if (WiFi.status() == WL_CONNECTED) {
            log(LOG_INFO, "Connecting to MQTT...");
            _client.connect();
        }
    }

    AsyncMqttClient _client;
    bool _connecting = false;
    int _pendingMessages = 0;
    uint32_t _lastLogTime = 0;

    String _username;
    String _password;
    String _server;
    uint16_t _port;

    uint32_t _lastConnectAttempt = 0;
    static constexpr uint32_t MQTT_RETRY_INTERVAL = 10000;
};
