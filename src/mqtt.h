#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#include "config.h"

class MQTTClient {
public:
    MQTTClient(): client(wifiClient) {
    }

    void begin(const String &serverURL, uint16_t port = 1883, const String &username = "", const String &password = "") {
        wifiClient.setInsecure();
        _server = serverURL;
        _port = port;
        _username = username;
        _password = password;

        client.setServer(_server.c_str(), _port);
    }

    void changeURL(const String &serverURL, uint16_t port = 1883, const String &username = "", const String &password = "") {
        _server = serverURL;
        _port = port;
        _username = username;
        _password = password;

        client.setServer(_server.c_str(), _port);
    }

    void tick(uint32_t now) {
        if (!client.connected()) {
            _reconnect(now);
        }
        client.loop();
    }

    bool sendMeteoData(const char *topic, const char *payload, bool retain = true) {
        if (!client.connected()) {
            return false;
        }
        return client.publish(topic, payload, retain);
    }

private:
    bool _reconnect(uint32_t now) {
        if (now - lastConnectAttempt < MQTT_RETRY_INTERVAL) {
            return false;
        }
        lastConnectAttempt = now;

        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }

        if (client.connect(PROJECT_NAME, _username.c_str(), _password.c_str())) {
            Serial.println("Connected to mqtt");
            return true;
        } else {
            Serial.print("FAIL connect to mqtt rc=");
            Serial.println(client.state());
            return false;
        }
    }

    WiFiClientSecure wifiClient;
    PubSubClient client;

    String _username;
    String _password;
    String _server;
    uint16_t _port;

    uint32_t lastConnectAttempt = 0;
    static constexpr uint32_t MQTT_RETRY_INTERVAL = 2000;
};
