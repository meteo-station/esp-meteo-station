#pragma once
#include <cstdint>

struct SCD41Data {
    float temperature;
    float humidity;
    uint16_t co2;
    bool is_valid;
};
