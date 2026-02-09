#pragma once

struct BME280Data {
    float temperature;
    float pressure;
    bool is_valid;
};