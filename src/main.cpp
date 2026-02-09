#include <Arduino.h>
#include "app.h"
#include <WiFiConnector.h>
#include <SettingsGyver.h>
#include <AutoOTA.h>
#include "settings.h"

App app(sensors, mqtt_client, db);

void setup() {
    Serial.begin(115200);
    while(!Serial) delay(10);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    WiFiConnector.onConnect([]() {
        // Пишем в консоль
        Serial.print("Local IP: ");
        Serial.println(WiFi.localIP());

        // Проверяем обновления прошивки
        ota.checkUpdate();
    });

    // Регистрируем коллбэк на ошибку подключения к Wi-fi
    WiFiConnector.onError([]() {
        // Пишем в консоль
        Serial.println("WiFi error");
    });

    db_init();

    // Подключаемся к Wi-Fi по заданным из бд названию и паролю
    WiFiConnector.connect(db[kk::wifi_ssid], db[kk::wifi_pass]);

    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {

        WiFiConnector.tick();
        delay(500);
        Serial.print(".");

        if (millis() > 30000) {

            Serial.println("WiFi connection timeout!");
            break;
        }
    }
    Serial.println("Connected!");


    // Инициализация настроек и сервера
    sett.begin(true, "esp");

    // Регистрируем коллбэк построения меню настроек
    sett.onBuild(build);

    // Регистрируем коллбэк обновления настроек
    sett.onUpdate(update);

    app.begin();
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    sett.tick();
    WiFiConnector.tick();
    app.tick();
}
