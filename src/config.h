#pragma once

#include <Arduino.h>

#define VERSION "v0.0.0"
#define PROJECT_NAME "MeteoStation"
#define PROJECT_JSON_PATH "project_local.json"

// I2C адреса
#define I2C_ADDR_BME280   0x76

// Настройки BME688
#define BME688_SAMPLE_RATE		BSEC_SAMPLE_RATE_LP

#define IS_HTTPS_ENABLED false