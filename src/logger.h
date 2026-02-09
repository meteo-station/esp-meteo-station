#pragma once
#include <Arduino.h>

enum LogLevel {
    LOG_NONE = 0, LOG_ERROR = 1, LOG_WARNING = 2, LOG_INFO = 3, LOG_DEBUG = 4
};

inline LogLevel globalLogLevel = LOG_LEVEL;

inline void log(LogLevel level, const char *format, ...) {
    if (level <= globalLogLevel) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // Печатаем время
        Serial.print(millis());

        // Выбираем префикс
        switch (level) {
            case LOG_ERROR: Serial.print(" [ERROR] ");
                break;
            case LOG_WARNING: Serial.print(" [WARN] ");
                break;
            case LOG_INFO: Serial.print(" [INFO] ");
                break;
            case LOG_DEBUG: Serial.print(" [DEBUG] ");
                break;
            default: break;
        }

        Serial.println(buffer);
    }
}
