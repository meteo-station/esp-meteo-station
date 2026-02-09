#pragma once

struct BME688Data {
    float temperature;
    float humidity;
    float pressure;
    float gas_percentage;
    uint8_t gas_percentage_accuracy;
    float iaq;
    uint8_t iaq_accuracy;
    float eco2;
    uint8_t eco2_accuracy;
    float iaq_static;
    uint8_t iaq_static_accuracy;
    float evoc;
    uint8_t evoc_accuracy;
    float stabilization_status;
    float run_in_status;

    bool is_valid;
};
