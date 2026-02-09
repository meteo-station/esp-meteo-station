#include <Arduino.h>
#include "app.h"
#include <WiFiConnector.h>
#include <SettingsGyver.h>
#include <AutoOTA.h>
#include "settings.h"

App app(sensors, mqtt_client, db);

void setup() {
    Serial.begin(115200);

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

    // Инициализация настроек и сервера
    sett.begin(true, "esp");

    // Регистрируем коллбэк построения меню настроек
    sett.onBuild(build);

    // Регистрируем коллбэк обновления настроек
    sett.onUpdate(update);

    app.begin();
}

void loop() {
    sett.tick();
    WiFiConnector.tick();
    app.tick();
}
