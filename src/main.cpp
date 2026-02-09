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
        log(LOG_INFO, "Local IP: %s", WiFi.localIP().toString().c_str());

        // Проверяем обновления прошивки
        ota.checkUpdate();
    });

    // Регистрируем коллбэк на ошибку подключения к Wi-fi
    WiFiConnector.onError([]() {
        // Пишем в консоль
        log(LOG_ERROR, "WiFi error");
    });

    db_init();

    // Подключаемся к Wi-Fi по заданным из бд названию и паролю
    WiFiConnector.connect(db[kk::wifi_ssid], db[kk::wifi_pass]);

    log(LOG_INFO, "Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {

        WiFiConnector.tick();
        delay(500);

        if (millis() > 30000) {

            log(LOG_ERROR, "WiFi connection timeout!");
            break;
        }
    }
    log(LOG_INFO, "Connected!");


    // Инициализация настроек и сервера
    sett.begin(true, "esp");

    // Регистрируем коллбэк построения меню настроек
    sett.onBuild(build);

    // Регистрируем коллбэк обновления настроек
    sett.onUpdate(update);

    app.begin();
    digitalWrite(LED_BUILTIN, HIGH);

    delay(500);
}

void loop() {
    sett.tick();
    WiFiConnector.tick();
    app.tick();
}
