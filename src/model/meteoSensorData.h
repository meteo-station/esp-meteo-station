#pragma once
#include "BME280Data.h"
#include "BME688Data.h"
#include "HTU21DData.h"

struct MeteoSensorData {
    BME280Data bme280;
    HTU21DData htu21d;
    BME688Data bme688;
};
