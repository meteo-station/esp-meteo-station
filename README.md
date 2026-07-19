# MeteoStation

Домашняя метеостанция на базе ESP8266 (D1 Mini). Собирает данные с окружающей среды и публикует их в MQTT-брокер каждую секунду.

## Железо

| Компонент | Назначение |
|-----------|-----------|
| ESP8266 D1 Mini | Основной микроконтроллер |
| BME280 (0x76) | Температура, давление |
| HTU21D | Температура, влажность |
| BME688 + BSEC2 | Температура, влажность, давление, IAQ, eCO2, eVOC |

Все датчики подключены по I2C.

## Данные

Данные публикуются в топик `esp-meteo-station/01/data` в формате JSON:

```json
{
  "bme280_t": 22.5,
  "bme280_p": 1013.2,
  "htu21d_t": 22.3,
  "htu21d_h": 48.1,
  "bme688_t": 22.4,
  "bme688_p": 1013.1,
  "bme688_h": 47.8,
  "bme688_eco2": 650.0,
  "bme688_evo2_acc": 2,
  "bme688_evoc": 0.5,
  "bme688_evoc_acc": 2,
  "bme688_iaq": 75.0,
  "bme688_iaq_acc": 2,
  "bme688_iaq_stat": 75.0,
  "bme688_iaq_stat_acc": 2,
  "bme688_gas_perc": 85.0,
  "bme688_gas_perc_acc": 2,
  "bme688_stab_stat": 1.0,
  "bme688_run_in_stat": 1.0
}
```

Поля с нулевой точностью (`*_acc == 0`) не включаются в payload — датчик ещё не стабилизировался.

## Настройка

Конфигурация хранится в LittleFS (`settings.db`) и доступна через веб-интерфейс.

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `wifi_ssid` | — | Название WiFi-сети |
| `wifi_pass` | — | Пароль WiFi |
| `mqtt_address` | `mqtt.bonavii.com` | Адрес MQTT-брокера |
| `mqtt_port` | `8883` | Порт MQTT-брокера |
| `mqtt_user` | — | Логин MQTT |
| `mqtt_pass` | — | Пароль MQTT |

После первой прошивки устройство поднимает точку доступа — подключитесь к ней и откройте веб-интерфейс для настройки.

## Сборка и прошивка

Проект собирается через [PlatformIO](https://platformio.org/).

```bash
# Собрать и прошить
pio run -t upload

# Загрузить файловую систему (настройки)
pio run -t uploadfs

# Монитор порта
pio device monitor
```

## Индикация

Встроенный светодиод мигает, когда нет подключения к MQTT-брокеру.

## Структура проекта

```
src/
├── main.cpp          # Точка входа
├── app.h             # Основная логика: опрос датчиков, публикация
├── meteoSensors.h    # Абстракция над датчиками
├── mqtt.h            # MQTT-клиент с авторекоинектом
├── db.h              # Конфигурационная БД (LittleFS)
├── settings.h        # Веб-интерфейс настроек + OTA
├── config.h          # Константы
├── logger.h          # Логирование в Serial
└── model/            # Структуры данных датчиков
lib/
└── BME688/           # Обёртка над Bosch BSEC2
```

## Зависимости

- [GyverBME280](https://github.com/GyverLibs/GyverBME280)
- [GyverHTU21D](https://github.com/GyverLibs/GyverHTU21D)
- [AsyncMqttClient](https://github.com/marvinroger/async-mqtt-client)
- [ArduinoJson](https://arduinojson.org/)
- [GyverDB](https://github.com/GyverLibs/GyverDB)
- [Settings (GyverLibs)](https://github.com/GyverLibs/Settings)
- [AutoOTA](https://github.com/GyverLibs/AutoOTA)
- [WiFiConnector](https://github.com/GyverLibs/WiFiConnector)
- [BSEC2](https://www.bosch-sensortec.com/software-tools/software/bme688-software/)
- [BME68x Sensor library](https://github.com/boschsensortec/BME68x_SensorAPI)