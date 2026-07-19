#pragma once
#include <AutoOTA.h>
#include <SettingsGyver.h>
#include <WiFiConnector.h>

#include "app.h"
#include "config.h"
#include "db.h"

// Берем версию из конфига
AutoOTA ota(VERSION, PROJECT_JSON_PATH);
SettingsGyver sett(PROJECT_NAME, &db);

void build(sets::Builder& b) {
    {
        sets::Group g(b, "WiFi");
        b.Input(kk::wifi_ssid, "SSID");
        b.Pass(kk::wifi_pass, "Pass", "");

        if (b.Button("wifi_save"_h, "Подключить")) {
            db.update();
            WiFiConnector.connect(db[kk::wifi_ssid], db[kk::wifi_pass]);
        }
    }
    {
        sets::Group g(b, "MQTT");
        b.Input(kk::mqtt_address, "Address");
        b.Input(kk::mqtt_port, "Port");
        b.Input(kk::mqtt_user, "User");
        b.Pass(kk::mqtt_pass, "Pass", "");

        if (b.Button("mqtt_save"_h, "Сохранить")) {
            db.update();
            mqtt_client.changeURL(db[kk::mqtt_address], db[kk::mqtt_port].toInt16(), db[kk::mqtt_user], db[kk::mqtt_pass]);
        }
    }
    {
        sets::Group g(b, "BME688");

        // Калибровка BSEC привязана к помещению. При переезде ее надо сбросить,
        // иначе старое состояние будет тянуть показания воздуха назад
        if (b.Confirm("bme688_reset"_h, "Сбросить калибровку BME688 и перезагрузиться?")) {
            BME688::resetSavedState();
            ESP.restart();
        }
    }

    if (b.Confirm("update"_h)) ota.update();
}

void update(sets::Updater& u) {
    if (ota.hasUpdate()) u.update("update"_h, "Доступно обновление. Обновить прошивку?");
}

void sett_tick() {

    // Тикаем настройки
    sett.tick();

    // Тикаем автообновление
    ota.tick();
}